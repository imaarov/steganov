# TODOs

## TODO BMP (DONE)
- [x] Get started with one file type (starting with BMP)
- [x] Read the file size and header, and verify them
- [x] Implement the basic steganography method (LSB) and complete the encoding process
- [x] Implement decoding from a file (without output file)

## TODO WAV (DONE)
- [x] Read the file size and header, and verify them
- [x] Implement the basic steganography proper for two byte samples, method (LSB) and complete the encoding process
- [x] Implement decoding from a file (without output file)

## TODO PNG
- [x] Implement CRC(Cyclic Redundancy Codes) Algorithm
- [x] Implement Adler32 algorithm
- [x] Implement bitstream
- [x] Implement Huffman algorithm
- [x] Implement canonical huffman algorithm
- [x] Implement LZ77 algorithm
- [x] Make basic LZ77 algorithm compatible with DEFLATE 
- [x] Complete Huffman and finally generate canonical huffman table length
- [x] Implement hashing for LZ77 find match function to improve the performance
- [x] Add Bitstream for encoding/decoding
- [x] Add endians type for bitstream
- [x] Create deflate module for connecting Huffman/LZ77/Bitstream together to make the final DEFLATE pipeline
- [x] Use ALL of the RFC 1951 specifications (header bits, way of embedding, HDIST, HCLEN, huffman tree embedding and ...)
- [x] Add ZLIB WRAPPER to mimic what zlib api does
- [x] Implement Min heap priority for huffman algorithm
- [x] Implement encoding/decoding for PNG
- [ ] FIX src/deflate.c:122:huffman code lengths > 15 detected, falling back to no compression
- [ ] Implement the unfilter step to make changes on raw pixels not filtered PIXELS!!!!!
- [ ] Implement Knuth Morris Pratt algorithm

## TODO Steganov
- [ ] Add custom print function for this project
- [ ] Come up with another algorithm besides LSB
- [ ] Implement a recovery mode to restore the file contents if an operation fails
