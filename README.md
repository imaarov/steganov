# steganov
Hide your data inside other data

**steganov** is an educational steganography project written entirely in C. It demonstrates how data can be hidden inside common media formats using the Least Significant Bit (LSB) technique while also exploring the internal structure of those formats.

Unlike many similar projects, this repository intentionally avoids thrid party libraries for the implemented algorithm. Componenets such as CRC32, Adler32, Bitstream handling, Huffman coding, LZ77 and the DEFLATE pipeline (with so many goddamn RFC 1951 specifics ): ) aree implemented from scratch for learning purposes.

> [!IMPORTANT]
> **This is an educational project, not a production-ready steganography library.**

> The primary goal is to understand how file formats, compression algorithms, and steganographic techniques work internally. The code is written to maximize readability and learning rather than to provide an enterprise-ready, security-hardened implementation.

## Documentation

The repository contains a dedicated **[docs](/docs)** directory that explains the concepts behind the implementation and documents the algorithms used throughour the project.

Documentation currently includes topics such as:

* Steganography fundamentals
* LSB encoding and decoding
* File structure ("blueprints")
* PNG chunk anatomy
* CRC32
* Adler32
* Bitstream implementation
* Huffman coding
* LZ77
* DEFLATE pipline
* RFC 1951 Specifications

If you are interested in understanding *how* the implementation works rather than simply using it, start with:
```test
docs/README.md
```

and then continue with the individual documents inside the `docs/` directory.

## Commands
build:
```bash
make
```
format:
```bash
make format
```
clean obj files:
```bash
make clean
```
build tests:
```bash
make test
```
run test
```bash
make run-test
# or
./bin/test_runner
```

Use:
```bash
./bin/steganov
steganov v1.0.0
select the operation:
1-encode data
2-decode data
3-exit
2
enter the file name where data should encode/decode there:
test.png
[DEBUG]: src/png.c:437:Header information:
[DEBUG]: src/png.c:450:signature first 4 byte : 474e5089
[DEBUG]: src/png.c:451:signature second 4 byte : a1a0a0d
[DEBUG]: src/png.c:453:png file signature is standard
[DEBUG]: src/png.c:478:IHDR chunk size: 13
[DEBUG]: src/png.c:479:IHDR ID: IHDR
[DEBUG]: src/png.c:480:IHDR width: 1280
[DEBUG]: src/png.c:482:IHDR height: 853
[DEBUG]: src/png.c:484:IHDR color type : 2
[DEBUG]: src/png.c:485:IHDR compression type: 0
[DEBUG]: src/png.c:486:IHDR filter type: 0
[DEBUG]: src/png.c:487:IHDR interlace method: 0
[DEBUG]: src/png.c:499:CRC CHECKED
[DEBUG]: src/decode.c:58:INFO: Successful decoding operation

INFO: secret message extracted from file is: fucktheworld

```

decode and test the expected string for encoded deflate string
```bash
./tools/validate_deflate.py
```

### Supported media
- BMP
- WAV
- PNG
