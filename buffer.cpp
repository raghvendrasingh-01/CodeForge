#include "buffer.h"

Buffer::Buffer()
{
    lines.emplace_back("");
}

int Buffer::lineCount() const
{
    return static_cast<int>(lines.size());
    /*lines.size() 
    actually returns an unsigned type called std::size_t*/
}
const std::string& Buffer::lineAt(int row) const
{
    static const std::string emptyLine;
    if (row < 0 || row >= lineCount()) {
        return emptyLine;
    }
    return lines[row];
}
void Buffer::display() const
{
    for(int i = 0 ; i < lineCount() ; i++)
    {
        std::cout << lineAt(i) << std::endl;
    }
}

void Buffer::clear()
{
    lines.clear();
}

void Buffer::insertChar(int row, int col, char ch)
{
    if (row < 0 || row >= lineCount()) return;
    if (col < 0) col = 0;
    if (col > static_cast<int>(lines[row].size())) {
        col = static_cast<int>(lines[row].size());
    }
    lines[row].insert(col, 1, ch);
}

void Buffer::deleteChar(int row, int col)
{
    if (row < 0 || row >= lineCount()) return;
    if (col < 0 || col >= static_cast<int>(lines[row].size())) return;
    lines[row].erase(col, 1);
}

void Buffer::insertLine(int pos, const std::string& line)
{
    if (pos < 0) pos = 0;
    if (pos > lineCount()) pos = lineCount();
    lines.insert(lines.begin() + pos, line);
}

void Buffer::deleteLine(int pos)
{
    if (pos < 0 || pos >= lineCount()) return;
    if (lineCount() == 1)
    {
        lines[0] = "";
        return;
    }
    lines.erase(lines.begin() + pos);
}

void Buffer::splitLine(int row, int col)
{
    if (row < 0 || row >= lineCount()) return;
    if (col < 0) col = 0;
    if (col > static_cast<int>(lines[row].size())) {
        col = static_cast<int>(lines[row].size());
    }
    insertLine(row + 1, lines[row].substr(col));
    lines[row].erase(col);
}

void Buffer::joinLines(int row)
{
    if (row < 0 || row + 1 >= lineCount()) return;
    lines[row].append(lines[row + 1]);
    deleteLine(row + 1);
}