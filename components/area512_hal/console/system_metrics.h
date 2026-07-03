#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int area512_metrics_battery_percent(void);
int area512_metrics_vm_used_pct(void);
int area512_metrics_dram_used_pct(void);
int area512_metrics_stack_used_pct(void);

#ifdef __cplusplus
}
#endif
