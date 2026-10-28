#include "area512_hal.h"
#include "console_internal.h"

#include <stdio.h>
#include <string.h>

#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "sdkconfig.h"

extern "C" void ble_store_config_init(void);

static constexpr const char *BLUETOOTH_CONFIG_PATH = "etc/bluetooth";
static constexpr int BLUETOOTH_CONFIG_LINE_BYTE_CAPACITY = 32;
static constexpr const char *BLUETOOTH_BOND_PATH = "etc/bluetooth_bonds";
static constexpr int BOND_FILE_RECORD_CAPACITY = CONFIG_BT_NIMBLE_MAX_BONDS * 2;

static constexpr int SCAN_DURATION_MILLISECONDS = 5000;
static constexpr int CONNECT_CANCEL_POLL_INTERVAL_MILLISECONDS = 10;
static constexpr int PAIRING_CONNECT_TIMEOUT_MILLISECONDS = 30000;
static constexpr int FOUND_KEYBOARD_CAPACITY = 9;
static constexpr int FOUND_KEYBOARD_LABEL_BYTE_SIZE = 32;
static constexpr int PAIRING_FAILURE_MESSAGE_BYTE_SIZE = 32;
static constexpr uint32_t PASSKEY_VALUE_COUNT = 1000000;

static constexpr uint8_t INCOMPLETE_UUID16_LIST_AD_TYPE = 0x02;
static constexpr uint8_t COMPLETE_UUID16_LIST_AD_TYPE = 0x03;
static constexpr uint8_t SHORTENED_LOCAL_NAME_AD_TYPE = 0x08;
static constexpr uint8_t COMPLETE_LOCAL_NAME_AD_TYPE = 0x09;
static constexpr uint8_t APPEARANCE_AD_TYPE = 0x19;
static constexpr uint16_t APPEARANCE_CATEGORY_MASK = 0xFFC0;
static constexpr uint16_t HID_APPEARANCE_CATEGORY = 0x03C0;

static constexpr uint16_t HID_SERVICE_UUID = 0x1812;
static constexpr uint16_t PROTOCOL_MODE_UUID = 0x2A4E;
static constexpr uint16_t REPORT_UUID = 0x2A4D;
static constexpr uint16_t BOOT_KEYBOARD_INPUT_REPORT_UUID = 0x2A22;
static constexpr uint16_t CLIENT_CHARACTERISTIC_CONFIGURATION_UUID = 0x2902;
static constexpr uint8_t BOOT_PROTOCOL_MODE = 0x00;
static constexpr uint8_t NOTIFICATION_ENABLED_VALUE[] = {0x01, 0x00};
static constexpr int INPUT_REPORT_CHARACTERISTIC_CAPACITY = 8;

static constexpr int KEYBOARD_REPORT_BYTE_SIZE = 8;
static constexpr int KEYBOARD_REPORT_FIRST_KEY_INDEX = 2;
static constexpr int KEYBOARD_REPORT_QUEUE_LENGTH = 8;
static constexpr uint8_t RELEASED_KEYBOARD_REPORT[KEYBOARD_REPORT_BYTE_SIZE] =
  {};

static constexpr uint8_t CTRL_MODIFIER_BITS = 0x11;
static constexpr uint8_t SHIFT_MODIFIER_BITS = 0x22;
static constexpr uint8_t ALT_MODIFIER_BITS = 0x44;

static constexpr uint8_t ERROR_ROLL_OVER_USAGE = 0x01;
static constexpr uint8_t A_USAGE = 0x04;
static constexpr uint8_t Z_USAGE = 0x1D;
static constexpr uint8_t ENTER_USAGE = 0x28;
static constexpr uint8_t ESCAPE_USAGE = 0x29;
static constexpr uint8_t BACKSPACE_USAGE = 0x2A;
static constexpr uint8_t TAB_USAGE = 0x2B;
static constexpr uint8_t SPACE_USAGE = 0x2C;
static constexpr uint8_t SLASH_USAGE = 0x38;
static constexpr uint8_t RIGHT_ARROW_USAGE = 0x4F;
static constexpr uint8_t LEFT_ARROW_USAGE = 0x50;
static constexpr uint8_t DOWN_ARROW_USAGE = 0x51;
static constexpr uint8_t UP_ARROW_USAGE = 0x52;
static constexpr uint8_t KEYPAD_ENTER_USAGE = 0x58;

static constexpr char UNSHIFTED_CHARACTERS[] =
  "abcdefghijklmnopqrstuvwxyz1234567890\0\0\0\0\0-=[]\\\0;'`,./";
static constexpr char SHIFTED_CHARACTERS[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()\0\0\0\0\0_+{}|\0:\"~<>?";

static_assert(sizeof UNSHIFTED_CHARACTERS == SLASH_USAGE - A_USAGE + 2);
static_assert(sizeof SHIFTED_CHARACTERS == SLASH_USAGE - A_USAGE + 2);

static constexpr uint8_t ASCII_BACKSPACE = 0x08;
static constexpr uint8_t ASCII_ESCAPE = 0x1B;

static constexpr int REPEAT_DELAY_MILLISECONDS = 450;
static constexpr int REPEAT_INTERVAL_MILLISECONDS = 55;

namespace {
struct FoundKeyboard {
  ble_addr_t address;
  char label[FOUND_KEYBOARD_LABEL_BYTE_SIZE];
};

struct AdvertisingDataFields {
  bool is_hid_device;
  const uint8_t *name;
  int name_length;
};

struct InputReportCharacteristic {
  uint16_t value_handle;
  uint16_t end_handle;
};

struct KeyboardAttributeHandles {
  uint16_t hid_service_start_handle;
  uint16_t hid_service_end_handle;
  uint16_t protocol_mode_value_handle;
  InputReportCharacteristic
    input_report_characteristics[INPUT_REPORT_CHARACTERISTIC_CAPACITY];
  int input_report_characteristic_count;
  uint16_t *unresolved_end_handle;
  uint16_t client_characteristic_configuration_handle;
};

struct BondFileRecord {
  uint8_t object_type;
  struct ble_store_value_sec security_value;
};
} // namespace

static bool s_is_enabled_in_config = false;
static bool s_is_host_synced = false;
static uint8_t s_own_address_type = 0;
static uint16_t s_connection_handle = BLE_HS_CONN_HANDLE_NONE;
static KeyboardAttributeHandles s_keyboard_attribute_handles = {};
static bool s_has_boot_keyboard_input_report_characteristic = false;
static int s_subscribing_input_report_characteristic_index = 0;
static int s_subscribed_input_report_characteristic_count = 0;

static BondFileRecord s_bond_file_records[BOND_FILE_RECORD_CAPACITY];
static int s_bond_file_record_count = 0;

static FoundKeyboard s_found_keyboards[FOUND_KEYBOARD_CAPACITY];
static int s_found_keyboard_count = 0;
static SemaphoreHandle_t s_scan_completed_semaphore = NULL;

static int s_pairing_state = AREA512_BLUETOOTH_PAIRING_INACTIVE;
static uint32_t s_pairing_passkey = 0;
static char s_pairing_failure_message[PAIRING_FAILURE_MESSAGE_BYTE_SIZE] = "";

static QueueHandle_t s_keyboard_report_queue = NULL;
static uint8_t s_previous_keyboard_report[KEYBOARD_REPORT_BYTE_SIZE] = {};
static uint8_t s_held_modifier_bits = 0;
static uint8_t s_repeat_usage = 0;
static TickType_t s_repeat_next_tick = 0;

// -----------------------------------------------------------------------------
// Connection
// -----------------------------------------------------------------------------

static int dispatch_connection_event(struct ble_gap_event *event, void *);

static bool
is_pairing_in_progress(void) {
  return s_pairing_state == AREA512_BLUETOOTH_PAIRING_RUNNING ||
         s_pairing_state == AREA512_BLUETOOTH_PAIRING_WAITING_FOR_PASSKEY_ENTRY;
}

static void
record_pairing_failure(const char *failed_step, int status) {
  if (!is_pairing_in_progress() || s_pairing_failure_message[0] != '\0')
    return;

  snprintf(
    s_pairing_failure_message,
    sizeof s_pairing_failure_message,
    "%s 0x%x",
    failed_step,
    status
  );
}

static void
mark_pairing_failed_if_in_progress(void) {
  if (is_pairing_in_progress())
    s_pairing_state = AREA512_BLUETOOTH_PAIRING_FAILED;
}

static void
start_connecting_to_bonded_keyboard(void) {
  ble_addr_t bonded_addresses[CONFIG_BT_NIMBLE_MAX_BONDS];
  int bonded_address_count = 0;

  if (
    s_connection_handle != BLE_HS_CONN_HANDLE_NONE || is_pairing_in_progress()
  )
    return;

  if (
    ble_store_util_bonded_peers(
      bonded_addresses,
      &bonded_address_count,
      CONFIG_BT_NIMBLE_MAX_BONDS
    ) != 0 ||
    bonded_address_count == 0
  )
    return;

  if (
    ble_gap_wl_set(
      bonded_addresses,
      (uint8_t)bonded_address_count
    ) != 0
  )
    return;

  ble_gap_connect(
    s_own_address_type,
    NULL,
    BLE_HS_FOREVER,
    NULL,
    dispatch_connection_event,
    NULL
  );
}

static void
disconnect_keyboard(void) {
  ble_gap_terminate(
    s_connection_handle,
    BLE_ERR_REM_USER_CONN_TERM
  );
}

static void
abort_keyboard_connection(const char *failed_step, int status) {
  record_pairing_failure(failed_step, status);
  disconnect_keyboard();
}

static void
generate_and_inject_pairing_passkey(uint16_t connection_handle) {
  struct ble_sm_io passkey_io = {};

  if (!is_pairing_in_progress())
    return;

  passkey_io.action = BLE_SM_IOACT_DISP;
  passkey_io.passkey = esp_random() % PASSKEY_VALUE_COUNT;

  if (ble_sm_inject_io(connection_handle, &passkey_io) != 0)
    return;

  s_pairing_passkey = passkey_io.passkey;
  s_pairing_state = AREA512_BLUETOOTH_PAIRING_WAITING_FOR_PASSKEY_ENTRY;
}

static bool
is_input_report_characteristic_value_handle(uint16_t attribute_handle) {
  const KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;

  for (int i = 0; i < handles->input_report_characteristic_count; i++) {
    if (
      handles->input_report_characteristics[i].value_handle == attribute_handle
    )
      return true;
  }

  return false;
}

static void
queue_keyboard_report(
  uint16_t attribute_handle,
  struct os_mbuf *report_buffer
) {

  uint8_t keyboard_report[KEYBOARD_REPORT_BYTE_SIZE];

  if (
    !is_input_report_characteristic_value_handle(attribute_handle) ||
    OS_MBUF_PKTLEN(report_buffer) != KEYBOARD_REPORT_BYTE_SIZE
  )
    return;

  os_mbuf_copydata(
    report_buffer,
    0,
    KEYBOARD_REPORT_BYTE_SIZE,
    keyboard_report
  );

  xQueueSend(s_keyboard_report_queue, keyboard_report, 0);
}

// -----------------------------------------------------------------------------
// HID service discovery
// -----------------------------------------------------------------------------

static void
discover_next_client_characteristic_configuration_or_mark_pairing_succeeded(void);

static int
count_subscription_then_move_to_next_input_report_on_reply(
  uint16_t,
  const struct ble_gatt_error *error,
  struct ble_gatt_attr *,
  void *
) {

  if (error->status != 0) {
    abort_keyboard_connection("subscribe", error->status);

    return 0;
  }

  s_subscribed_input_report_characteristic_count++;
  s_subscribing_input_report_characteristic_index++;

  discover_next_client_characteristic_configuration_or_mark_pairing_succeeded();

  return 0;
}

static int
record_client_characteristic_configuration_then_enable_notification_on_reply(
  uint16_t,
  const struct ble_gatt_error *error,
  uint16_t,
  const struct ble_gatt_dsc *descriptor,
  void *
) {

  KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;
  int result;

  if (error->status == 0) {
    if (
      ble_uuid_u16(&descriptor->uuid.u) ==
      CLIENT_CHARACTERISTIC_CONFIGURATION_UUID
    )
      handles->client_characteristic_configuration_handle = descriptor->handle;

    return 0;
  }

  if (error->status != BLE_HS_EDONE) {
    abort_keyboard_connection("descriptors", error->status);

    return 0;
  }

  if (handles->client_characteristic_configuration_handle == 0) {
    s_subscribing_input_report_characteristic_index++;

    discover_next_client_characteristic_configuration_or_mark_pairing_succeeded();

    return 0;
  }

  result =
    ble_gattc_write_flat(
      s_connection_handle,
      handles->client_characteristic_configuration_handle,
      NOTIFICATION_ENABLED_VALUE,
      sizeof NOTIFICATION_ENABLED_VALUE,
      count_subscription_then_move_to_next_input_report_on_reply,
      NULL
    );

  if (result != 0)
    abort_keyboard_connection("subscribe", result);

  return 0;
}

static void
discover_next_client_characteristic_configuration_or_mark_pairing_succeeded(void) {
  KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;
  int result;

  while (
    s_subscribing_input_report_characteristic_index <
      handles->input_report_characteristic_count
  ) {

    const InputReportCharacteristic *input_report_characteristic =
      &handles->input_report_characteristics
         [s_subscribing_input_report_characteristic_index];

    if (
      input_report_characteristic->end_handle <=
        input_report_characteristic->value_handle
    ) {

      s_subscribing_input_report_characteristic_index++;

      continue;
    }

    handles->client_characteristic_configuration_handle = 0;

    result =
      ble_gattc_disc_all_dscs(
        s_connection_handle,
        input_report_characteristic->value_handle,
        input_report_characteristic->end_handle,
        record_client_characteristic_configuration_then_enable_notification_on_reply,
        NULL
      );

    if (result != 0)
      abort_keyboard_connection("descriptors", result);

    return;
  }

  if (s_subscribed_input_report_characteristic_count == 0) {
    abort_keyboard_connection("subscribe", 0);

    return;
  }

  if (is_pairing_in_progress())
    s_pairing_state = AREA512_BLUETOOTH_PAIRING_SUCCEEDED;
}

static void
record_hid_characteristic(const struct ble_gatt_chr *characteristic) {

  KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;
  uint16_t characteristic_uuid = ble_uuid_u16(&characteristic->uuid.u);

  bool is_input_report_characteristic =
    characteristic_uuid == BOOT_KEYBOARD_INPUT_REPORT_UUID ||
    (
      characteristic_uuid == REPORT_UUID &&
      (characteristic->properties & BLE_GATT_CHR_PROP_NOTIFY) != 0
    );

  InputReportCharacteristic *input_report_characteristic;

  if (handles->unresolved_end_handle != NULL) {
    *handles->unresolved_end_handle = characteristic->def_handle - 1;
    handles->unresolved_end_handle = NULL;
  }

  if (characteristic_uuid == PROTOCOL_MODE_UUID) {
    if (handles->protocol_mode_value_handle == 0)
      handles->protocol_mode_value_handle = characteristic->val_handle;

    return;
  }

  if (
    !is_input_report_characteristic ||
    handles->input_report_characteristic_count >=
      INPUT_REPORT_CHARACTERISTIC_CAPACITY
  )
    return;

  if (characteristic_uuid == BOOT_KEYBOARD_INPUT_REPORT_UUID)
    s_has_boot_keyboard_input_report_characteristic = true;

  input_report_characteristic =
    &handles->input_report_characteristics
       [handles->input_report_characteristic_count++];

  input_report_characteristic->value_handle = characteristic->val_handle;
  handles->unresolved_end_handle = &input_report_characteristic->end_handle;
}

static int
record_hid_characteristic_then_start_subscribing_on_reply(
  uint16_t,
  const struct ble_gatt_error *error,
  const struct ble_gatt_chr *characteristic,
  void *
) {

  KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;

  if (error->status == 0) {
    record_hid_characteristic(characteristic);

    return 0;
  }

  if (error->status != BLE_HS_EDONE) {
    abort_keyboard_connection("characteristics", error->status);

    return 0;
  }

  if (handles->unresolved_end_handle != NULL) {
    *handles->unresolved_end_handle = handles->hid_service_end_handle;
    handles->unresolved_end_handle = NULL;
  }

  if (
    s_has_boot_keyboard_input_report_characteristic &&
    handles->protocol_mode_value_handle != 0
  )
    ble_gattc_write_no_rsp_flat(
      s_connection_handle,
      handles->protocol_mode_value_handle,
      &BOOT_PROTOCOL_MODE,
      sizeof BOOT_PROTOCOL_MODE
    );

  if (handles->input_report_characteristic_count == 0) {
    abort_keyboard_connection("no input report", 0);

    return 0;
  }

  discover_next_client_characteristic_configuration_or_mark_pairing_succeeded();

  return 0;
}

static int
record_hid_service_then_discover_characteristics_on_reply(
  uint16_t,
  const struct ble_gatt_error *error,
  const struct ble_gatt_svc *service,
  void *
) {

  KeyboardAttributeHandles *handles = &s_keyboard_attribute_handles;
  int result;

  if (error->status == 0) {
    if (handles->hid_service_start_handle == 0)
      handles->hid_service_start_handle = service->start_handle;

    handles->hid_service_end_handle = service->end_handle;

    return 0;
  }

  if (error->status != BLE_HS_EDONE) {
    abort_keyboard_connection("hid service", error->status);

    return 0;
  }

  if (handles->hid_service_start_handle == 0) {
    abort_keyboard_connection("no hid service", 0);

    return 0;
  }

  result =
    ble_gattc_disc_all_chrs(
      s_connection_handle,
      handles->hid_service_start_handle,
      handles->hid_service_end_handle,
      record_hid_characteristic_then_start_subscribing_on_reply,
      NULL
    );

  if (result != 0)
    abort_keyboard_connection("characteristics", result);

  return 0;
}

static void
discover_hid_services(void) {
  static const ble_uuid16_t hid_service_uuid =
    BLE_UUID16_INIT(HID_SERVICE_UUID);

  int result;

  result =
    ble_gattc_disc_svc_by_uuid(
      s_connection_handle,
      &hid_service_uuid.u,
      record_hid_service_then_discover_characteristics_on_reply,
      NULL
    );

  if (result != 0)
    abort_keyboard_connection("hid service", result);
}

static int
dispatch_connection_event(struct ble_gap_event *event, void *) {
  struct ble_gap_conn_desc connection_description;
  int result;

  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    if (event->connect.status != 0) {
      if (event->connect.status != BLE_HS_EAPP) {
        record_pairing_failure("connect", event->connect.status);
        mark_pairing_failed_if_in_progress();
      }

      start_connecting_to_bonded_keyboard();

      return 0;
    }

    s_connection_handle = event->connect.conn_handle;
    s_keyboard_attribute_handles = {};
    s_has_boot_keyboard_input_report_characteristic = false;
    s_subscribing_input_report_characteristic_index = 0;
    s_subscribed_input_report_characteristic_count = 0;

    result = ble_gap_security_initiate(s_connection_handle);

    if (result != 0 && result != BLE_HS_EALREADY)
      abort_keyboard_connection("security", result);

    return 0;

  case BLE_GAP_EVENT_DISCONNECT:
    s_connection_handle = BLE_HS_CONN_HANDLE_NONE;

    xQueueSend(s_keyboard_report_queue, RELEASED_KEYBOARD_REPORT, 0);
    record_pairing_failure("disconnect", event->disconnect.reason);
    mark_pairing_failed_if_in_progress();
    start_connecting_to_bonded_keyboard();

    return 0;

  case BLE_GAP_EVENT_ENC_CHANGE:
    if (event->enc_change.status != 0) {
      abort_keyboard_connection("encryption", event->enc_change.status);

      return 0;
    }

    discover_hid_services();

    return 0;

  case BLE_GAP_EVENT_PASSKEY_ACTION:
    if (event->passkey.params.action == BLE_SM_IOACT_DISP)
      generate_and_inject_pairing_passkey(event->passkey.conn_handle);

    return 0;

  case BLE_GAP_EVENT_REPEAT_PAIRING:
    if (
      ble_gap_conn_find(
        event->repeat_pairing.conn_handle,
        &connection_description
      ) == 0
    )
      ble_store_util_delete_peer(&connection_description.peer_id_addr);

    return BLE_GAP_REPEAT_PAIRING_RETRY;

  case BLE_GAP_EVENT_NOTIFY_RX:
    queue_keyboard_report(event->notify_rx.attr_handle, event->notify_rx.om);

    return 0;

  default:
    return 0;
  }
}

// -----------------------------------------------------------------------------
// Scan
// -----------------------------------------------------------------------------

static uint16_t
read_little_endian_uint16(const uint8_t *bytes) {
  return (uint16_t)(bytes[0] | bytes[1] << 8);
}

static AdvertisingDataFields
parse_advertising_data(const uint8_t *advertising_data, int data_length) {
  AdvertisingDataFields fields = {};
  int ad_structure_offset = 0;

  while (ad_structure_offset < data_length) {
    int ad_structure_length = advertising_data[ad_structure_offset];

    if (
      ad_structure_length == 0 ||
      ad_structure_offset + 1 + ad_structure_length > data_length
    )
      break;

    uint8_t ad_type = advertising_data[ad_structure_offset + 1];
    const uint8_t *ad_data = &advertising_data[ad_structure_offset + 2];
    int ad_data_length = ad_structure_length - 1;

    switch (ad_type) {
    case INCOMPLETE_UUID16_LIST_AD_TYPE:
    case COMPLETE_UUID16_LIST_AD_TYPE:
      for (int i = 0; i + 1 < ad_data_length; i += 2) {
        if (read_little_endian_uint16(&ad_data[i]) == HID_SERVICE_UUID)
          fields.is_hid_device = true;
      }

      break;

    case APPEARANCE_AD_TYPE:
      if (
        ad_data_length == 2 &&
        (read_little_endian_uint16(ad_data) & APPEARANCE_CATEGORY_MASK) ==
          HID_APPEARANCE_CATEGORY
      )
        fields.is_hid_device = true;

      break;

    case SHORTENED_LOCAL_NAME_AD_TYPE:
    case COMPLETE_LOCAL_NAME_AD_TYPE:
      fields.name = ad_data;
      fields.name_length = ad_data_length;

      break;

    default:
      break;
    }

    ad_structure_offset += 1 + ad_structure_length;
  }

  return fields;
}

static FoundKeyboard *
find_found_keyboard(const ble_addr_t *address) {
  for (int i = 0; i < s_found_keyboard_count; i++) {
    if (ble_addr_cmp(&s_found_keyboards[i].address, address) == 0)
      return &s_found_keyboards[i];
  }

  return NULL;
}

static void
record_found_keyboard(const struct ble_gap_disc_desc *discovery) {
  AdvertisingDataFields fields =
    parse_advertising_data(discovery->data, discovery->length_data);
  FoundKeyboard *found_keyboard = find_found_keyboard(&discovery->addr);

  if (found_keyboard == NULL) {
    if (
      !fields.is_hid_device ||
      s_found_keyboard_count >= FOUND_KEYBOARD_CAPACITY
    )
      return;

    found_keyboard = &s_found_keyboards[s_found_keyboard_count++];
    found_keyboard->address = discovery->addr;

    snprintf(
      found_keyboard->label,
      sizeof found_keyboard->label,
      "%02x:%02x:%02x:%02x:%02x:%02x",
      discovery->addr.val[5],
      discovery->addr.val[4],
      discovery->addr.val[3],
      discovery->addr.val[2],
      discovery->addr.val[1],
      discovery->addr.val[0]
    );
  }

  if (fields.name != NULL)
    snprintf(
      found_keyboard->label,
      sizeof found_keyboard->label,
      "%.*s",
      fields.name_length,
      fields.name
    );
}

static int
record_found_keyboard_or_give_scan_completed_semaphore_on_scan_event(
  struct ble_gap_event *event,
  void *
) {

  if (event->type == BLE_GAP_EVENT_DISC)
    record_found_keyboard(&event->disc);
  else if (event->type == BLE_GAP_EVENT_DISC_COMPLETE)
    xSemaphoreGive(s_scan_completed_semaphore);

  return 0;
}

// -----------------------------------------------------------------------------
// Host
// -----------------------------------------------------------------------------

static void
write_bond_file_records_into_store(void) {
  for (int i = 0; i < s_bond_file_record_count; i++) {
    const BondFileRecord *record = &s_bond_file_records[i];

    if (record->object_type == BLE_STORE_OBJ_TYPE_PEER_SEC)
      ble_store_write_peer_sec(&record->security_value);
    else
      ble_store_write_our_sec(&record->security_value);
  }
}

static void
restore_bonds_and_connect_to_keyboard_on_sync(void) {
  write_bond_file_records_into_store();

  if (ble_hs_id_infer_auto(0, &s_own_address_type) != 0)
    return;

  s_is_host_synced = true;

  start_connecting_to_bonded_keyboard();
}

static void
mark_host_unsynced_on_reset(int) {
  s_is_host_synced = false;
}

static void
run_host_task(void *) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static void
read_bond_file(void) {
  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      BLUETOOTH_BOND_PATH,
      resolved_path,
      sizeof resolved_path
    ) < 0
  )
    return;

  FILE *bond_file = fopen(resolved_path, "rb");

  if (bond_file == NULL)
    return;

  s_bond_file_record_count =
    (int)fread(
      s_bond_file_records,
      sizeof s_bond_file_records[0],
      BOND_FILE_RECORD_CAPACITY,
      bond_file
    );

  fclose(bond_file);
}

static int
copy_bond_record_on_iterate(
  int object_type,
  union ble_store_value *store_value,
  void *
) {

  BondFileRecord *record;

  if (s_bond_file_record_count >= BOND_FILE_RECORD_CAPACITY)
    return 1;

  record = &s_bond_file_records[s_bond_file_record_count++];
  record->object_type = (uint8_t)object_type;
  record->security_value = store_value->sec;

  return 0;
}

void
area512_bluetooth_load_config(void) {
  char resolved_path[AREA512_PATH_MAX];
  char line[BLUETOOTH_CONFIG_LINE_BYTE_CAPACITY];

  if (
    area512_resolve_data_path(
      BLUETOOTH_CONFIG_PATH,
      resolved_path,
      sizeof resolved_path
    ) < 0
  )
    return;

  FILE *file = fopen(resolved_path, "r");

  if (file == NULL)
    return;

  while (fgets(line, sizeof line, file) != NULL) {
    line[strcspn(line, "\r\n")] = '\0';

    if (strcmp(line, "ble=on") == 0)
      s_is_enabled_in_config = true;
    else if (strcmp(line, "ble=off") == 0)
      s_is_enabled_in_config = false;
  }

  fclose(file);
}

void
area512_bluetooth_start_if_enabled(void) {
  if (!s_is_enabled_in_config)
    return;

  s_scan_completed_semaphore = xSemaphoreCreateBinary();
  s_keyboard_report_queue =
    xQueueCreate(KEYBOARD_REPORT_QUEUE_LENGTH, KEYBOARD_REPORT_BYTE_SIZE);

  if (s_scan_completed_semaphore == NULL || s_keyboard_report_queue == NULL)
    return;

  if (nimble_port_init() != ESP_OK)
    return;

  ble_hs_cfg.sync_cb = restore_bonds_and_connect_to_keyboard_on_sync;
  ble_hs_cfg.reset_cb = mark_host_unsynced_on_reset;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
  ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
  ble_hs_cfg.sm_bonding = 1;
  ble_hs_cfg.sm_mitm = 1;
  ble_hs_cfg.sm_sc = 1;
  ble_hs_cfg.sm_our_key_dist =
    BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.sm_their_key_dist =
    BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

  ble_store_config_init();
  read_bond_file();
  nimble_port_freertos_init(run_host_task);
}

bool
area512_bluetooth_save_bonds(void) {
  char resolved_path[AREA512_PATH_MAX];
  bool is_saved;

  if (
    area512_resolve_data_path(
      BLUETOOTH_BOND_PATH,
      resolved_path,
      sizeof resolved_path
    ) < 0
  )
    return false;

  if (area512_sd_mount("/sdcard") != 0)
    return false;

  FILE *bond_file = fopen(resolved_path, "wb");

  if (bond_file == NULL) {
    area512_sd_unmount();

    return false;
  }

  s_bond_file_record_count = 0;

  ble_store_iterate(
    BLE_STORE_OBJ_TYPE_OUR_SEC,
    copy_bond_record_on_iterate,
    NULL
  );
  ble_store_iterate(
    BLE_STORE_OBJ_TYPE_PEER_SEC,
    copy_bond_record_on_iterate,
    NULL
  );

  is_saved =
    fwrite(
      s_bond_file_records,
      sizeof s_bond_file_records[0],
      s_bond_file_record_count,
      bond_file
    ) == (size_t)s_bond_file_record_count;

  if (fclose(bond_file) != 0)
    is_saved = false;

  area512_sd_unmount();

  return is_saved;
}

// -----------------------------------------------------------------------------
// Pairing
// -----------------------------------------------------------------------------

bool
area512_bluetooth_is_running(void) {
  return s_is_host_synced;
}

bool
area512_bluetooth_is_keyboard_connected(void) {
  return s_connection_handle != BLE_HS_CONN_HANDLE_NONE;
}

int
area512_bluetooth_scan_keyboards(void) {
  struct ble_gap_disc_params scan_parameters = {};

  s_pairing_state = AREA512_BLUETOOTH_PAIRING_RUNNING;
  s_pairing_failure_message[0] = '\0';
  s_found_keyboard_count = 0;

  ble_gap_conn_cancel();

  while (ble_gap_conn_active())
    vTaskDelay(pdMS_TO_TICKS(CONNECT_CANCEL_POLL_INTERVAL_MILLISECONDS));

  scan_parameters.filter_duplicates = 1;

  if (
    ble_gap_disc(
      s_own_address_type,
      SCAN_DURATION_MILLISECONDS,
      &scan_parameters,
      record_found_keyboard_or_give_scan_completed_semaphore_on_scan_event,
      NULL
    ) != 0
  )
    return -1;

  xSemaphoreTake(s_scan_completed_semaphore, portMAX_DELAY);

  return s_found_keyboard_count;
}

const char *
area512_bluetooth_found_keyboard_label(int found_keyboard_index) {
  return s_found_keyboards[found_keyboard_index].label;
}

void
area512_bluetooth_start_pairing(int found_keyboard_index) {
  const ble_addr_t *address = &s_found_keyboards[found_keyboard_index].address;
  int result;

  ble_store_util_delete_peer(address);

  result =
    ble_gap_connect(
      s_own_address_type,
      address,
      PAIRING_CONNECT_TIMEOUT_MILLISECONDS,
      NULL,
      dispatch_connection_event,
      NULL
    );

  if (result == 0)
    return;

  record_pairing_failure("connect", result);
  s_pairing_state = AREA512_BLUETOOTH_PAIRING_FAILED;

  start_connecting_to_bonded_keyboard();
}

void
area512_bluetooth_cancel_pairing(void) {
  s_pairing_state = AREA512_BLUETOOTH_PAIRING_INACTIVE;

  if (s_connection_handle != BLE_HS_CONN_HANDLE_NONE) {
    disconnect_keyboard();

    return;
  }

  if (ble_gap_conn_cancel() == 0)
    return;

  start_connecting_to_bonded_keyboard();
}

int
area512_bluetooth_pairing_state(void) {
  return s_pairing_state;
}

uint32_t
area512_bluetooth_pairing_passkey(void) {
  return s_pairing_passkey;
}

const char *
area512_bluetooth_pairing_failure_message(void) {
  return s_pairing_failure_message;
}

// -----------------------------------------------------------------------------
// Key input
// -----------------------------------------------------------------------------

static const char *
lookup_arrow_key_sequence(uint8_t usage) {
  switch (usage) {
  case RIGHT_ARROW_USAGE:
    return "\x1b[C";
  case LEFT_ARROW_USAGE:
    return "\x1b[D";
  case DOWN_ARROW_USAGE:
    return "\x1b[B";
  case UP_ARROW_USAGE:
    return "\x1b[A";
  default:
    return NULL;
  }
}

static char
lookup_key_character(uint8_t usage, bool is_shift_pressed) {
  if (usage < A_USAGE || usage > SLASH_USAGE)
    return '\0';

  if (is_shift_pressed)
    return SHIFTED_CHARACTERS[usage - A_USAGE];

  return UNSHIFTED_CHARACTERS[usage - A_USAGE];
}

static void
push_key_to_console(uint8_t usage, uint8_t modifier_bits) {
  bool is_ctrl_pressed = (modifier_bits & CTRL_MODIFIER_BITS) != 0;
  bool is_shift_pressed = (modifier_bits & SHIFT_MODIFIER_BITS) != 0;
  bool is_alt_pressed = (modifier_bits & ALT_MODIFIER_BITS) != 0;
  const char *arrow_key_sequence = lookup_arrow_key_sequence(usage);

  if (arrow_key_sequence != NULL) {
    area512_console_flush_ime();
    area512_console_stdin_push_bytes(arrow_key_sequence);

    return;
  }

  switch (usage) {
  case ENTER_USAGE:
  case KEYPAD_ENTER_USAGE:
    area512_console_flush_ime();
    area512_console_stdin_push('\r');

    return;

  case ESCAPE_USAGE:
    area512_console_flush_ime();
    area512_console_stdin_push(ASCII_ESCAPE);

    return;

  case BACKSPACE_USAGE:
    area512_console_feed_ime(ASCII_BACKSPACE);

    return;

  case TAB_USAGE:
    area512_console_feed_ime('\t');

    return;

  case SPACE_USAGE:
    if (is_ctrl_pressed)
      area512_console_toggle_ime();
    else
      area512_console_feed_ime(' ');

    return;

  default:
    break;
  }

  char character = lookup_key_character(usage, is_shift_pressed);

  if (character == '\0')
    return;

  if (is_ctrl_pressed && usage >= A_USAGE && usage <= Z_USAGE) {
    area512_console_flush_ime();
    area512_console_stdin_push((uint8_t)(character & 0x1F));

    return;
  }

  if (is_alt_pressed) {
    area512_console_flush_ime();
    area512_console_stdin_push(ASCII_ESCAPE);
    area512_console_stdin_push((uint8_t)character);

    return;
  }

  area512_console_feed_ime((uint8_t)character);
}

static bool
has_key_usage(const uint8_t *keyboard_report, uint8_t usage) {
  for (
    int i = KEYBOARD_REPORT_FIRST_KEY_INDEX; i < KEYBOARD_REPORT_BYTE_SIZE; i++
  ) {

    if (keyboard_report[i] == usage)
      return true;
  }

  return false;
}

static void
push_newly_pressed_keys_to_console(const uint8_t *keyboard_report) {
  if (
    keyboard_report[KEYBOARD_REPORT_FIRST_KEY_INDEX] == ERROR_ROLL_OVER_USAGE
  )
    return;

  s_held_modifier_bits = keyboard_report[0];

  for (
    int i = KEYBOARD_REPORT_FIRST_KEY_INDEX; i < KEYBOARD_REPORT_BYTE_SIZE; i++
  ) {

    uint8_t usage = keyboard_report[i];

    if (usage < A_USAGE || has_key_usage(s_previous_keyboard_report, usage))
      continue;

    push_key_to_console(usage, s_held_modifier_bits);

    s_repeat_usage = usage;
    s_repeat_next_tick =
      xTaskGetTickCount() + pdMS_TO_TICKS(REPEAT_DELAY_MILLISECONDS);
  }

  if (!has_key_usage(keyboard_report, s_repeat_usage))
    s_repeat_usage = 0;

  memcpy(
    s_previous_keyboard_report,
    keyboard_report,
    KEYBOARD_REPORT_BYTE_SIZE
  );
}

static void
repeat_held_key(void) {
  TickType_t now = xTaskGetTickCount();

  if (s_repeat_usage == 0 || (int32_t)(now - s_repeat_next_tick) < 0)
    return;

  push_key_to_console(s_repeat_usage, s_held_modifier_bits);

  s_repeat_next_tick = now + pdMS_TO_TICKS(REPEAT_INTERVAL_MILLISECONDS);
}

void
area512_bluetooth_poll_keyboard(void) {
  uint8_t keyboard_report[KEYBOARD_REPORT_BYTE_SIZE];

  if (s_keyboard_report_queue == NULL)
    return;

  while (
    xQueueReceive(s_keyboard_report_queue, keyboard_report, 0) == pdTRUE
  )
    push_newly_pressed_keys_to_console(keyboard_report);

  repeat_held_key();
}
