#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

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
constexpr int BMP_SCALE = 12;
constexpr int BMP_BYTES_PER_PIXEL = 3;
constexpr int BMP_ROW_ALIGNMENT = 4;
constexpr int BMP_FILE_HEADER_SIZE = 14;
constexpr int BMP_INFO_HEADER_SIZE = 40;
constexpr int BMP_HEADER_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;

struct Point {
    int x;
    int y;
};

constexpr Point NEIGHBOR_OFFSETS[EIGHT_NEIGHBORS] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
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

struct Color {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

bool isOnGrid(const Point &point) {
    return point.x >= 0 && point.x < GRID_WIDTH &&
           point.y >= 0 && point.y < GRID_HEIGHT;
}

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

vector<Point> rasterizeBresenham(Point start, Point end) {
    int deltaX = abs(end.x - start.x);
    int deltaY = abs(end.y - start.y);
    int stepX = (end.x > start.x) - (end.x < start.x);
    int stepY = (end.y > start.y) - (end.y < start.y);

    // Advance along the major axis every step; the decision controls the other.
    bool steep = deltaY > deltaX;
    int majorDelta = steep ? deltaY : deltaX;
    int minorDelta = steep ? deltaX : deltaY;
    int decision = 2 * minorDelta - majorDelta;

    vector<Point> pixels;
    Point current = start;
    int step = 0;
    for (; step <= majorDelta; ++step) {
        pixels.push_back(current);
        if (step == majorDelta) {
            break;
        }

        // Break ties by taking a diagonal step toward the endpoint.
        if (decision >= 0) {
            if (steep) {
                current.x += stepX;
            } else {
                current.y += stepY;
            }
            decision -= 2 * majorDelta;
        }

        if (steep) {
            current.y += stepY;
        } else {
            current.x += stepX;
        }
        decision += 2 * minorDelta;
    }
    return pixels;
}

vector<Point> rasterizeDDA(Point start, Point end) {
    int deltaX = end.x - start.x;
    int deltaY = end.y - start.y;
    int stepCount = max(abs(deltaX), abs(deltaY));
    if (stepCount == 0) {
        return {start};
    }

    vector<Point> pixels;
    double currentX, currentY;
    int step = 0;
    for (; step <= stepCount; ++step) {
        // Divide last to keep exact half-pixel ties on this grid.
        currentX = (start.x * stepCount + step * deltaX) / static_cast<double>(stepCount);
        currentY = (start.y * stepCount + step * deltaY) / static_cast<double>(stepCount);
        pixels.push_back({static_cast<int>(round(currentX)),
                          static_cast<int>(round(currentY))});
    }
    return pixels;
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

int calculateCrossProduct(const Point &start, const Point &end, const Point &point) {
    return (end.x - start.x) * (point.y - start.y) -
           (end.y - start.y) * (point.x - start.x);
}

bool isConvexPolygon(const vector<Point> &vertices) {
    size_t edge = 0;
    size_t vertex, nextEdge;
    Point start, end, point;
    int side, direction = 0;

    // Each edge of a convex polygon has every vertex on the same inner side.
    for (; edge < vertices.size(); ++edge) {
        nextEdge = (edge + 1) % vertices.size();
        start = vertices[edge];
        end = vertices[nextEdge];
        vertex = 0;
        for (; vertex < vertices.size(); ++vertex) {
            if (vertex == edge) {
                continue;
            }

            point = vertices[vertex];
            if (point.x == start.x && point.y == start.y) {
                return false;
            }

            side = calculateCrossProduct(start, end, point);
            if (side == 0) {
                // A vertex strictly between the edge endpoints was listed out of order.
                if ((point.x - start.x) * (point.x - end.x) +
                    (point.y - start.y) * (point.y - end.y) < 0) {
                    return false;
                }
                continue;
            }
            if (direction == 0) {
                direction = side > 0 ? 1 : -1;
            }
            if (side * direction < 0) {
                return false;
            }
        }
    }
    return direction != 0;
    //отсекает случай, когда все все вершины лежат на одной прямой
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

void drawContour(Grid &grid, const vector<Point> &vertices) {
    size_t vertex = 0;
    size_t nextVertex;
    for (; vertex < vertices.size(); ++vertex) {
        nextVertex = (vertex + 1) % vertices.size();
        drawPixels(grid, rasterizeBresenham(vertices[vertex], vertices[nextVertex]));
    }
}

FloodStats floodFill(Grid &grid, Point seed, int neighborCount, char fillPixel) {
    FloodStats stats;
    if (!isOnGrid(seed)) {
        return stats;
    }

    char targetPixel = grid.pixels[seed.y][seed.x];
    if (targetPixel == LINE_PIXEL || targetPixel == fillPixel) {
        return stats;
    }

    // A vector-backed stack avoids a recursive call for every visited pixel.
    vector<Point> pending;
    pending.push_back(seed);
    grid.pixels[seed.y][seed.x] = fillPixel;
    stats.maxStackSize = pending.size();

    Point current, next;
    int neighbor;
    while (!pending.empty()) {
        current = pending.back();
        pending.pop_back();
        ++stats.filledPixels;

        if (current.x == 0 || current.x == GRID_WIDTH - 1 ||
            current.y == 0 || current.y == GRID_HEIGHT - 1) {
            stats.reachedGridEdge = true;
        }

        neighbor = 0;
        for (; neighbor < neighborCount; ++neighbor) {
            next = {current.x + NEIGHBOR_OFFSETS[neighbor].x,
                    current.y + NEIGHBOR_OFFSETS[neighbor].y};
            if (isOnGrid(next) && grid.pixels[next.y][next.x] == targetPixel) {
                // Coloring on insertion keeps each pixel from entering the stack twice.
                grid.pixels[next.y][next.x] = fillPixel;
                pending.push_back(next);
            }
        }

        stats.maxStackSize = max(stats.maxStackSize, pending.size());
    }
    return stats;
}

ScanlineStats scanlineFill(Grid &grid, const vector<Point> &vertices) {
    ScanlineStats stats;
    vector<double> intersections;
    int row = 0;
    int column;
    size_t edge, pairIndex;
    Point start, end;
    double intersectionX, left, right;

    for (; row < GRID_HEIGHT; ++row) {
        ++stats.rowsScanned;
        intersections.clear();
        edge = 0;
        for (; edge < vertices.size(); ++edge) {
            start = vertices[edge];
            end = vertices[(edge + 1) % vertices.size()];
            if (start.y > end.y) {
                swap(start, end);
            }

            // The upper endpoint is excluded, and horizontal edges have an empty interval.
            if (row >= start.y && row < end.y) {
                intersectionX = (start.x * (end.y - start.y) +
                                 (row - start.y) * (end.x - start.x)) /
                                static_cast<double>(end.y - start.y);
                intersections.push_back(intersectionX);
            }
        }

        sort(intersections.begin(), intersections.end());
        pairIndex = 0;
        for (; pairIndex + 1 < intersections.size(); pairIndex += 2) {
            left = intersections[pairIndex];
            right = intersections[pairIndex + 1];

            // Integer coordinates are pixel centers; keep the raster boundary intact.
            column = static_cast<int>(floor(left)) + 1;
            for (; column < right; ++column) {
                if (grid.pixels[row][column] == EMPTY_PIXEL) {
                    grid.pixels[row][column] = FILL_PIXEL;
                    ++stats.filledPixels;
                }
            }
        }
    }
    return stats;
}

//Записывает в точку рекомендуемые координаты 
bool findInteriorPixel(const Grid &grid, Point &point) {
    int row = 0;
    int column;
    for (; row < GRID_HEIGHT; ++row) {
        column = 0;
        for (; column < GRID_WIDTH; ++column) {
            if (grid.pixels[row][column] == FILL_PIXEL) {
                point = {column, row};
                return true;
            }
        }
    }
    return false;
}

Color getPixelColor(char pixel) {
    switch (pixel) {
        case LINE_PIXEL:
            return {45, 35, 25};
        case FILL_PIXEL:
            return {105, 150, 35};
        case FIRST_ONLY_PIXEL:
            return {55, 125, 230};
        case SECOND_ONLY_PIXEL:
            return {225, 105, 50};
        default:
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

    // BMP uses little-endian fields and stores positive-height images bottom first.
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

    // Both algorithms start from identical copies of the original contour.
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
