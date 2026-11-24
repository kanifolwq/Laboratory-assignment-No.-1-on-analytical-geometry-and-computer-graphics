#include "geometry_lab.h"

#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

bool isOnGrid(const Point &point) {
    return point.x >= 0 && point.x < GRID_WIDTH &&
           point.y >= 0 && point.y < GRID_HEIGHT;
}

void clearGrid(Grid &grid) {
    int row = 0;
    int column;
    for (; row < GRID_HEIGHT; ++row) {
        column = 0;
        for (; column < GRID_WIDTH; ++column) {
            grid.pixels[row][column] = EMPTY_PIXEL;
        }
    }
}

void drawPixels(Grid &grid, const vector<Point> &pixels) {
    size_t pixelIndex = 0;
    Point pixel;
    for (; pixelIndex < pixels.size(); ++pixelIndex) {
        pixel = pixels[pixelIndex];
        grid.pixels[pixel.y][pixel.x] = LINE_PIXEL;
    }
}

void printGrid(const Grid &grid) {
    // Print the largest y first so that the y-axis points upward.
    int row = GRID_HEIGHT - 1;
    int column;
    for (; row >= 0; --row) {
        std::cout << std::setw(2) << row << ' ';
        column = 0;
        for (; column < GRID_WIDTH; ++column) {
            std::cout << grid.pixels[row][column];
        }
        std::cout << '\n';
    }

    std::cout << "   ";
    column = 0;
    for (; column < GRID_WIDTH; ++column) {
        std::cout << column / 10;
    }
    std::cout << "  x tens\n   ";
    column = 0;
    for (; column < GRID_WIDTH; ++column) {
        std::cout << column % 10;
    }
    std::cout << "  x units\n";
}

int countDifferences(const Grid &first, const Grid &second) {
    int differenceCount = 0;
    int row = 0;
    int column;
    for (; row < GRID_HEIGHT; ++row) {
        column = 0;
        for (; column < GRID_WIDTH; ++column) {
            if (first.pixels[row][column] != second.pixels[row][column]) {
                ++differenceCount;
            }
        }
    }
    return differenceCount;
}
