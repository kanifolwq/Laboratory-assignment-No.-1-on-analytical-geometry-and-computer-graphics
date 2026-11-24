#include "geometry_lab.h"

#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//Я не знал, что этим нельзя пользоваться до сегодняшней лекции
using namespace std;

bool readPoint(const string &prompt, Point &point) {
    string input, extra;
    istringstream inputStream;
    while (true) {
        std::cout << prompt;
        if (!getline(cin, input)) {
            return false;
        }

        inputStream.clear();
        inputStream.str(input);
        if (!(inputStream >> point.x) || !isspace(inputStream.peek()) ||
            !(inputStream >> point.y) || (inputStream >> extra)) {
            std::cout << "Error: enter exactly two integers separated by whitespace.\n";
            continue;
        }
        if (!isOnGrid(point)) {
            std::cout << "Error: x must be from 0 to " << GRID_WIDTH - 1
                 << ", y from 0 to " << GRID_HEIGHT - 1 << ".\n";
            continue;
        }
        return true;
    }
}

bool readInteger(const string &prompt, int minimum, int maximum, int &value) {
    string input, extra;
    istringstream inputStream;
    while (true) {
        std::cout << prompt;
        if (!getline(cin, input)) {
            return false;
        }

        inputStream.clear();
        inputStream.str(input);
        if ((inputStream >> value) && !(inputStream >> extra) &&
            value >= minimum && value <= maximum) {
            return true;
        }

        std::cout << "Error: enter one integer from " << minimum
             << " to " << maximum << ".\n";
    }
}

bool readVertices(vector<Point> &vertices) {
    int vertexCount;
    int vertex;
    while (true) {
        if (!readInteger("Number of vertices (3-8): ", MIN_VERTICES, MAX_VERTICES, vertexCount)) {
            return false;
        }

        vertices.resize(vertexCount);
        std::cout << "List vertices in order around the boundary.\n";
        vertex = 0;
        for (; vertex < vertexCount; ++vertex) {
            if (!readPoint("Vertex " + to_string(vertex + 1) + " (x y): ", vertices[vertex])) {
                return false;
            }
        }

        if (isConvexPolygon(vertices)) {
            return true;
        }
        std::cout << "Error: enter a convex polygon with distinct vertices and nonzero area.\n";
    }
}

void showGrid(const string &title, const Grid &grid, const string &filename) {
    std::cout << '\n' << title << ":\n";
    printGrid(grid);
    if (saveBmp(grid, filename)) {
        std::cout << "Saved: " << filename << '\n';
    } else {
        std::cout << "Error: could not write " << filename << ".\n";
    }
}

void printFloodStats(const FloodStats &stats) {
    std::cout << "Filled pixels: " << stats.filledPixels
         << ", stack removals: " << stats.filledPixels
         << ", peak pending pixels: " << stats.maxStackSize << '\n'
         << "Reached the edge of the grid: " << (stats.reachedGridEdge ? "yes" : "no") << '\n';
}

void compareFills(const Grid &first, const Grid &second, const string &firstName,
                  const string &secondName, const string &filename) {
    Grid comparison = first;
    int row = 0;
    int column;
    int firstOnly = 0;
    int secondOnly = 0;
    bool inFirst, inSecond;
    for (; row < GRID_HEIGHT; ++row) {
        column = 0;
        for (; column < GRID_WIDTH; ++column) {
            inFirst = first.pixels[row][column] == FILL_PIXEL;
            inSecond = second.pixels[row][column] == FILL_PIXEL;
            if (inFirst && !inSecond) {
                comparison.pixels[row][column] = FIRST_ONLY_PIXEL;
                ++firstOnly;
            } else if (inSecond && !inFirst) {
                comparison.pixels[row][column] = SECOND_ONLY_PIXEL;
                ++secondOnly;
            }
        }
    }

    showGrid("Fill comparison", comparison, filename);
    std::cout << "* = both, F = " << firstName << " only, S = " << secondName << " only.\n"
         << firstName << " only: " << firstOnly << ", " << secondName << " only: " << secondOnly
         << ", differing cells: " << firstOnly + secondOnly << ".\n";
}

bool runLineComparison() {
    Point start, end;
    if (!readPoint("Start point (x y): ", start) ||
        !readPoint("End point (x y): ", end)) {
        return false;
    }

    vector<Point> bresenhamPixels = rasterizeBresenham(start, end);
    vector<Point> ddaPixels = rasterizeDDA(start, end);
    Grid bresenhamGrid, ddaGrid;
    clearGrid(bresenhamGrid);
    clearGrid(ddaGrid);
    drawPixels(bresenhamGrid, bresenhamPixels);
    drawPixels(ddaGrid, ddaPixels);

    showGrid("Bresenham", bresenhamGrid, "line_bresenham.bmp");
    showGrid("DDA", ddaGrid, "line_dda.bmp");

    std::cout << "\nBresenham pixels: " << bresenhamPixels.size()
         << ", steps between pixels: " << bresenhamPixels.size() - 1 << '\n';
    std::cout << "DDA pixels: " << ddaPixels.size()
         << ", steps between pixels: " << ddaPixels.size() - 1 << '\n';
    std::cout << "Geometric length in pixel units: "
         << hypot(end.x - start.x, end.y - start.y) << '\n';
    int differenceCount = countDifferences(bresenhamGrid, ddaGrid);
    std::cout << "Differing cells: " << differenceCount << '\n';
    if (differenceCount == 0) {
        std::cout << "The pixel sets match.\n";
    } else {
        std::cout << "On an exact tie, Bresenham steps toward the endpoint.\n"
             << "DDA rounds a half up on this grid.\n";
    }
    return true;
}

bool runPolygonFill() {
    vector<Point> vertices;
    if (!readVertices(vertices)) {
        return false;
    }

    Grid contour;
    clearGrid(contour);
    drawContour(contour, vertices);
    showGrid("Bresenham contour", contour, "polygon_contour.bmp");

    Grid scanlineGrid = contour;
    ScanlineStats scanlineStats = scanlineFill(scanlineGrid, vertices);
    Point seed, suggestedSeed;
    if (!findInteriorPixel(scanlineGrid, suggestedSeed)) {
        std::cout << "No interior pixels for a seed. Try a larger polygon.\n";
        return true;
    }

    std::cout << "Example interior seed: " << suggestedSeed.x << ' ' << suggestedSeed.y << '\n';
    while (true) {
        if (!readPoint("Interior seed (x y): ", seed)) {
            return false;
        }
        if (scanlineGrid.pixels[seed.y][seed.x] == FILL_PIXEL) {
            break;
        }
        std::cout << "Error: choose an interior pixel away from the contour.\n";
    }

    int connectivityChoice;
    if (!readInteger("Connectivity (1 = four neighbors, 2 = eight neighbors): ",
                     1, 2, connectivityChoice)) {
        return false;
    }
    int neighborCount = connectivityChoice == 1 ? FOUR_NEIGHBORS : EIGHT_NEIGHBORS;

    Grid floodGrid = contour;
    FloodStats floodStats = floodFill(floodGrid, seed, neighborCount, FILL_PIXEL);
    string neighborLabel = to_string(neighborCount) + "-neighbor";
    showGrid(neighborLabel + " flood fill", floodGrid, "polygon_flood_" + to_string(neighborCount) + ".bmp");
    printFloodStats(floodStats);

    showGrid("Scanline fill", scanlineGrid, "polygon_scanline.bmp");
    std::cout << "Filled pixels: " << scanlineStats.filledPixels
         << ", rows scanned: " << scanlineStats.rowsScanned << '\n';

    compareFills(floodGrid, scanlineGrid, neighborLabel + " flood fill", "scanline fill",
                 "polygon_comparison.bmp");
    return true;
}

void runStaircaseDemo() {
    const vector<Point> vertices = {{8, 2}, {13, 7}, {8, 12}, {3, 7}};
    const Point seed = {8, 7};
    Grid contour;
    clearGrid(contour);
    drawContour(contour, vertices);
    std::cout << "\nOne-pixel diagonal staircase, seed (" << seed.x << ", " << seed.y << ").\n";
    showGrid("Bresenham contour", contour, "staircase_contour.bmp");

    Grid fourGrid = contour;
    Grid eightGrid = contour;
    FloodStats fourStats = floodFill(fourGrid, seed, FOUR_NEIGHBORS, FILL_PIXEL);
    FloodStats eightStats = floodFill(eightGrid, seed, EIGHT_NEIGHBORS, FILL_PIXEL);
    showGrid("Four-neighbor flood fill", fourGrid, "staircase_flood_4.bmp");
    printFloodStats(fourStats);
    showGrid("Eight-neighbor flood fill", eightGrid, "staircase_flood_8.bmp");
    printFloodStats(eightStats);
    compareFills(fourGrid, eightGrid, "four-neighbor fill", "eight-neighbor fill",
                 "staircase_comparison.bmp");
    std::cout << "From (8, 3) to (7, 2), a diagonal step crosses between "
         << "the boundary pixels (8, 2) and (7, 3).\n";
}

int main() {
    std::cout << "Line rasterization and polygon filling. Grid "
         << GRID_WIDTH << " x " << GRID_HEIGHT << ".\n"
         << "Coordinates: x from 0 to " << GRID_WIDTH - 1
         << ", y from 0 to " << GRID_HEIGHT - 1 << ".\n"
         << ". = empty, # = line or boundary, * = filled.\n";

    int choice;
    while (true) {
        std::cout << "\n1. Compare Bresenham and DDA\n"
             << "2. Draw and fill a convex polygon\n"
             << "3. Compare connectivity on a diagonal staircase\n"
             << "0. Exit\n";

        if (!readInteger("Select a mode: ", 0, 3, choice)) {
            break;
        }

        switch (choice) {
            case 0:
                return 0;
            case 1:
                if (!runLineComparison()) {
                    std::cout << "\nInput ended.\n";
                    return 0;
                }
                break;
            case 2:
                if (!runPolygonFill()) {
                    std::cout << "\nInput ended.\n";
                    return 0;
                }
                break;
            case 3:
                runStaircaseDemo();
                break;
        }
    }

    std::cout << "\nInput ended.\n";
    return 0;
}
