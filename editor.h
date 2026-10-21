#pragma once

#include "buffer.h"
#include "cursor.h"
#include <string>

class Editor
{
private:
    Buffer buffer;
    Cursor cursor;
    int rowOffset;
    int colOffset;
    int screenRows;
    int screenCols;
    bool isRunning;
    std::string filename;
    std::string statusMessage;

    void enableRawMode();
    void disableRawMode();
    void updateWindowSize();
    void scroll();
    int readKey();
    void processKeyPress();
    void drawRows(std::string& output);
    void drawStatusBar(std::string& output);
    void drawMessageBar(std::string& output);
    void setStatusMessage(const std::string& msg);
    std::string prompt(const std::string& promptText);

public:
    Editor();
    ~Editor();

    bool openFile(const std::string& path);
    bool saveFile();

    void insertChar(char ch);
    void deleteChar();
    void deleteForward();
    void insertNewLine();

    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorUp();
    void moveCursorDown();

    void render();
    void run();
};