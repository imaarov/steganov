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
decode and test the expected string for encoded deflate string
```bash
./tools/validate_deflate.py
```

### Supported media
- BMP
- WAV
- PNG
