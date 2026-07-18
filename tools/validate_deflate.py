#!/usr/bin/env python3
"""
validate raw DEFLATE (RFC 1951) output from a encoder.

Usage:
    python3 validate_deflate.py "05 C0 A1 0D 00 00 00..."
    python3 validate_deflate.py "05C0A10D000000..."
    python3 validate_deflate.py --expect "hellp" "05 C0 A1 ..."

this decompresses your raw deflate stream (no zlib/gzip header,
wbits=-15, matching what BTYPE_NO_COMPRESSION / BTYPE_DYNAMIC_HUFFMAN
produce) and prints the recovered bytes. If --expect is given, it also
checks the result matches exactly.

Note: your bytes will almost never match zlib's own compress() output
byte-for-byte, that's expected. Different encoders make different (but
equally valid) choices about tree shapes and block types. The only thing
that actually needs to hold is: decompress(your_bytes) == original_data.
"""
import sys
import zlib
import argparse


def main():
    parser = argparse.ArgumentParser(description="validate raw DEFLATE hex output")
    parser.add_argument("hexstr", help="hex bytes, spaces optional, quote it")
    parser.add_argument("--expect", help="original data to compare against (as text)")
    parser.add_argument("--expect-hex", help="original data to compare against (as hex)")
    args = parser.parse_args()

    clean_hex = args.hexstr.replace(" ", "").replace("\n", "").strip()
    try:
        data = bytes.fromhex(clean_hex)
    except ValueError as e:
        print(f"ERROR: not valid hex ({e})")
        sys.exit(1)

    print(f"Input: {len(data)} bytes")
    print(f"Hex:   {data.hex(' ')}")
    print()

    try:
        result = zlib.decompress(data, wbits=-15)
    except zlib.error as e:
        print(f"FAILED TO DECOMPRESS: {e}")
        print()
        print("Common causes:")
        print("  - BFINAL/BTYPE bits wrong (check first 3 bits)")
        print("  - HLIT/HDIST/HCLEN miscount")
        print("  - Code-length (CL) Huffman tree incomplete (Kraft sum != 1)")
        print("  - Huffman codes packed in the wrong bit direction (MSB vs LSB)")
        print("  - Missing/extra bits before EOB (256) symbol")
        sys.exit(1)

    print(f"Decompressed OK: {result!r}")
    print(f"Decompressed {len(result)} bytes from {len(data)} compressed bytes")

    expected = None
    if args.expect is not None:
        expected = args.expect.encode()
    elif args.expect_hex is not None:
        expected = bytes.fromhex(args.expect_hex.replace(" ", ""))

    if expected is not None:
        if result == expected:
            print("MATCH: decompressed output equals expected data")
        else:
            print("MISMATCH: decompressed output does NOT equal expected data")
            print(f"  expected: {expected!r}")
            print(f"  got:      {result!r}")
            sys.exit(1)


if __name__ == "__main__":
    main()
