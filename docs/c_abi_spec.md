# AAII C ABI Specification (Draft Proposal)

> [!IMPORTANT]
> **Status**: Draft Proposal — Not Yet Implemented or Frozen  
> **ABI Target Version**: 1.0  
> **Language Boundary**: C-Compatible ABI (`extern "C"`)  
> **Primary Implementation**: Native C++  
>
> This specification represents a proposed external C interface contract. It is intentionally kept as an unfrozen draft while the underlying C++ ingestion pipeline, canonical image representation, error types, and consumer contracts (CLI tools, SDL3 GUI, Representation Arena) are implemented and exercised in code. The C ABI will be frozen only after these components have stabilized through real-world usage.

---

## 1. Purpose & Design Goals

The AAII C ABI proposal defines a minimal, stable C-compatible interface for image loading, format detection, header inspection, and pixel buffer retrieval.

The C ABI is designed to serve as the external boundary for:
- Standalone CLI utilities
- GUI observers (such as SDL3 desktop applications)
- External language bindings and foreign function interfaces (FFI for Python, Rust, etc.)
- Dynamically linked shared libraries (`AAII.dll` on Windows / `libaaii.so` on Linux)

### Proposed Boundary Invariants
1. **Zero C++ Types Across Boundary**: No standard library types (`std::string`, `std::vector`, `std::istream`), templates, or C++ classes cross the ABI.
2. **No Uncaught Exceptions**: All C++ exceptions are trapped internally at the API boundary and translated into explicit `AAII_Error` return codes.
3. **Explicit Memory Ownership**: Allocations performed inside AAII are freed exclusively by `aaii_image_free()`. Callers never execute raw `free()` or `delete` on library handles.
4. **Opaque Pointers**: Image objects are represented by an opaque handle (`AAII_Image*`), keeping internal C++ implementation details strictly hidden.
5. **Fixed-Width Integers**: All integer fields specify explicit bit-widths (`uint32_t`, `int32_t`, `uint8_t`, `size_t`) via `<stdint.h>`.

---

## 2. Proposed Header Contract (`aaii.h`)

```c
#ifndef AAII_H
#define AAII_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AAII_ABI_VERSION_MAJOR 1
#define AAII_ABI_VERSION_MINOR 0

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef AAII_BUILD_DLL
        #define AAII_API __declspec(dllexport)
    #else
        #define AAII_API __declspec(dllimport)
    #endif
#else
    #define AAII_API __attribute__((visibility("default")))
#endif

/* -------------------------------------------------------------
 * 1. Error Status Codes
 * ------------------------------------------------------------- */
typedef enum AAII_Error {
    AAII_OK = 0,

    AAII_ERROR_INVALID_ARGUMENT,
    AAII_ERROR_OUT_OF_MEMORY,

    AAII_ERROR_IO,
    AAII_ERROR_TRUNCATED_INPUT,

    AAII_ERROR_UNKNOWN_FORMAT,
    AAII_ERROR_INVALID_SIGNATURE,
    AAII_ERROR_INVALID_HEADER,
    AAII_ERROR_CORRUPT_DATA,

    AAII_ERROR_UNSUPPORTED_FORMAT,
    AAII_ERROR_UNSUPPORTED_FEATURE,

    AAII_ERROR_FORMAT_MISMATCH,

    AAII_ERROR_INTERNAL
} AAII_Error;

AAII_API const char* aaii_error_string(AAII_Error error);

/* -------------------------------------------------------------
 * 2. Canonical Pixel Formats
 * ------------------------------------------------------------- */
typedef enum AAII_PixelFormat {
    AAII_PIXEL_UNKNOWN = 0,
    AAII_PIXEL_GRAY8   = 1,
    AAII_PIXEL_RGB8    = 2,
    AAII_PIXEL_RGBA8   = 3
} AAII_PixelFormat;

/* -------------------------------------------------------------
 * 3. Image Metadata
 * ------------------------------------------------------------- */
typedef struct AAII_ImageInfo {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t stride;
    AAII_PixelFormat pixel_format;
} AAII_ImageInfo;

/* -------------------------------------------------------------
 * 4. Opaque Image Handle & Lifecycle
 * ------------------------------------------------------------- */
typedef struct AAII_Image AAII_Image;

AAII_API void aaii_image_free(AAII_Image* image);

AAII_API AAII_Error aaii_image_info(
    const AAII_Image* image,
    AAII_ImageInfo* info
);

AAII_API AAII_Error aaii_image_pixels(
    const AAII_Image* image,
    const uint8_t** pixels,
    size_t* size
);

/* -------------------------------------------------------------
 * 5. Ingestion Functions
 * ------------------------------------------------------------- */
AAII_API AAII_Error aaii_load_file(
    const char* path,
    AAII_Image** image
);

AAII_API AAII_Error aaii_load_memory(
    const void* data,
    size_t size,
    AAII_Image** image
);

#ifdef __cplusplus
}
#endif

#endif /* AAII_H */
```

---

## 3. Function Behavior & Ownership Rules

### 3.1. Error Reporting
```c
AAII_API const char* aaii_error_string(AAII_Error error);
```
- Returns a pointer to a null-terminated static ASCII string describing the error code.
- The returned pointer is owned by the library; the caller must not free or mutate it.

---

### 3.2. Image Loading
```c
AAII_API AAII_Error aaii_load_file(const char* path, AAII_Image** image);
AAII_API AAII_Error aaii_load_memory(const void* data, size_t size, AAII_Image** image);
```
- **Format Sniffing**: Formats are detected from byte signatures; extensions are not authoritative.
- **Output Pointer Safety**: On `AAII_OK`, `*image` contains a pointer to an allocated `AAII_Image`. On any failure, `*image` is guaranteed to be set to `NULL`.
- **Memory Buffer Ingestion**: For `aaii_load_memory`, the caller maintains ownership of the input buffer and must ensure it remains valid for the duration of the call. AAII copies required bytes during decode.

---

### 3.3. Metadata & Pixel Extraction
```c
AAII_API AAII_Error aaii_image_info(const AAII_Image* image, AAII_ImageInfo* info);
AAII_API AAII_Error aaii_image_pixels(const AAII_Image* image, const uint8_t** pixels, size_t* size);
```
- `aaii_image_info` populates `info` with width, height, channel count, unpacked row stride, and canonical pixel format.
- `aaii_image_pixels` yields a zero-copy read-only pointer to the contiguous raster pixel buffer and its total byte size. The pointer remains valid as long as the parent `AAII_Image` has not been freed.

---

### 3.4. Deallocation
```c
AAII_API void aaii_image_free(AAII_Image* image);
```
- Releases all internal raster buffers and object allocations associated with the image handle.
- Passing `NULL` is safe and performs no operation.
