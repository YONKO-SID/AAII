# AAII Documentation

This directory contains technical specifications, architecture decisions, format notes, and API boundaries for the AAII image parsing library.

---

## Documentation Index

- **[Development & Architecture Guide](Development_guide.md)**  
  High-level architecture, design philosophy, bounded byte-reader ingestion model (`const uint8_t*` + `size_t`, C++17 compatible), multi-stage validation lifecycle (detection → header parsing → validation → support check → decoding), canonical image memory contract, frontend decoupling (CLI vs. SDL3 GUI), and implementation milestones. Explicitly separates immediate commitments from the deferred roadmap.

- **[BMP Architecture & Decoding Specification](File_type_specific/BMP_Architecture.md)**  
  Detailed binary specification for Windows Bitmap (BMP) parsing in `aaii::bmpParser`. Clearly distinguishes the full BMP format survey from the **Milestone 1 supported contract** (uncompressed 24-bit BGR with 40-byte `BITMAPINFOHEADER`). Details explicit byte deserialization via `ByteReader` (never direct struct casting), buffer-bounds validation, integer overflow guards (`INT32_MIN`), 4-byte wire row alignment, and scanline unpack to canonical RGB8.

- **[C ABI Specification (Draft Proposal)](c_abi_spec.md)**  
  The proposed C-compatible application binary interface (`aaii.h`). Details boundary guarantees, memory ownership rules, error status codes, metadata inspection structs, opaque image handles, and C ABI ingestion functions. Marked as a **draft proposal — not yet implemented or frozen** pending validation with real consumers.

---

## Directory Conventions

- Core library implementation and C++ public headers reside in [`PARSER/`](../PARSER/).
- Format-specific decoding specifications live in [`File_type_specific/`](File_type_specific/).
- SDL3 observer application files reside in [`GUI/`](../GUI/).
- CLI observer application files reside in [`CLI/`](../CLI/).