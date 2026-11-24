#include "geometry_lab.h"

#include <cstdint>
#include <fstream>
#include <string>

using namespace std;

constexpr int BMP_SCALE = 12;
constexpr int BMP_BYTES_PER_PIXEL = 3;
constexpr int BMP_ROW_ALIGNMENT = 4;
constexpr int BMP_FILE_HEADER_SIZE = 14;
constexpr int BMP_INFO_HEADER_SIZE = 40;
constexpr int BMP_HEADER_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;

struct Color {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

Color getPixelColor(char pixel) {
    switch (pixel) {
        case LINE_PIXEL:
            //Серый
            return {45, 35, 25};
        case FILL_PIXEL:
            //Зеленый
            return {105, 150, 35};
        case FIRST_ONLY_PIXEL:
            //синий
            return {55, 125, 230};
        case SECOND_ONLY_PIXEL:
            //Оранжевый
            return {225, 105, 50};
        default:
            //Белый
            return {255, 255, 255};
    }
}

void writeBmpValue(ostream &file, uint32_t value, int byteCount) {
    int byte = 0;
    for (; byte < byteCount; ++byte) {
        file.put(static_cast<char>(value & 0xffu));
        value >>= 8;
    }
}

bool saveBmp(const Grid &grid, const string &filename) {
    ofstream file(filename, ios::binary);
    if (!file) {
        return false;
    }

    int imageWidth = GRID_WIDTH * BMP_SCALE;
    int imageHeight = GRID_HEIGHT * BMP_SCALE;
    int rowBytes = imageWidth * BMP_BYTES_PER_PIXEL;
    int paddingBytes = (BMP_ROW_ALIGNMENT - rowBytes % BMP_ROW_ALIGNMENT) % BMP_ROW_ALIGNMENT;
    int imageBytes = (rowBytes + paddingBytes) * imageHeight;

    file.write("BM", 2);
    writeBmpValue(file, BMP_HEADER_SIZE + imageBytes, 4);
    writeBmpValue(file, 0, 4);  // Reserved fields.
    writeBmpValue(file, BMP_HEADER_SIZE, 4);
    writeBmpValue(file, BMP_INFO_HEADER_SIZE, 4);
    writeBmpValue(file, imageWidth, 4);
    writeBmpValue(file, imageHeight, 4);
    writeBmpValue(file, 1, 2);  // One color plane.
    writeBmpValue(file, BMP_BYTES_PER_PIXEL * 8, 2);
    writeBmpValue(file, 0, 4);  // Uncompressed RGB.
    writeBmpValue(file, imageBytes, 4);
    writeBmpValue(file, 0, 4);  // Horizontal resolution unspecified.
    writeBmpValue(file, 0, 4);  // Vertical resolution unspecified.
    writeBmpValue(file, 0, 4);  // No color table.
    writeBmpValue(file, 0, 4);  // All colors are important.

    int row = 0;
    int column;
    Color color;
    for (; row < imageHeight; ++row) {
        column = 0;
        for (; column < imageWidth; ++column) {
            color = getPixelColor(grid.pixels[row / BMP_SCALE][column / BMP_SCALE]);
            file.put(static_cast<char>(color.blue));
            file.put(static_cast<char>(color.green));
            file.put(static_cast<char>(color.red));
        }
        writeBmpValue(file, 0, paddingBytes);
    }

    file.close();
    return static_cast<bool>(file);
}
