#include "cursor.h"

Cursor::Cursor() : row(0), col(0) {}

int Cursor::getRow() const { return row; }
int Cursor::getColumn() const { return col; }

void Cursor::setPosition(int row, int col)
{
    this->row = (row < 0) ? 0 : row;
    this->col = (col < 0) ? 0 : col;
}
