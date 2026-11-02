# AAII Architecture & Development Guide

## 1. Overview & Core Philosophy

AAII is a lightweight native C++ image parsing library built to bypass heavy I/O stacks in favor of explicit byte parsing, proper format verification, and clean memory management. The core philosophyis straightforward: This is starting as my learning project and a way for me to learn things from ground up so at the current stage most code is hand written by me and **_Ai use_** is mostly limited to writing docs and discussing architecture plus other planning related tasks, well because I don't have anyone else to mentor me. Enough extra talk, the structure of library is as such :-> ingest binary data safely, validate format integrity, decode supported bitstreams into a predictable and proper memory buffers, and expose that buffer cleanly for downstream consumers (patches, transforms, GUI renderers, ML models, etc).

This guide distinguishes between three levels of commitment to keep things from spiraling into premature architecture (I have a tendency to over-engineer afterall):
1. **Architecture**: The enduring structural boundaries and contracts.
2. **Initial Implementation Specification (Milestone 1)**: The concrete, bounded subset being implemented and verified right now.
3. **Future Roadmap**: Optimizations, concurrency, and dataset features deferred until the core single-file pipeline is proven correct.

---

## 2. Architecture & Design Principles

### Core Boundaries
```
+-----------------------------------------------------------------------+
| Input Acquisition (FileLoader / Memory / Future Stream Adapters)      |
+-----------------------------------------------------------------------+
                                  |
                   Owned Byte Buffer (std::vector<uint8_t>)
                                  |
                   Non-owning View (const uint8_t* + size_t)
                                  v
+-----------------------------------------------------------------------+
| ByteReader (Bounds-checked cursor & range reads, endian helpers)      |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
| Format Detection (Signature sniffing, e.g. 'BM' -> BMP candidate)    |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
| Format Parser & Validator (Header verification, bounds sanity checks) |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
| Format Decoder (Bitstream decode, row alignment, color normalization) |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
| Canonical Image (Tightly-packed top-down buffer, explicit stride)     |
+-----------------------------------------------------------------------+
           |                                             |
           v                                             v
  Downstream Consumers                         Decoupled Observers
  (Patches, Models)                     (CLI Tools, SDL3 GUI)
```

1. **Bounded Buffer as Primary Ingestion Interface**: Rather than mandating an abstract stream interface (`std::istream&`) across every internal component, the primary contract for format parsers is a **bounded byte buffer**. `FileLoader` reads the file into a `std::vector<uint8_t>` (owned bytes), and parsers receive a non-owning view of those bytes — just a `const uint8_t*` pointer and a `std::size_t` length. Stream adapters can wrap this boundary later if genuinely needed, but the core parser requires only bounded random-access memory.
2. **Separation of Detection, Validation, and Decoding**:
   - *Detection*: Identifies candidate formats via leading magic bytes.
   - *Header Parsing*: Deserializes format-specific header fields without overrun.
   - *Validation*: Verifies declared dimensions, offsets, and buffer bounds internally consistent.
   - *Support Check*: Determine if the library supports this specific format variant.
   - *Decoding*: Converts verified source bytes into canonical pixels.
   A signature match (`BM`) is never treated as proof of file integrity, nor does it authorize arbitrary memory allocations.
3. **Canonical Contiguous Memory**: Image output is normalized into a dense, contiguous row-major memory buffer with explicit dimensions, format, and stride.
4. **Decoupled Observers**: CLI utilities and GUI applications (e.g., SDL3 viewers) are downstream observers consuming the canonical `Image`. They don't contain parsing logic or dictate library internals.

---

## 3. Ingestion Pipeline & Component Responsibilities

AAII partitions ingestion responsibilities into distinct logical components. These are *logical responsibilities*, not necessarily seven separate classes — start with whatever is smallest and preserves the boundaries.

| Component | Responsibility |
| :--- | :--- |
| **`FileLoader`** | Opens files strictly in binary mode (`std::ios::binary`), reads the entire file into a `std::vector<uint8_t>`, and reports clear filesystem/IO failures before parser invocation. This is where `std::ifstream` lives. |
| **`ByteReader`** | Takes a non-owning `const uint8_t*` + `std::size_t` view of loaded bytes. Provides bounded, cursor-managed reads for scalar types (`read_u8`, `read_u16_le`, `read_u32_le`, `read_i32_le`) and byte ranges. Rejects any read extending past available input. Knows nothing about BMP, PNG, or any format. |
| **`FormatDetector`** | Inspects leading magic bytes to identify candidate formats (e.g., `0x42 0x4D` for BMP). Rejects unknown or truncated signatures without allocating decoder state. |
| **`BmpParser`** | Deserializes BMP headers field-by-field through `ByteReader`, checks internal structural consistency against the actual buffer size, and verifies that the file matches supported feature sets. |
| **`BmpDecoder`** | Converts validated raw scanlines into canonical pixels, discarding row padding, reordering BGR to RGB, and inverting row order to produce top-down rasters. |
| **`Image`** | Owns the final decoded raster buffer (`std::vector<uint8_t>`) and exposes immutable metadata (width, height, channels, stride, pixel format). |
| **`Status` / Error** | Structured error codes distinguishing IO errors, truncated inputs, invalid signatures, malformed headers, unsupported features, and allocation limits. |

### How `FileLoader` and `ByteReader` Connect

This is the part that was confusing — `FileLoader` and `ByteReader` are two different layers:

```
FileLoader (uses std::ifstream internally)
    |
    | produces: std::vector<uint8_t>  (owned file bytes)
    v
ByteReader (takes const uint8_t*, size_t)
    |
    | provides: read_u16_le(), read_u32_le(), seek(), remaining()
    v
Format Parsers (BmpParser, etc.)
```

`FileLoader` is the only place that touches `std::ifstream`. Once the bytes are loaded into a vector, everything downstream works on a raw pointer+length view through `ByteReader`. The parser never opens a file, never calls `file.read()`, and never worries about stream state.

---

## 4. Canonical Image Contract

To ensure predictable integration with consumers (like the Representation Arena or SDL3 renderers), the canonical `Image` contract enforces the following invariants:

1. **Pixel Formats**:
   - `PixelFormat::RGB8`: 3 bytes per pixel, strictly ordered `[R, G, B]`.
   - `PixelFormat::Gray8`: 1 byte per pixel, luminance/grayscale.
   *(RGBA8 is deferred until an explicit alpha-channel interpretation policy is established.)*
2. **Row Stride & Packing**:
   - In Milestone 1, rows are tightly packed: $\text{stride} = \text{width} \times \text{channels}$.
   - Format-specific wire padding (such as BMP's 4-byte scanline alignment) is stripped during the decode pass.
3. **Orientation**:
   - Canonical image memory is always normalized to **top-down** row order ($y=0$ is the top row of the image). Bottom-up source rasters are inverted during decode.
4. **Memory Ownership**:
   - The pixel buffer is exclusively owned by the `Image` instance (`std::vector<uint8_t>`). Consumers receive const-ref or read-only view access.
5. **Allocation Safety**:
   - Total required memory ($\text{width} \times \text{height} \times \text{channels}$) must be checked for integer overflow using 64-bit arithmetic and validated against a configurable maximum threshold before allocating the destination buffer.

---

## 5. Implementation Milestones

### Milestone 1: The Bounded 24-bit BMP Pipeline (Current Focus)
The immediate goal is a working, fully-tested baseline single-file pipeline:
- [x] Header wire structures and field definitions documented.
- [ ] Implement `ByteReader` with safe little-endian scalar reads and bounds checking.
- [ ] Implement `FileLoader` reading binary files into owned `std::vector<uint8_t>` buffers.
- [ ] Implement `FormatDetector` for BMP signatures (`0x4D42`).
- [ ] Implement explicit BMP header parsing for 40-byte `BITMAPINFOHEADER`.
- [ ] Implement validation: bounds checks, dimensions check ($>0$, overflow guards, `height != INT32_MIN`), offset within file.
- [ ] Implement uncompressed 24-bit (`BI_RGB`) decoder: 4-byte padding removal, BGR-to-RGB conversion, top-down and bottom-up orientation handling.
- [ ] Build end-to-end unit tests: small known BMPs, row padding edge cases, malformed/truncated samples.

### Milestone 2: Format & Depth Extensions
- 32-bit BMP support with verified alpha-channel policy (distinguishing valid RGBA from unused padding bytes).
- Indexed color modes (8-bit and 4-bit) with palette validation and bounds checking.
- Additional formats (PPM/PGM text formats for basic testing, followed by PNG sniff/chunk architecture).

### Milestone 3: Downstream Integration & Interfaces
- SDL3 observer application consuming canonical `Image` buffers.
- Exercising C ABI proposal with real consumers before freezing.

---

## 6. Deferred Roadmap (Non-Immediate Features)

The following capabilities are recognized as potentially useful down the road, but are explicitly removed from the immediate development scope:

- **Complex Work Schedulers & Memory-Budget Pools**: Premature now though but the first concurrency milestone will use simple, bounded file-level worker threads.
- **Power-of-Eight Batch Sizing**: No hard requirement for this. Batch sizes will be determined empirically based on available RAM and thread counts once there's actually a working decoder to benchmark.
- **Content Hashing & Perceptual Hashing (pHash)**: Deduplication and similarity indexing belong to external dataset preprocessing or caching layers, not the foundational image decoding pipeline.
