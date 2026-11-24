#include "geometry_lab.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

constexpr Point NEIGHBOR_OFFSETS[EIGHT_NEIGHBORS] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
};

FloodStats floodFill(Grid &grid, Point seed, int neighborCount, char fillPixel) {
    FloodStats stats;
    if (!isOnGrid(seed)) {
        return stats;
    }

    char targetPixel = grid.pixels[seed.y][seed.x];
    if (targetPixel == LINE_PIXEL || targetPixel == fillPixel) {
        return stats;
    }

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
                grid.pixels[next.y][next.x] = fillPixel;
                //закрашиваем точку сразу, чтобы она не добавилась в вектор дважды
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

//Записывает в точку рекомендуемые координаты для floodfill
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
