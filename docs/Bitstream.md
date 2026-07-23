# Bitstream Explanation

A bitstream is a sequence of individual bits that can be written or read one at a time instead of working with whole bytes. While most programs manipulate data in 8-bit chunks, many compression algorithms operate on values whose lengths are not multiples of eight. For example, a Huffman code may be 3 bits long, another may be 11 bits long, and another may be 15 bits long.

Since standard memory is byte-addressable, we need a way to pack arbitrary numbers of bits into a continuous byte buffer. That is exactly what this bitstream implementation provides.

In **DEFLATE**, nearly everything is written at the bit level:

- Block headers
- Huffman codes
- Extra bits for lengths and distances
- End-of-block markers

Without a bitstream abstraction, writing a DEFLATE encoder or decoder becomes extremely difficult because every field would have to manually manage bit offsets inside bytes.

The source file is located in `src/bitstream.c`.

## Header file

```C
#define BITSTREAM_INITIAL_CAPACITY 1024

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t byte_pos;
    uint8_t bit_pos;
    bool owns_data;
} bitstream_t;
```

The `bitstream_t` structure stores all the information required to read or write bits.

- **data** points to the underlying byte buffer.
- **capacity** is the total allocated size of the buffer. For read-only streams, it represents the size of the input buffer.
- **byte_pos** keeps track of the current byte.
- **bit_pos** keeps track of the current bit inside that byte (0-7).
- **owns_data** tells the implementation whether the buffer was allocated internally or supplied by the caller. This prevents accidentally freeing or resizing memory that belongs to someone else.

The stream starts with an initial capacity of **1024 bytes** and automatically grows whenever additional space is required.

---

# Initializing a stream

```C
void bitstream_init(bitstream_t *stream);
```

When creating a stream for writing, memory is allocated dynamically and the entire buffer is initialized to zero.

```C
stream->capacity = BITSTREAM_INITIAL_CAPACITY;
stream->data = malloc(stream->capacity);
memset(stream->data, 0, stream->capacity);
```

Zeroing the buffer is important because when writing bits, the implementation only sets bits whose value is **1**. Bits that should remain **0** are already cleared, so there is no need to explicitly write zeros.

For reading, no allocation is necessary.

```C
void bitstream_init_from_buffer(bitstream_t *stream,
                                const byte *data,
                                size_t size);
```

Instead, the stream simply points to an existing buffer and marks it as read-only.

---

# Writing bits

The implementation provides two writing functions.

```C
bitstream_write_bits_lsb(...)
```

and

```C
bitstream_write_bits_msb(...)
```

The difference is the order in which bits are taken from the value.

### LSB-first writing

```C
for (uint8_t i = 0; i < count; i++)
```

The loop begins with bit zero of the value and progresses toward the most significant bit.

For every bit, the implementation checks whether it is equal to one.

```C
if (value & (1u << i))
```

If the bit is set, the corresponding position inside the output buffer is also set.

```C
stream->data[stream->byte_pos] |=
        (uint8_t)(1u << stream->bit_pos);
```

Notice that only bits equal to **1** are written. Since the buffer was initialized with zeros, writing zero bits is unnecessary.

After every bit, the current position advances.

```C
if (stream->bit_pos == 7)
```

Once the eighth bit of a byte has been filled, writing continues at the beginning of the next byte.

This bit order matches the DEFLATE specification, where most fields are stored least-significant bit first.

---

### MSB-first writing

Some values in compression formats are written in the opposite direction.

```C
for (int i = count - 1; i >= 0; i--)
```

Instead of beginning with the least significant bit, writing starts from the most significant bit and moves downward.

Apart from the bit order, the implementation is identical.

---

# Reading bits

Reading is the reverse operation.

```C
int bitstream_read_bit(...)
```

The current bit is extracted using a shift followed by a mask.

```C
int bit =
(stream->data[stream->byte_pos] >> stream->bit_pos) & 1;
```

After reading, the bit position advances exactly the same way it does while writing.

Reading multiple bits is simply a repeated call to `bitstream_read_bit()`.

```C
bitstream_read_bits_lsb(...)
```

Each bit is placed back into the resulting integer using

```C
val |= ((uint32_t)b) << i;
```

which reconstructs the original value one bit at a time.

---

# Byte alignment

Sometimes compression formats require the next field to begin on a byte boundary.

```C
void bitstream_align_byte(...)
```

If the current position is somewhere in the middle of a byte, the remaining unused bits are skipped and reading or writing continues from the next byte.

---

# Flushing

When writing finishes, the last byte may not be completely full.

```C
void bitstream_flush(...)
```

If there are unwritten bits remaining in the current byte, the function advances to the next byte so that the partially written byte becomes part of the final output.

Without flushing, the final byte would not be counted as valid output.

---

# Growing the buffer

A compressed stream can easily become larger than the initial allocation.

Whenever the current byte reaches the end of the buffer, the stream automatically expands itself.

```C
size_t new_capacity = stream->capacity * 2;
```

The implementation doubles the capacity using `realloc()`.

After resizing, the newly allocated memory is cleared with `memset()` so that unwritten bits remain zero.

This strategy keeps memory allocations relatively infrequent while allowing the stream to grow to accommodate arbitrarily large outputs.

---

# Why are both `byte_pos` and `bit_pos` needed?

A single integer is not enough to represent the current position because writing happens one bit at a time.

For example,

```
byte 0:
10101101
     ^
```

The stream needs to know both

- which byte it is currently accessing, and
- which bit inside that byte is being read or written.

`byte_pos` identifies the byte, while `bit_pos` identifies the position within that byte.

Together they allow the stream to move seamlessly across byte boundaries without the caller having to manage individual bits manually.

---

This bitstream implementation is intentionally generic. Although it was written for the DEFLATE encoder and decoder used in this project, it is independent of the compression algorithm itself and can be reused anywhere data must be processed at the bit level. Separating bit manipulation into its own module also keeps the compression code considerably simpler, since higher-level components only need to request that bits be written or read without worrying about how those bits are packed into memory.
