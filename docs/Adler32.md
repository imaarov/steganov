# Adler32 Explanation
Adler-32 is a 32-bit checksum used to verify the integrity of transmitted data. It uses two running sums with a modulo operation based on the largest prime number smaller than 2^16, and combines them to generate the final checksum.

It computes very quickly because of its simplicity. However, that same simplicity also means it is more prone to collisions compared to stronger algorithms. For integrity checks where speed is important, Adler-32 is often a reasonable trade-off.

In PNG's **DEFLATE** compression, more specifically inside the **IDAT** data chunk, two error-detection algorithms are used:
- CRC32
- Adler-32

CRC32 is used for the entire PNG chunk structure, not only for IDAT but for all chunks defined in the PNG format. Adler-32, on the other hand, is primarily used for error detection and checksum verification of the DEFLATE-compressed data stream.

With the use cases out of the way, let's look at the implementation. The code is relatively simple.

The source file is located in ``src/adler32.c``.

## Header file
```C
#define BASE_MODULO 65521
#define INIT_SUM_A  1
#define INIT_SUM_B  0

uint32_t adler32(const char *message, uint32_t message_size);
```
We use BASE_MODULO with the value **65521** because it is the largest prime number smaller than **2^16**. By applying the modulo operation with this value, both running sums remain within the 16-bit range and the probability of collisions is reduced.

**INIT_SUM_A** is initialized to **1** so that the checksum of an empty string is always **1**, which is the standard for Adler-32 behavior.

## Main source code
```C
uint32_t adler32(const char *message, uint32_t message_size) {
    uint32_t sumA = INIT_SUM_A;
    uint32_t sumB = INIT_SUM_B;

    for (int i = 0; i < message_size; i++) {
        sumA += message[i];
        sumB += sumA;

        sumA %= BASE_MODULO;
        sumB %= BASE_MODULO;
    }

    return (sumB << 16) + sumA;
}
```
### Why are two sums used?
The first sum, **sumA**, is simply the running total of all data bytes. It does not track the order of the bytes; it only accumulates their values.

The second sum, **sumB**, is what introduces order sensitivity. If two bytes are swapped, the total byte sum may remain the same, but the sequence of intermediate **sumA** values changes, which causes **sumB** to produce a different result.

For example, two messages can contain the same bytes and have the same overall byte sum, but if the byte order is different, Adler-32 will usually produce a different checksum.

The main goal of Adler-32 is speed. It is significantly faster than CRC32, but that speed comes at the cost of reduced reliability. It is generally weaker than both CRC algorithms and **Fletcher-32** (Adler-32 itself is a modification of the Fletcher checksum), and it should not be relied upon to protect against intentional data modification or malicious tampering.
