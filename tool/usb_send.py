#!/usr/bin/env python3
"""Send a file or directory to a Cardputer waiting in AREA512 recv mode."""

import argparse
from pathlib import Path, PurePosixPath
import time
import zlib

import serial
from serial.tools import list_ports


ESPRESSIF_VENDOR_ID = 0x303A
REPLY_PREFIX = "AREA512 "
READY_REPLY_LINE = REPLY_PREFIX + "READY"
CHUNK_BYTE_COUNT = 4096
REPLY_TIMEOUT_SECONDS = 5


class TransferError(Exception):
    pass


def find_port():
    for port_info in list_ports.comports():
        if port_info.vid == ESPRESSIF_VENDOR_ID:
            return port_info.device

    raise TransferError("no Espressif USB port found")


def open_port(port_path):
    serial_port = serial.Serial()
    serial_port.port = port_path
    serial_port.timeout = REPLY_TIMEOUT_SECONDS
    serial_port.open()

    return serial_port


def wait_for_ready(serial_port):
    deadline = time.monotonic() + REPLY_TIMEOUT_SECONDS

    serial_port.reset_input_buffer()

    while time.monotonic() < deadline:
        line_bytes = serial_port.readline()
        line_text = line_bytes.decode("utf-8", "replace").rstrip("\r\n")

        if line_text == READY_REPLY_LINE:
            return

    raise TransferError("device is not in receive mode")


def read_reply(serial_port):
    deadline = time.monotonic() + REPLY_TIMEOUT_SECONDS

    while time.monotonic() < deadline:
        line_bytes = serial_port.readline()
        line_text = line_bytes.decode("utf-8", "replace").rstrip("\r\n")

        if line_text.startswith(REPLY_PREFIX) and line_text != READY_REPLY_LINE:
            return line_text[len(REPLY_PREFIX):]

    return None


def expect_ok(serial_port, device_path):
    reply_text = read_reply(serial_port)

    if reply_text is None:
        raise TransferError(f"{device_path} NG no reply from device")

    if reply_text != "OK":
        raise TransferError(f"{device_path} {reply_text}")


def send_directory(serial_port, device_path):
    serial_port.write(f"MKDIR {device_path}\n".encode())
    expect_ok(serial_port, device_path)


def send_file(serial_port, local_path, device_path):
    file_bytes = local_path.read_bytes()
    file_crc32 = zlib.crc32(file_bytes)

    serial_port.write(
        f"FILE {device_path} {len(file_bytes)} {file_crc32:08x}\n".encode()
    )
    expect_ok(serial_port, device_path)

    for chunk_offset in range(0, len(file_bytes), CHUNK_BYTE_COUNT):
        chunk_bytes = file_bytes[chunk_offset:chunk_offset + CHUNK_BYTE_COUNT]

        serial_port.write(f"DATA {len(chunk_bytes)}\n".encode() + chunk_bytes)
        expect_ok(serial_port, device_path)

    serial_port.write(b"END\n")
    expect_ok(serial_port, device_path)


def send_done(serial_port):
    serial_port.write(b"DONE\n")
    expect_ok(serial_port, "DONE")


def send_tree(serial_port, source_path, destination_directory):
    device_path = str(PurePosixPath(destination_directory) / source_path.name)

    if source_path.is_dir():
        send_directory(serial_port, device_path)
        print(f"{device_path} OK")

        for child_path in sorted(source_path.iterdir()):
            send_tree(serial_port, child_path, device_path)
    else:
        send_file(serial_port, source_path, device_path)
        print(f"{device_path} OK")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination")
    parser.add_argument("--port")
    args = parser.parse_args()

    try:
        port_path = args.port or find_port()

        with open_port(port_path) as serial_port:
            wait_for_ready(serial_port)
            send_tree(serial_port, args.source, args.destination)
            send_done(serial_port)
    except (OSError, serial.SerialException, TransferError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
