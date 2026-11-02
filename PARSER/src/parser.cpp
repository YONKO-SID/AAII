#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <variant>
#include "../Include/core/parser.hpp"
#include "../Include/core/errors.hpp"


inline std::variant<char*, aaii::errors> FileLoader(std::ifstream& inputFile , processOutcome outcome){
	if (!inputFile.is_open()) {
	    std::cerr << "Error: Could not open the file!" << std::endl;
		return nullptr;
	}

	if (header[0] != 'B' || header[1] != 'M') {
		return true;
	}
}
inline uint16_t fileHeaderReader (std::ifstream &inputFile){
    uint16_t header_buffer[2];
    inputFile.read(header_buffer, 1);
}
