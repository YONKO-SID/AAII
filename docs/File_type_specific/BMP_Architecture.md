# BMP Format Architecture & Decoding Specification

## 1. Overview & Implementation Scope

The Windows Bitmap (BMP) format is an uncompressed or run-length encoded raster image format designed around direct byte mapping. While conceptually simple, robust native BMP decoding requires precisely handling little-endian byte ordering, arbitrary header variations across Windows versions, mandatory 4-byte row alignment padding, bottom-up scanline storage, and indexed color palette tables.

To set clear goals, this specification clearly separates:
1. **The Full BMP Format**: A survey of the format's historical variants, header structures, and encoding schemes.
2. **AAII Milestone 1 Contract**: The narrow, strictly validated subset implemented in the initial release.
3. **Subsequent Milestones**: Explicit support rules for 32-bit, indexed, and compressed variants.

---

## 2. Milestone 1 Supported Contract

The initial decoder implementation commits exclusively to the following profile:

| Parameter | Milestone 1 Supported Value | Handling for Other Values |
| :--- | :--- | :--- |
| **Signature** | `0x4D42` (`'B'`, `'M'`) | Reject (`Status::InvalidSignature`) |
| **DIB Header Size** | Exactly `40` bytes (`BITMAPINFOHEADER`) | Reject (`Status::UnsupportedHeaderVersion`) |
| **Planes** | Exactly `1` | Reject (`Status::MalformedHeader`) |
| **Bit Depth** | Exactly `24` bits per pixel | Reject / Defer (`Status::UnsupportedBitDepth`) |
| **Compression** | `0` (`BI_RGB`, uncompressed) | Reject / Defer (`Status::UnsupportedCompression`) |
| **Image Width** | $\text{width} > 0$ and $\le \text{MAX\_DIMENSION}$ | Reject (`Status::InvalidDimensions`) |
| **Image Height** | Signed int32: $\text{height} \neq 0$ and $\text{height} \neq -2^{31}$ | Reject (`Status::InvalidDimensions`) |
| **Orientation** | Positive (bottom-up) and Negative (top-down) | Normalized to canonical top-down output |
| **Wire Pixel Layout**| Packed BGR (`B`, `G`, `R`), 4-byte row aligned | Unpacked & converted to canonical RGB8 |

All other BMP variants (1/4/8-bit indexed, 16-bit, 32-bit direct, RLE compression, `BI_BITFIELDS`, OS/2 headers, V4/V5 color profiles) are explicitly classified as unsupported in Milestone 1 and must be rejected safely during the support-check phase without parsing corruption or unauthorized allocations.

---

## 3. Strict Byte Deserialization (Do Not Cast to C++ Structs)

> [!WARNING]
> **Never reinterpret wire bytes directly as C++ structs.**
> Direct pointer casting (`reinterpret_cast<const BmpHeaderSection*>(ptr)`) or reading directly into C++ structs introduces severe portability and safety defects:
> - **Compiler Padding & Alignment**: Compilers insert alignment padding between struct members unless non-standard packing pragmas are used.
> - **Wire Layout Discrepancies**: In the 14-byte BMP file header, there are two distinct 16-bit reserved fields (`reserved1` and `reserved2`), totaling 4 bytes. A naive struct with a single `uint16_t reserved` misaligns all subsequent fields.
> - **Endianness & Type Sign**: C++ structs do not enforce little-endian deserialization on big-endian architectures or validate signed integer ranges.

### The Correct Deserialization Pattern
AAII uses C++ structures solely as **logical containers** for verified fields. Bytes must be extracted explicitly using bounds-checked little-endian primitives from `ByteReader`:

```
Input Buffer  -->  ByteReader  -->  Explicit Decode  -->  Logical Struct
[raw bytes]        [bounds check]   [little-endian]       (BmpHeaderSection)
```

#### Step-by-Step Deserialization:
1. Verify that the reader contains at least 14 bytes for the file header.
2. Read signature (`read_u16_le()`) and confirm it equals `0x4D42`.
3. Read total file size (`read_u32_le()`).
4. Read `reserved1` (`read_u16_le()`) and `reserved2` (`read_u16_le()`).
5. Read pixel data offset (`read_u32_le()`).
6. Populate the logical `BmpHeaderSection` container.

---

## 4. Multi-Stage Ingestion & Validation Pipeline

To ensure memory safety and clear diagnostics, the parser breaks verification into five distinct sequential stages:

```
[ Raw Buffer ]
      |
      v
+-------------------------------------------------------------+
| Stage 1: Format Detection                                   |
| - Verify at least 2 bytes exist                             |
| - Check magic bytes == 'B', 'M' (0x4D42)                    |
+-------------------------------------------------------------+
      | (Candidate confirmed)
      v
+-------------------------------------------------------------+
| Stage 2: Header Deserialization                             |
| - Read 14-byte File Header via ByteReader                   |
| - Read 4-byte DIB Header Size                               |
| - Read remaining DIB fields according to declared size      |
+-------------------------------------------------------------+
      | (Fields populated)
      v
+-------------------------------------------------------------+
| Stage 3: Input Bounds & Structural Validation               |
| - Actual buffer size is ultimate authority                  |
| - Check dataOffset <= buffer.size()                         |
| - Check declared header sizes fit within buffer             |
| - Check dimensions against integer overflow and bounds      |
| - Guard against height == INT32_MIN                         |
+-------------------------------------------------------------+
      | (Structurally sound)
      v
+-------------------------------------------------------------+
| Stage 4: Feature Support Verification                       |
| - Verify DIB size == 40 (BITMAPINFOHEADER)                  |
| - Verify bpp == 24                                          |
| - Verify compression == BI_RGB (0)                          |
| - Verify planes == 1                                        |
+-------------------------------------------------------------+
      | (Supported in Milestone 1)
      v
+-------------------------------------------------------------+
| Stage 5: Scanline Decoding                                  |
| - Calculate padded wire stride with overflow checks         |
| - Verify pixel byte range: [dataOffset, dataOffset + size]  |
| - Allocate canonical Image buffer (width * height * 3)      |
| - Strip 4-byte row padding & convert BGR to RGB             |
| - Invert rows if bottom-up to produce top-down output       |
+-------------------------------------------------------------+
```

---

## 5. Bounds Checking & Safety Invariants

The actual byte buffer length is the ultimate security boundary. The parser enforces the following sanity checks:

### 5.1. Height & Integer Safety
- BMP heights are signed 32-bit integers (`int32_t`).
- **`INT32_MIN` Guard**: In two's complement, `-(-2147483648)` overflows 32-bit signed integers. Any image declaring $\text{height} = -2^{31}$ must be rejected immediately.
- If $\text{height} > 0$, the image is bottom-up; if $\text{height} < 0$, it is top-down. The absolute height is taken as $\text{actual\_height} = |\text{height}|$.
- Width must be strictly positive: $\text{width} > 0$.

### 5.2. Scanline Stride & Overflow Math
BMP scanlines are padded to 4-byte (32-bit) boundaries on the wire. Wire row stride is calculated as:

$$\text{wire\_row\_stride} = \left\lfloor \frac{\text{width} \times \text{bpp} + 31}{32} \right\rfloor \times 4$$

For Milestone 1 (`bpp = 24`):
$$\text{wire\_row\_stride} = \left\lfloor \frac{\text{width} \times 24 + 31}{32} \right\rfloor \times 4$$

Calculations must use checked 64-bit unsigned arithmetic (`uint64_t`) to prevent multiplication overflow before verifying:
1. $\text{total\_wire\_pixel\_bytes} = \text{wire\_row\_stride} \times \text{actual\_height}$
2. $\text{dataOffset} + \text{total\_wire\_pixel\_bytes} \le \text{buffer.size()}$

If the actual buffer contains fewer bytes than required by the declared dimensions, the input is truncated and rejected with `Status::TruncatedInput`.

### 5.3. Output Allocation Sanity Thresholds
Before allocating the canonical `Image` buffer:
- Ensure $\text{width} \times \text{actual\_height} \times 3$ does not overflow `size_t`.
- Validate against a maximum library sanity limit (e.g., maximum dimension of 32,768 pixels or maximum decoded memory budget of 512 MB) to prevent denial-of-service from maliciously crafted dimensions.

---

## 6. Scanline Conversion: Wire BGR to Canonical RGB8

In Milestone 1, the decoder performs three conversions in a single pass:
1. **Row Inversion**: If the source is bottom-up ($\text{height} > 0$), wire row $0$ maps to output row $\text{actual\_height} - 1$. If top-down ($\text{height} < 0$), wire row $y$ maps directly to output row $y$.
2. **Padding Removal**: Reads exactly $\text{width} \times 3$ pixel bytes per scanline, skipping $\text{wire\_row\_stride} - (\text{width} \times 3)$ padding bytes before the next row.
3. **Channel Reordering**: Wire pixels are stored in BGR order (`[byte0: Blue, byte1: Green, byte2: Red]`). The decoder writes them to the canonical buffer in standard RGB order (`[R, G, B]`).

---

## 7. Format Survey & Future Milestones

The following features belong to the wider BMP specification and are cataloged here for future milestones:

### Milestone 2: 32-bit BMP Direct Color
- 32-bit `BI_RGB` stores pixels as `[B, G, R, X]`, where the 4th byte is frequently unused padding rather than a meaningful alpha channel. AAII will implement an explicit alpha policy before decoding 32-bit images to prevent treating garbage padding bytes as transparent alpha.
- 32-bit `BI_BITFIELDS` defines arbitrary bitmasks for Red, Green, Blue, and Alpha channels.

### Milestone 3: Indexed Formats ($\le 8\text{ bpp}$)
- 1-bit, 4-bit, and 8-bit images require an indexed palette table of `BmpColorPalette` (BGRA quads) located between the DIB header and pixel data.
- Validation requires ensuring the palette fits between the header end and `dataOffset` without overlapping pixel data.

### Milestone 4: Run-Length Compression (RLE4 / RLE8)
- Encodes repeated pixel indices using control byte sequences. Highly susceptible to out-of-bounds scanline writes if control codes are malformed. Requires strict cursor bounds checks per opcode.
