#include "geometry_lab.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace std;

vector<Point> rasterizeBresenham(Point start, Point end) {
    int deltaX = abs(end.x - start.x);
    int deltaY = abs(end.y - start.y);
    int stepX = (end.x > start.x) - (end.x < start.x);
    int stepY = (end.y > start.y) - (end.y < start.y);

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
        currentX = (start.x * stepCount + step * deltaX) / static_cast<double>(stepCount);
        currentY = (start.y * stepCount + step * deltaY) / static_cast<double>(stepCount);
        pixels.push_back({static_cast<int>(round(currentX)),
                          static_cast<int>(round(currentY))});
    }
    return pixels;
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

void drawContour(Grid &grid, const vector<Point> &vertices) {
    size_t vertex = 0;
    size_t nextVertex;
    for (; vertex < vertices.size(); ++vertex) {
        nextVertex = (vertex + 1) % vertices.size();
        drawPixels(grid, rasterizeBresenham(vertices[vertex], vertices[nextVertex]));
    }
}
