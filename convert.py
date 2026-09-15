#!/usr/bin/env python3
"""Convert an image to a 320x240 or 240x135, big-endian, headerless RGB565 background."""

import argparse
from pathlib import Path
import struct

from PIL import Image, ImageOps


SIZES = {
    "320x240": (320, 240),
    "240x135": (240, 135),
}


def convert(source, destination, width, height):
    if source.resolve() == destination.resolve():
        raise ValueError("input and output must be different files")

    with Image.open(source) as original:
        image = ImageOps.exif_transpose(original).convert("RGBA")
        background = Image.new("RGBA", image.size, (0, 0, 0, 255))
        image = Image.alpha_composite(background, image).convert("RGB")
        image = image.resize((width, height), Image.Resampling.LANCZOS)

    pixels = bytearray(width * height * 2)
    for index, (red, green, blue) in enumerate(image.getdata()):
        color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
        struct.pack_into(">H", pixels, index * 2, color)

    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(pixels)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--size", choices=SIZES.keys(), default="320x240")
    args = parser.parse_args()
    width, height = SIZES[args.size]
    try:
        convert(args.input, args.output, width, height)
    except (OSError, ValueError) as error:
        parser.exit(1, f"convert: {error}\n")
    print(f"{args.output}: {width}x{height} RGB565, {width * height * 2} bytes")


if __name__ == "__main__":
    main()
