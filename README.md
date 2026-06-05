# steganov
Hide your data inside other data

steganov is a tool that hides your data (data is only a string for now) inside other data (mainly a file like images or audio), a technique called steganography.
The basic form of doing steganography is manipulating the LSB (Least Significant Bit) and the LSBs we manipulate should not be the header section or in reserved sections of the file; most of the time standard files have standard blueprint, which contains the boundaries of each section for example: header, file size, reserved, actual data (for example pixels data).

The part we aim to manipulate is the actual data, in this way (LSB way of embedding) it just manipulates the Least Significant Bit, so you really can't see a difference between the original file and the manipulated file and that's of course if we define the act of seeing as a shallow analyze of a file for example see the picture or hear the voice, that's you cant find any differences.

But of course it can change if you use SHA or signature tools to see if the hash of the file is changed or not, that would easily tell you that its changed.

Another part of LSB manipulation is that, by definition, it only manipulates the Least Significant Bit of a byte, and LSB is just a bit located in the Least Significant position
for example:
```c
unsigned char byte = 0b00101001;
```
LSB here is 1 located at the first position of this byte.

So if we aim to embed data in another set of data using LSB, we can embed one bit in the LSB of a byte of target data set like a file or anything.

If we have 1 byte data, then we need (1 * 8) Byte to store that.

```c
char *data = "hi";
unsigned int minimumFileSize = strlen(data) * 8 // file size in bytes;
```

In other words, we should have one byte of file data for each 1 bit of your message or data plan to embed it, the math above its not accurate because we should subtract it by the header size of the file plus the reserved size, because we don't want to corrupt the file, we should not manipulate positions that are not meant to be modified.
### Supported File Types:
- BMP
- WAV


## TODO
- [x] Get started with one file type (starting with BMP)
- [x] Read the file size and header, and verify them
- [x] Implement the basic steganography method (LSB) and complete the encoding process
- [x] Implement decoding from a file
- [ ] Implement the Huffman algorithm for encoding/decoding, and give the user the option to select from the available algorithms
- [ ] Implement a recovery mode to restore the file contents if an operation fails
