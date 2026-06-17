# steganov
Hide your data inside other data

steganov is a tool that hides your data (currently, data is only a string) inside other data (mainly files like images and audio), a technique called steganography.
The basic form of doing steganography is manipulating the LSB (Least Significant Bit) and the LSBs we manipulate should not be the header section or in reserved sections of the file. 

Most of the time standard files have standard blueprint, which contains the boundaries of each section for example: header, file size, reserved, actual data (for example pixels data).

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

As i move to another file type, which is WAV, i find out that the above statement:
> In other words, we should have one byte of file data for each 1 bit ...

assumes something important, that each part of the data consists of one byte. 

This is not true for all file types, as I found when I moved to WAV files.
See, WAV files store samples, each sample (usually PCM) is 2 bytes, not one byte, that means a sample looks like this:
```c
unsigned char sample[2]; 
```
In a file, it looks like this:
```
0xd8 0xff
```
or
```
d8ff
```
And as you should know by now, CPU interprets these as little-endian, so we should not manipulate both bytes. We should only manipulate the least significant bit of the byte that is the least significant.

In the above example, the data is in little-endian format, which means that `sample[0]`is the byte that we should manipulate. The sample is actually interpreted like this:
```
0xff 0xd8
```

Based on that we should only manipulate the LSB of the `oxd8` byte.
And all of that is also an assumption, in a WAV file , the header directly tells you the `BitsPerSample` value so you know what to do for LSB encoding and decoding.


### Supported File Types:
- BMP
- WAV


## TODO
- [x] Get started with one file type (starting with BMP)
- [x] Read the file size and header, and verify them
- [x] Implement the basic steganography method (LSB) and complete the encoding process
- [x] Implement decoding from a file
- [x] Implement CRC(Cyclic Redundancy Codes) Algorithm
- [x] Implement Adler32 algorithm
- [ ] Implement bitstream
- [ ] Implement Huffman algorithm
- [x] Implement LZ77 algorithm
- [ ] Implement DEFLATE decompression/compression
- [ ] Implement Knuth Morris Pratt algorithm
- [ ] Come up with another algorithm besides LSB
- [ ] Implement encoding/decoding for PNG
- [ ] Implement the Huffman algorithm for encoding/decoding, and provide the user with the option to select from the available algorithms
- [ ] Implement a recovery mode to restore the file contents if an operation fails
