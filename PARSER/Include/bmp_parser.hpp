/**
 * @file bmp_parser.hpp
 * @brief Header for BMP image parser library
 * It contains the public API for parsing BMP images.
 */

#ifndef AAII_BMP_PARSER_HPP
#define AAII_BMP_PARSER_HPP

#include <sched.h>
#include <cstdint>
#include <string>


namespace aaii {
namespace bmpParser {

/**
 * @brief Represents the color palette of a BMP image
 */
struct BmpColorPalette {
    uint8_t BLUE;
    uint8_t GREEN;
    uint8_t RED;
    uint8_t RESERVED;
};
/*
 * @brief representing the image orientation
 * @info The orientation of the image depends on the value of height, height > 0 = bottom-up, height < 0 = top-down.
 * width doesn't include any scanline boundary padding.
 */
enum class BmpOrientation : uint8_t {
    bottom_up,
    top_down,
};

/**
 * @brief Represents the bit depth of a BMP image
 * @info Bmp files with bpp (bits per pixel) values greater than 16 don't need a color palette.
 * @info Bmp files with bpp (bits per pixel) values less than 16 need a color palette.
 */
enum class BmpBitsPerPixel : uint16_t {
    UNKNOWN = 0,
    BBP1 = 1,
    BBP4 = 4,
    BBP8 = 8,
    BBP16 = 16,
    BBP24 = 24,
    BBP32 = 32
};

/**
 * @brief Represents the compression type of a BMP image
 */
enum class BmpCompressionType : uint16_t {
    NONE = 0,
    RLE8 = 1,
    RLE4 = 2,
    BITERPLANES = 3
};

/**
 * @brief Represents the data type of pixel values in a BMP image
 */
enum class BmpPixelDataType : uint8_t {
    UNKNOWN = 0,
    BYTE = 4,
    WORD = 4,
    DWORD = 8,
    SHORT = 2,
    LONG = 8,
};

/**
 * @brief Represents the color space type of a BMP image
 */
enum class BmpColorSpaceType : uint8_t {
    CALIBRATED_RGB = 0x00,
    DEVICE_DEPENDENT_RGB = 0x01,
    DEVICE_DEPENDENT_CMYK = 0x02,
};

/**
 * @brief Structure representing a section of the BMP file header
 */
struct BmpHeaderSection {
    uint16_t signature;
    uint32_t fileSize;
    uint16_t reserved;
    uint32_t dataOffset; // Offset to pixel data

    bool isValid() const noexcept { return dataOffset > 0 && dataOffset < 0x80000000u; }
};

/**
 * @brief the bit field masks for the color channels
 * @info if the bitmap contains 16 to 32 bits per pixel then only a compression value of 3 is supported
 */
struct BmpBitFieldMasks {
    uint32_t BitMaskRed;
    uint32_t BitMaskGreen;
    uint32_t BitMaskBlue;
    uint32_t BitMaskAlpha;
};

/**
 *  @brief color end point coordinates
 * @info these values are only used when color space type is 00h which is calibrated RGB
 */
struct BmpColorEndPointCoords {
    uint32_t RedX;          /* X coordinate of red endpoint */
    uint32_t RedY;          /* Y coordinate of red endpoint */
    uint32_t RedZ;          /* Z coordinate of red endpoint */
    uint32_t GreenX;        /* X coordinate of green endpoint */
    uint32_t GreenY;        /* Y coordinate of green endpoint */
    uint32_t GreenZ;        /* Z coordinate of green endpoint */
    uint32_t BlueX;         /* X coordinate of blue endpoint */
    uint32_t BlueY;         /* Y coordinate of blue endpoint */
    uint32_t BlueZ;         /* Z coordinate of blue endpoint */
};

/**
 * @brief gamma coordinate scale values
 */
struct BmpGammaScaleCoords {
    uint32_t GammaRed;      /* Gamma red coordinate scale value */
    uint32_t GammaGreen;    /* Gamma green coordinate scale value */
    uint32_t GammaBlue;     /* Gamma blue coordinate scale value */
};

/**
 * @brief Structure representing the DIB header (BITMAPINFOHEADER)
 */
struct BmpHeader {
    uint32_t size; // Size of header in bytes
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t sizeofBitmap; // Size of the image data in bytes
    uint32_t HorzResolution;
    uint32_t VertResolution;
    uint32_t colorused;
    uint32_t colorImportant; // min number of important colors
    BmpBitFieldMasks bitFieldMasks;
    uint32_t ColorSpaceType;
    BmpColorEndPointCoords colorEndPointCoords;
    BmpGammaScaleCoords gammaScaleCoords;
};

/**
 * @brief Class representing a BMP file to manage BMP file operations
 */
class BmpFile{

public:
    BmpFile() = default;
    ~BmpFile() = default;

    bool isValid() const noexcept {
        return verdict;
    }

private:
    BmpPixelDataType bmpPixelDataType;
    BmpHeaderSection bmpheaderSection;
    BmpHeader bmpHeader;
    BmpBitsPerPixel bmpBitsPerPixel;
    BmpColorPalette bmpColorPalette;
    BmpCompressionType bmpCompressionType;
    BmpOrientation bmpOrientation;

    // to know if the file is a valid BMP
    bool verdict = false;
};

/**
 * @brief Validates if a file is a valid BMP file
 *
 * @param filePath Path to the file
 * @return true if the file appears to be a valid BMP, false otherwise
 */
bool isValidBmpFile(const std::string& filePath);

} // namespace bmpparser
} // namespace aaii

#endif // AAII_BMP_PARSER_HPP
