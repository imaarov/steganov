# Cyclic Redundancy Check (CRC) Explanation

CRC is an algorithm for integrity checking of data. It produces a checksum that ensures the transmitted data has no errors. It is more complicated than Adler-32, but better in terms of collision rate, which is relatively lower.


## Math of CRC

In some checksum algorithms, we sum the bytes of the data and label the result a checksum (this is literally where the term "checksum" comes from).
In CRC, however, we do division, to be more precise, binary division.

Do you remember long division from grade school? At each step you look at the leading digits and ask: does the divisor fit here? If yes, you subtract it and bring down the next digit.

CRC does exactly this, but in binary, and subtraction becomes **XOR** (because in GF(2) math, subtraction and addition are both XOR).

Ultimately, we have a CRC register of size 8, 16 or 32 bits, which holds the reminder at each step of the division. At the end, the final reminder is our CRC checksum for that data.

### Properties

- CRC Register
- Polynomial

The **CRC Register** is the register that holds the reminder of each step of our division, computed with the help of the XOR operation.

The **Polynomial** is the divisor used in this calculation. Typically, there are predefined polynomials that mathematicians have determined to be well suited for CRC algorithms, so you don't have to worry about generating a better one yourself (unless you're interested, of course).

The polynomial has some implicit behavior that, if you don't internalize it, will leave you very confused partway through the explanation, trust me. So let's define that implicit behavior.

#### Polynomial Function Display

```
Polynomial (binary)   : 0011 0001
Polynomial (hex)      : 0x31
Polynomial (function) : x^8 + x^5 + x^4 + 1
```

Notice something? The `x^8` term does not appear in the binary or hex representation, in fact, we omit it in the actual code implementation too. But to grasp the idea and fully map it in your head, you should *not* omit it the way we do in the implementation. The leading `x^8` bit is always implicitly `1` for an 8-bit CRC polynomial; it's left out of the stored value because it would otherwise overflow the register.

### Table-Based CRC
In the basic CRC algorithm, we have two loops:

- An outer loop that iterates over each byte of the input data.
- An inner loop that iterates 8 times, once for each bit in the current byte.

For every bit, we perform the required CRC operations (shift, conditional XOR with the polynomial, etc.).

The idea behind a **table-based CRC** is that the work done by the inner loop depends only on the current byte value and the current CRC state. Since a byte can have only **256 possible values (0-255)**, we can **precompute** the result of processing each possible byte.

To build the table

- Loop from **0 to 255**.
- Treat the loop index as a byte.
- Run the normal **8-bit CRC process** on that byte.
- Store the resulting CRC value in a lookup table at the corresponding index.

Then, when processing the actual data:

- Initialize the CRC.
- For each data byte, use the byte (combined with part of the current CRC) as an **index into the table**.
- Update the CRC using the precomputed table value instead of running the inner 8 iteration loop again

This replaces the expensive bit-by0bit processing with a much fastere table lookup.

## Implementation
### CRC-8 (Simple Bit-by-Bit Version)
To implement a simple CRC using an 8-bit register, we can write:
```C
#define DATA_SIZE 3
typedef unsigned char byte;
typedef unsigned short u_int16;

int main() {
    byte input_stream[DATA_SIZE]   = {0x01, 0x02, 0x00};
    byte polynomial                = 0x1D;
    byte crc                       = 0x00;

    for (int i = 0; i < DATA_SIZE; i++) {
        crc ^= input_stream[i];
        for (int b = 0; b < 8; b++) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ polynomial;
            }else {
                crc <<= 1;
            }
        }
    }
    printf("the shift register of the CRC8 is %d\n", crc);
    return 0;
}
```

#### Explanation
**input_stream** contains the data bytes for which we went to compute the CRC.

**polynomial** is the CRC polynomial. Here it is **0x1D**, which is **00011101** in binary.

For an 8-bit CRC, the polynomial is interpreted as:

`` X^8 + x^4 + x^3 + x^2 + 1 ``

The highest-degree term (**x^8**) is omitted when stored in the register, which is why only **0x1D** is stored.

**crc** is the 8-bit shift register that holds the intermediate remainder during the calculation and eventually contains the final CRC value.

#### What the First Loop Does
The outer loop iterates through each byte of input data.
``crc ^= input_stream[i]`` this mixes the next input byte into the CRC register using XOR.

In a bit-by-bit CRC implementation, this is equivalent to loading the next 8 bits int the top part of the register before processing them one bit at a time.

#### What the Second Loop Does
The inner loop runs **8 times**, once for each bit of the current byte.

For each interation:
- Check the **most significant bit (MSB)** of the current CRC register.
- Shift the register left by one bit
- If the old MSB was 1, XOR the register with the polynomial.

#### Core CRC Step
A usefull way to think about it is:
- The shift is always performed.
- The XOR with the polynomial is conditional.

A clearer equivalent implementation is: 
```C
for (int b = 0; b < 8; b++) {
    byte msb = crc & 0x80;      // Save the old MSB

    crc <<= 1;                  // Always shift

    if (msb) {
        crc ^= polynomial;      // Apply polynomial if the old MSB was 1
    }
}
```

### Reflected Implementation
In CRC terminology, **reflected** (also called **reversed**) means that the bits are processed **least significant bit first (LSB-first)** instead of **most significant bit first (MSB-first)**.

Normal (non-reflected) CRC

Bits are processed from left to right, the CRC register shift left, and the top bit is tested.

**Reflected CRC**

The same byte is processed in reverse bit order, the CRC registers shifts **right**, and the low bit is tested (for example crc & 0x01).

#### Why use reflection
Many communication protocols transmit bits **LSB-first**. A reflected CRC allows the software or hardware to process bits in the same order they arrive on the wire.

Common reflected CRCs include:
- CRC-32
- CRC-16/IBM
- CRC-16/ARC

#### Example with CRC-8
```C
for (int b = 0; b < 8; b++) {
    if (crc & 0x01) {
        crc = (crc >> 1) ^ 0xBB;
    } else {
        crc >>= 1;
    }
}
```
#### Why does the polynomial change
when a CRC is reflected, the polynomial bits are also reflected.

For the CRC-8 polynomial:
- Normal     0x1D = 00011101
- Reflected  0xBB = 10111000

### Initial Value and Final XOR
Instead of starting the CRC register at 0x00, we start it with a predefined value.

For example:
- CRC-8 simple      ``0x00``
- CRC-8/MAXIM       ``0x00``
- CRC-8/SAE-J1850   ``0xFF``

#### Why use an initial value
If the CRC starts at **0x00**, leading zero bytes may have less effect on the result. Starting with a nonzero value (often **0xFF**) helps detect certain classes of errors involving leading zeros.

Think of it as **preloading the shift register** before any data is processed.

#### Final XOR (XorOut)
After all data has been processed, we XOR the final CRC with another constant.

#### Why use a final XOR
It prevents some predictable outputs and is part of the definition of many CRC standards. It also helps distinguish different CRC variants that use the same polynomial.

#### Implementation

```C
byte input_stream[DATA_SIZE] = {0x01, 0x02, 0x00};
byte polynomial = 0x1D;
byte crc = 0xFF; // initial value

for (int i = 0; i < DATA_SIZE; i++) {
    crc ^= input_stream[i];
    for (int b = 0; b < 8; b++) {
        byte msb = crc & 0x80;;
        crc <<= 1;
        if (msb) {
            crc ^= polynomial;
        }
    }
}
crc ^= 0xFF; // final XOR
```

(a few things i should cover later)
common poly table
how to verify 
limitations
