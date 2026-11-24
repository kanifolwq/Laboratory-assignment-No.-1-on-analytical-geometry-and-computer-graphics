#ifndef GEOMETRY_LAB_H
#define GEOMETRY_LAB_H

#include <stddef.h>
#include <string>
#include <vector>

constexpr int GRID_WIDTH = 40;
constexpr int GRID_HEIGHT = 20;
constexpr int MIN_VERTICES = 3;
constexpr int MAX_VERTICES = 8;
constexpr int FOUR_NEIGHBORS = 4;
constexpr int EIGHT_NEIGHBORS = 8;
constexpr char EMPTY_PIXEL = '.';
constexpr char LINE_PIXEL = '#';
constexpr char FILL_PIXEL = '*';
constexpr char FIRST_ONLY_PIXEL = 'F';
constexpr char SECOND_ONLY_PIXEL = 'S';

struct Point {
    int x;
    int y;
};

struct Grid {
    char pixels[GRID_HEIGHT][GRID_WIDTH];
};

struct FloodStats {
    int filledPixels = 0;
    size_t maxStackSize = 0;
    bool reachedGridEdge = false;
};

struct ScanlineStats {
    int filledPixels = 0;
    int rowsScanned = 0;
};

bool isOnGrid(const Point &point);
void clearGrid(Grid &grid);
void drawPixels(Grid &grid, const std::vector<Point> &pixels);
void printGrid(const Grid &grid);
int countDifferences(const Grid &first, const Grid &second);

std::vector<Point> rasterizeBresenham(Point start, Point end);
std::vector<Point> rasterizeDDA(Point start, Point end);
bool isConvexPolygon(const std::vector<Point> &vertices);
void drawContour(Grid &grid, const std::vector<Point> &vertices);

FloodStats floodFill(Grid &grid, Point seed, int neighborCount, char fillPixel);
ScanlineStats scanlineFill(Grid &grid, const std::vector<Point> &vertices);
bool findInteriorPixel(const Grid &grid, Point &point);

bool saveBmp(const Grid &grid, const std::string &filename);

#endif
