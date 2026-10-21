#include "editor.h"
#include <iostream>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <csignal>
#include <cstdlib>

namespace {
    struct termios orig_termios;
    bool rawModeEnabled = false;

    void cleanUpAtExit() {
        if (rawModeEnabled) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            rawModeEnabled = false;
        }
        std::cout << "\x1b[?25h" << std::flush;
    }

    void handleSignal(int sig) {
        (void)sig;
        cleanUpAtExit();
        std::exit(1);
    }

    enum Key {
        KEY_CTRL_Q = 17,
        KEY_CTRL_S = 19,
        KEY_BACKSPACE = 127,
        KEY_ARROW_UP = 1000,
        KEY_ARROW_DOWN,
        KEY_ARROW_RIGHT,
        KEY_ARROW_LEFT,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_HOME,
        KEY_END,
        KEY_DELETE
    };
}

Editor::Editor()
    : rowOffset(0), colOffset(0), screenRows(24), screenCols(80), isRunning(true), filename("[No Name]"), statusMessage("HELP: Ctrl+S = save | Ctrl+Q = quit")
{
    updateWindowSize();
}

Editor::~Editor()
{
    disableRawMode();
}

void Editor::enableRawMode()
{
    if (rawModeEnabled) return;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;

    static bool cleanupRegistered = false;
    if (!cleanupRegistered) {
        std::atexit(cleanUpAtExit);
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        cleanupRegistered = true;
    }

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1) {
        rawModeEnabled = true;
    }
}

void Editor::disableRawMode()
{
    cleanUpAtExit();
}

void Editor::updateWindowSize()
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        screenRows = 24;
        screenCols = 80;
    } else {
        screenCols = ws.ws_col;
        screenRows = (ws.ws_row > 2) ? (ws.ws_row - 2) : 1; // reserve 1 line for status bar + 1 line for message bar
    }
}

bool Editor::openFile(const std::string& path)
{
    filename = path;
    std::ifstream file(path);
    if (!file.is_open()) {
        setStatusMessage("New file: " + filename);
        return false;
    }

    buffer.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        buffer.insertLine(buffer.lineCount(), line);
    }

    if (buffer.lineCount() == 0) {
        buffer.insertLine(0, "");
    }

    cursor.setPosition(0, 0);
    setStatusMessage("Opened " + filename);
    return true;
}

std::string Editor::prompt(const std::string& promptText)
{
    std::string input;
    while (true) {
        setStatusMessage(promptText + input);
        render();

        std::cout << "\x1b[" << (screenRows + 2) << ";"
                  << (promptText.size() + input.size() + 1) << "H" << std::flush;

        int c = readKey();
        if (c == KEY_BACKSPACE || c == 8 || c == KEY_DELETE) {
            if (!input.empty()) input.pop_back();
        } else if (c == '\x1b' || c == KEY_CTRL_Q || c == '\0') {
            setStatusMessage("Save aborted");
            return "";
        } else if (c == '\r' || c == '\n') {
            if (!input.empty()) {
                setStatusMessage("");
                return input;
            } else {
                setStatusMessage("Save aborted");
                return "";
            }
        } else if (c >= 32 && c <= 126) {
            input.push_back(static_cast<char>(c));
        }
    }
}

bool Editor::saveFile()
{
    if (filename.empty() || filename == "[No Name]") {
        std::string newName = prompt("Save as: ");
        if (newName.empty()) {
            return false;
        }
        filename = newName;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        setStatusMessage("Can't save! I/O error on " + filename);
        return false;
    }

    int bytes = 0;
    for (int i = 0; i < buffer.lineCount(); i++) {
        file << buffer.lineAt(i) << "\n";
        bytes += static_cast<int>(buffer.lineAt(i).size()) + 1;
    }
    file.flush();
    if (!file) {
        setStatusMessage("Can't save! Write error on " + filename);
        return false;
    }

    setStatusMessage(std::to_string(bytes) + " bytes written to disk (" + filename + ")");
    return true;
}

void Editor::insertChar(char ch)
{
    buffer.insertChar(cursor.getRow(), cursor.getColumn(), ch);
    cursor.setPosition(cursor.getRow(), cursor.getColumn() + 1);
}

void Editor::deleteChar()
{
    int row = cursor.getRow();
    int col = cursor.getColumn();

    if (col > 0) {
        buffer.deleteChar(row, col - 1);
        cursor.setPosition(row, col - 1);
    } else if (row > 0) {
        int prevRowLen = static_cast<int>(buffer.lineAt(row - 1).size());
        buffer.joinLines(row - 1);
        cursor.setPosition(row - 1, prevRowLen);
    }
}

void Editor::deleteForward()
{
    int row = cursor.getRow();
    int col = cursor.getColumn();
    int lineLen = static_cast<int>(buffer.lineAt(row).size());

    if (col < lineLen) {
        buffer.deleteChar(row, col);
    } else if (row + 1 < buffer.lineCount()) {
        buffer.joinLines(row);
    }
}

void Editor::insertNewLine()
{
    int row = cursor.getRow();
    int col = cursor.getColumn();
    buffer.splitLine(row, col);
    cursor.setPosition(row + 1, 0);
}

void Editor::moveCursorLeft()
{
    if (cursor.getColumn() > 0) {
        cursor.setPosition(cursor.getRow(), cursor.getColumn() - 1);
    } else if (cursor.getRow() > 0) {
        int prevRow = cursor.getRow() - 1;
        cursor.setPosition(prevRow, static_cast<int>(buffer.lineAt(prevRow).size()));
    }
}

void Editor::moveCursorRight()
{
    int row = cursor.getRow();
    int col = cursor.getColumn();
    int lineLen = static_cast<int>(buffer.lineAt(row).size());

    if (col < lineLen) {
        cursor.setPosition(row, col + 1);
    } else if (row + 1 < buffer.lineCount()) {
        cursor.setPosition(row + 1, 0);
    }
}

void Editor::moveCursorUp()
{
    if (cursor.getRow() > 0) {
        int newRow = cursor.getRow() - 1;
        int targetCol = std::min(cursor.getColumn(), static_cast<int>(buffer.lineAt(newRow).size()));
        cursor.setPosition(newRow, targetCol);
    }
}

void Editor::moveCursorDown()
{
    if (cursor.getRow() + 1 < buffer.lineCount()) {
        int newRow = cursor.getRow() + 1;
        int targetCol = std::min(cursor.getColumn(), static_cast<int>(buffer.lineAt(newRow).size()));
        cursor.setPosition(newRow, targetCol);
    }
}

void Editor::scroll()
{
    if (cursor.getRow() < rowOffset) {
        rowOffset = cursor.getRow();
    }
    if (cursor.getRow() >= rowOffset + screenRows) {
        rowOffset = cursor.getRow() - screenRows + 1;
    }
    if (cursor.getColumn() < colOffset) {
        colOffset = cursor.getColumn();
    }
    if (cursor.getColumn() >= colOffset + screenCols) {
        colOffset = cursor.getColumn() - screenCols + 1;
    }
}

int Editor::readKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) return '\0';
        if (nread == 0 && !rawModeEnabled) return '\0';
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
            }
        }
        return '\x1b';
    }

    return c;
}

void Editor::processKeyPress()
{
    int c = readKey();
    if (c == '\0') {
        isRunning = false;
        return;
    }

    switch (c) {
        case KEY_CTRL_Q:
            isRunning = false;
            break;

        case KEY_CTRL_S:
            saveFile();
            break;

        case '\r':
        case '\n':
            insertNewLine();
            break;

        case '\t':
            for (int i = 0; i < 4; i++) {
                insertChar(' ');
            }
            break;

        case KEY_BACKSPACE:
        case 8:
            deleteChar();
            break;

        case KEY_DELETE:
            deleteForward();
            break;

        case KEY_ARROW_LEFT:
            moveCursorLeft();
            break;

        case KEY_ARROW_RIGHT:
            moveCursorRight();
            break;

        case KEY_ARROW_UP:
            moveCursorUp();
            break;

        case KEY_ARROW_DOWN:
            moveCursorDown();
            break;

        case KEY_HOME:
            cursor.setPosition(cursor.getRow(), 0);
            break;

        case KEY_END:
            cursor.setPosition(cursor.getRow(), static_cast<int>(buffer.lineAt(cursor.getRow()).size()));
            break;

        case KEY_PAGE_UP:
            for (int i = 0; i < screenRows; i++) moveCursorUp();
            break;

        case KEY_PAGE_DOWN:
            for (int i = 0; i < screenRows; i++) moveCursorDown();
            break;

        default:
            if (c >= 32 && c <= 126) {
                insertChar(static_cast<char>(c));
            }
            break;
    }
}

void Editor::drawRows(std::string& output)
{
    for (int y = 0; y < screenRows; y++) {
        int fileRow = y + rowOffset;
        if (fileRow >= buffer.lineCount()) {
            output.append("~");
        } else {
            const std::string& line = buffer.lineAt(fileRow);
            int len = static_cast<int>(line.size()) - colOffset;
            if (len < 0) len = 0;
            if (len > screenCols) len = screenCols;
            if (len > 0) {
                output.append(line.substr(colOffset, len));
            }
        }
        output.append("\x1b[K\r\n");
    }
}

void Editor::drawStatusBar(std::string& output)
{
    output.append("\x1b[7m");
    std::string status = " " + filename + " - " + std::to_string(buffer.lineCount()) + " lines";
    std::string pos = std::to_string(cursor.getRow() + 1) + "," + std::to_string(cursor.getColumn() + 1) + " | Ctrl+S: Save | Ctrl+Q: Quit ";
    
    int len = static_cast<int>(status.size());
    int rlen = static_cast<int>(pos.size());
    if (len > screenCols) len = screenCols;
    output.append(status.substr(0, len));

    int spaces = screenCols - len - rlen;
    if (spaces > 0) {
        output.append(std::string(spaces, ' '));
        output.append(pos);
    } else if (screenCols > len) {
        output.append(std::string(screenCols - len, ' '));
    }
    output.append("\x1b[m\r\n");
}

void Editor::drawMessageBar(std::string& output)
{
    output.append("\x1b[K");
    int msgLen = static_cast<int>(statusMessage.size());
    if (msgLen > screenCols) msgLen = screenCols;
    if (msgLen > 0) {
        output.append(statusMessage.substr(0, msgLen));
    }
}

void Editor::setStatusMessage(const std::string& msg)
{
    statusMessage = msg;
}

void Editor::render()
{
    updateWindowSize();
    scroll();

    std::string output;
    output.reserve(screenRows * screenCols + 256);

    output.append("\x1b[?25l");
    output.append("\x1b[H");

    drawRows(output);
    drawStatusBar(output);
    drawMessageBar(output);

    int cursorScreenY = (cursor.getRow() - rowOffset) + 1;
    int cursorScreenX = (cursor.getColumn() - colOffset) + 1;
    output.append("\x1b[" + std::to_string(cursorScreenY) + ";" + std::to_string(cursorScreenX) + "H");
    output.append("\x1b[?25h");

    std::cout << output << std::flush;
}

void Editor::run()
{
    enableRawMode();
    updateWindowSize();

    while (isRunning) {
        render();
        processKeyPress();
    }

    std::cout << "\x1b[2J\x1b[H" << std::flush;
}