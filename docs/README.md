# Documentation on how Steganov works, plus all algorithms used and written from scratch in this project.
### Steganography

**Steganography** is a technique that helps you hide your data (including strings of text, media files, zip archives, etc.) inside other data.

- Steganography
    - [LSB method](LSB-method)
- Carrier files
    - [file blueprint](file-structure-or-blueprint)
    - [PNG blueprint](png-file-blueprint)
    - [PNG Chunk anatomy](chunk-anatomy)
- Data
    - [Data itself](data-itself)
    - [PNG Data](PNG-data)
        - [CRC checksum](CRC-checksum)
        - [LZ77](LZ77)
        - [Huffman](Huffman)
        - [Bitstream](Bitstream)
        - [Adler32](Adler32)


### LSB method
If we consdier a media file (such as an image, audio, or video) as the carrier, there are many ways to embed the data you intend to hide. LSB (Least Significant Bit) is one of the most common methods.

LSB (Least Significant Bit) is a Steganographic method where we manipulate only the LSB part of the byte/bytes of a media sample (for example, PCM audio samples or image pixels).

In this way, the changes to the overall media graphic or audio are minimal, andd a person should not be able to find out that the media has been manipulated, except in cases wehre the person has the hash and **checksum** of the file before we embedded data into it.

We have other methods as well for embedding data in digital files. However, methods like LSB are fairly easy when you move out to the other parts of this project.

### File structure or Blueprint

Every media file or files that have a standard extension also have a standard file structure. I call them "file blueprint" because they are super similar to a blueprint, and you'll see why.

Most file structure and bluepritns start with a "File Signature", a 2 byte, 4 byte, or 8 byte piece of data represented in documents as hexadecimal numbers. We use this to check the file signature or true extension, though it is required but not enough to prove the file is standard or not corrupt.

After that blueprints vary based on the file extension. Many file extensions and media extensions are chunk based, while many others store the raw data offset, and you just have to find it and jump to that offset. For example the PNG bluerpint is structured similary to the following:

### PNG file blueprint

```text
+-----------------------------+
| PNG Signature (8 bytes)     |
+-----------------------------+
| Length (4)                  |
+-----------------------------+
| Chunk Type (4)              |
+-----------------------------+
| Chunk Data (Length bytes)   |
+-----------------------------+
| CRC (4)                     |
+-----------------------------+
| Length                      |
+-----------------------------+
| Chunk Type                  |
+-----------------------------+
| Chunk Data                  |
+-----------------------------+
| CRC                         |
+-----------------------------+
|             ...             |
+-----------------------------+
| IEND Chunk                  |
+-----------------------------+
```

As you can see, this repeating pattern is called a "chunk". Each chunk has its own blueprint and anatomy.

## Chunk Anatomy

Every chunk in PNG follows this exact structure:

| Field      | Size (bytes) | Description                                        |
|------------|--------------|----------------------------------------------------|
| Length     | 4            | Length of the Data field  |
| Type       | 4            | 4 ASCII characters (e.g., `IHDR`, `IDAT`, `IEND`) |
| Data       | Variable     | Actual chunk data (size = Length)                  |
| CRC        | 4            | Checksum of Type + Data fields                     |

So we, or any program that acts as a decoder such as image viewers or media players are essentially blind. They must be guided by the essential chunks. If these chunks are missing or misplaced we should close the file and call it corrupt.

The same holds true for the inner structure of each chunk. If  we don't have the length of a chunk, we don't know how much data we should read. Likewise , if the CRC we calculate does not match the CRC embedded at the end of the chunk, we should also call it corrupt.

### Data itself
But the data itself is a whole new chapter. In PNG we have the DEFLATE algorithm which uses Huffman coding, LZ77 and Adler32(checksum). After that you may not get the raw piexels if they are effected by filters. These algorithms are all implemented and put into a library called zlib. However in this project I don't use any third party libraries, i build and implemented them myself from scratch.


### PNG Data

