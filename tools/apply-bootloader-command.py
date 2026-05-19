#!/usr/bin/env python3
from pathlib import Path
import re
import sys


CMD_CPP = Path("src/cmd.cpp")


def fail(message: str) -> None:
    print(f"apply-bootloader-command: {message}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    if not CMD_CPP.exists():
        fail(f"{CMD_CPP} was not found")

    text = CMD_CPP.read_text(encoding="utf-8")
    original = text

    if '#include "pico/bootrom.h"' not in text:
        if '#include "device/usbd.h"\n' in text:
            text = text.replace(
                '#include "device/usbd.h"\n',
                '#include "device/usbd.h"\n#include "pico/bootrom.h"\n',
                1,
            )
        elif '#include "pico/time.h"\n' in text:
            text = text.replace(
                '#include "pico/time.h"\n',
                '#include "pico/bootrom.h"\n#include "pico/time.h"\n',
                1,
            )
        else:
            fail("could not find a stable include insertion point")

    text = text.replace(
        "// 0x03 reconnect tinyusb device;",
        "// 0x03 reconnect tinyusb device",
    )
    if "// 0x04 enter UF2 bootloader" not in text:
        text = text.replace(
            "// 0x03 reconnect tinyusb device",
            "// 0x03 reconnect tinyusb device\n    // 0x04 enter UF2 bootloader",
            1,
        )

    if "buffer[0] == 0x04" not in text:
        reconnect = re.search(
            r"(?P<block>\n\s*if\s*\(\s*buffer\[0\]\s*==\s*0x03\s*\)\s*\{"
            r".*?tud_disconnect\(\);\s*"
            r".*?sleep_ms\(150\);\s*"
            r".*?tud_connect\(\);\s*"
            r"\n\s*\})",
            text,
            flags=re.DOTALL,
        )
        if reconnect is None:
            fail("could not find the reconnect USB command block")

        bootloader = """
    if (buffer[0] == 0x04) {
        printf("[CMD] Enter UF2 bootloader\\n");
        tud_disconnect();
        sleep_ms(150);
        reset_usb_boot(0, 0);
    }"""
        insert_at = reconnect.end("block")
        text = text[:insert_at] + bootloader + text[insert_at:]

    if text == original:
        print("UF2 bootloader command is already present.")
        return

    CMD_CPP.write_text(text, encoding="utf-8")
    print("Applied UF2 bootloader command patch.")


if __name__ == "__main__":
    main()
