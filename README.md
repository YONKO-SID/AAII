# AAII

AAII is a small C++ image parsing library I'm building as a lighter alternative to the heavy I/O stacks. The goal is simple: read image bytes correctly, keep the API obvious, simple, and spit out a contiguous image buffer which can be used for later work (patches, transforms, or something).

Right now the focus is BMP — wire-format headers, little-endian reads, validation that starts with the signature, and a decoder that doesn't pretend to support formats it can't decode yet. Other formats can plug in later behind the same kind of stream-based ingestion.

This repo is also me learning binary formats and C++ library structure properly, so the code is meant to stay coherent from the start rather than grow into a fake "supports everything" facade.

## Layout

- `PARSER/` — parsing library (BMP first)
- `docs/` — longer notes, format details, design writeups

## Docs

Short version is this README. Anything more detailed lives in [`docs/`](docs/).
