#!/usr/bin/env python3
"""Compare two binary PPM/P6 images and report RGB differences."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


def read_token(data: bytes, offset: int) -> tuple[bytes, int]:
    size = len(data)
    while offset < size and data[offset:offset + 1].isspace():
        offset += 1
    if offset < size and data[offset:offset + 1] == b"#":
        while offset < size and data[offset:offset + 1] not in {b"\n", b"\r"}:
            offset += 1
        return read_token(data, offset)

    start = offset
    while offset < size and not data[offset:offset + 1].isspace():
        offset += 1
    if start == offset:
        raise ValueError("Unexpected end of PPM header")
    return data[start:offset], offset


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    offset = 0
    magic, offset = read_token(data, offset)
    if magic != b"P6":
        raise ValueError(f"{path}: expected binary PPM/P6, got {magic!r}")
    width_token, offset = read_token(data, offset)
    height_token, offset = read_token(data, offset)
    maxval_token, offset = read_token(data, offset)
    width = int(width_token)
    height = int(height_token)
    maxval = int(maxval_token)
    if width <= 0 or height <= 0:
        raise ValueError(f"{path}: invalid size {width}x{height}")
    if maxval != 255:
        raise ValueError(f"{path}: unsupported max value {maxval}, expected 255")

    if offset >= len(data) or not data[offset:offset + 1].isspace():
        raise ValueError(f"{path}: missing PPM header terminator")
    offset += 1
    pixels = data[offset:]
    expected = width * height * 3
    if len(pixels) != expected:
        raise ValueError(
            f"{path}: expected {expected} RGB bytes, found {len(pixels)}"
        )
    return width, height, pixels


def compare_ppm(reference: Path, actual: Path) -> tuple[float, int, int, tuple[float, float, float]]:
    ref_width, ref_height, ref_pixels = read_ppm(reference)
    actual_width, actual_height, actual_pixels = read_ppm(actual)
    if (ref_width, ref_height) != (actual_width, actual_height):
        raise ValueError(
            "Image sizes differ: "
            f"{reference} is {ref_width}x{ref_height}, "
            f"{actual} is {actual_width}x{actual_height}"
        )

    total = 0
    channel_totals = [0, 0, 0]
    max_diff = 0
    changed = 0
    for index, (ref, got) in enumerate(zip(ref_pixels, actual_pixels)):
        diff = abs(ref - got)
        total += diff
        channel_totals[index % 3] += diff
        max_diff = max(max_diff, diff)
        if diff:
            changed += 1
    mean = total / max(1, len(ref_pixels))
    pixels = max(1, len(ref_pixels) // 3)
    channel_means = (
        channel_totals[0] / pixels,
        channel_totals[1] / pixels,
        channel_totals[2] / pixels,
    )
    return mean, max_diff, changed, channel_means


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("reference", type=Path)
    parser.add_argument("actual", type=Path)
    parser.add_argument("--max-mean", type=float, default=3.0)
    parser.add_argument("--max-pixel", type=int, default=32)
    args = parser.parse_args()

    try:
        mean, max_diff, changed, channel_means = compare_ppm(args.reference, args.actual)
    except Exception as exc:
        print(f"PPM compare ERROR: {exc}", file=sys.stderr)
        return 2

    print(
        "PPM compare: "
        f"mean abs diff {mean:.4f}, max channel diff {max_diff}, "
        f"changed channels {changed}, "
        f"channel means R {channel_means[0]:.4f} "
        f"G {channel_means[1]:.4f} B {channel_means[2]:.4f}"
    )
    if mean > args.max_mean or max_diff > args.max_pixel:
        print(
            "PPM compare FAIL: "
            f"limits mean <= {args.max_mean}, max <= {args.max_pixel}",
            file=sys.stderr,
        )
        return 1
    print("PPM compare PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
