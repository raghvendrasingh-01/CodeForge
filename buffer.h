#pragma once

#include <iostream>
#include <vector>
#include <string>

class Buffer {
private:
    std::vector<std::string> lines;

public:
    Buffer();
    int lineCount() const;
    const std::string& lineAt(int row) const;
    void insertChar(int row, int col, char ch);
    void deleteChar(int row, int col);
    void insertLine(int pos, const std::string& line);
    void deleteLine(int pos);
    void splitLine(int row, int col);
    void joinLines(int row);
    void clear();
    void display() const;
};