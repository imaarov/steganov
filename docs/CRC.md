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

Notice something? The `x^8` term does not appear in the binary or hex representation, in fact, we omit it in the actual code implementation too. But to grasp the idea and fully map it in your head, you should *not* omit it the way we do in the implementation. The leading 'x^8` bit is always implicitly `1` for an 8-bit CRC polynomial; it's left out of the stored value because it would otherwise overflow the register.


(a few things i should cover later)
worked example
why we shift before checking 
why we shift before xor
why we shift at all
table driven crc (look up table)
reflected/reversed crc variants
inital value and final xor
common poly table
how to verify 
limitations
