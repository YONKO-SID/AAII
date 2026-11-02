/**
 * @file errors.hpp
 * @brief Header for errors of the image parser library
 * It contains the public API for listing types of errors that can occur.
 */
namespace aaii{
    namespace errors {

enum class ErrorCode : int {
    success = 0 ,

    // Generic/File Errors
    FileNotFound,
    InvalidArgs,
    EmptyBuffer,
    UnsupportedFormat,
    MemoryAllocationFailed,

    // BMP Specific Errors
    BmpInvalidSignature,       // Missing 'BM'
    BmpUnsupportedBpp,         // Missing support
    BmpInvalidDimensions,
    BmpCorruptedHeader
};

    }
}
