CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic
SOURCES = main.cpp grid.cpp geometry.cpp fill.cpp bmp.cpp
TARGET = lab1_full.exe

$(TARGET): $(SOURCES) geometry_lab.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@
