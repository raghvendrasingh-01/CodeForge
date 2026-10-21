#pragma once

class Cursor {
private:
    int row, col;

public:
    Cursor();
    int getRow() const;
    int getColumn() const;
    void setPosition(int row, int col);
};