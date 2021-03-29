#ifndef TERM_HPP
#define TERM_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

#ifdef __linux__
#include <cstdio>
#include <sys/ioctl.h>
#include <sys/select.h>
#endif

#ifdef _WIN32
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "termUtils.hpp"

namespace term
{

/** @brief Reset all */
constexpr std::string_view RESET("\x1b[!p");

/** @brief Numbers of rows and columns */
struct Size {
    std::size_t rows;
    std::size_t cols;

    Size()
        : rows(0)
        , cols(0)
    {
    }

    Size(const std::size_t rows, const std::size_t cols)
        : rows(rows)
        , cols(cols)
    {
    }

    bool operator == (const Size& sz) {
        return rows == sz.rows && cols == sz.cols;
    }

    bool operator != (const Size& sz) {
        return rows != sz.rows || cols != sz.cols;
    }
};

/** @brief Position with row and column */
struct Pos {
    std::size_t row;
    std::size_t col;

    Pos()
        : row(0)
        , col(0)
    {
    }

    Pos(const std::size_t row, const std::size_t col)
        : row(row)
        , col(col)
    {
    }
};

/** @brief Reset all function */
void reset()
{
    std::cout << RESET << std::flush;
}

/**
 * @brief Size of the terminal
 * @return Size rows and cols of terminal
 */
Size size()
{
    std::size_t r = 0;
    std::size_t c = 0;

#ifdef __linux__
    winsize w;
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    r = w.ws_row;
    c = w.ws_col;
#endif

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO sbInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
    c = sbInfo.dwSize.X;
    r = sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1;
#endif
    return Size(r, c);
}

/** @brief Key event */
struct KeyEvent {
    term::Key code;
    int value;

    KeyEvent(term::Key code, int value)
        : code(code)
        , value(value)
    {
    }

    KeyEvent()
        : code(term::Key::None)
        , value(0)
    {
    }

    std::string toChar()
    {
        char c = static_cast<char>(value);
        std::string s(&c);
        return s;
    }
};

/**
 * @brief Check if a key is pressed.
 * @return True if the key is pressed, false otherwise
 */
bool isKeyPressed()
{
    return termUtils::kbHit() > 0;
}

/**
 * @brief Get key presses
 * @return Key with code and value
 */
KeyEvent keyPress()
{
    int chr = 0;
    int hit = termUtils::kbHit();
    std::vector<int> chars;
    KeyEvent k;

    while(hit > 0) {
#ifdef _WIN32
        chr = _getch();
        chars.emplace_back(chr);
        if(chr == 0 || chr == 224) {
            chr = _getch();
            chars.emplace_back(chr);
        }

#endif
#ifdef __linux__
        chr = std::getchar();
        chars.emplace_back(chr);
#endif

        hit--;
    }

    /*
        if(chr > 0) {
            for(std::size_t i = 0; i < chars.size(); ++i) {
                std::cout << chars[i] << " ";
            }
            std::cout << std::endl;
        }
    */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return KeyEvent(termUtils::getKeyCode(chars), chr);
}

void setEchoOn()
{
#ifdef __linux__
    termios term = termUtils::savedTerm;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
#endif
}

void setEchoOff()
{
#ifdef __linux__
    termios term = termUtils::savedTerm;
    term.c_lflag &= ~(termUtils::TERM_FLAGS);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
#endif
}

/**
 * @brief Console initialisation and save state
 */
void initConsole()
{
#ifdef __linux__
    tcgetattr(STDIN_FILENO, &termUtils::savedTerm);
    setEchoOff();
#endif

#ifdef _WIN32
    DWORD outMode = 0;
    DWORD inMode = 0;

    termUtils::stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    termUtils::stdinHandle = GetStdHandle(STD_INPUT_HANDLE);

    if(termUtils::stdoutHandle == INVALID_HANDLE_VALUE || termUtils::stdinHandle == INVALID_HANDLE_VALUE) {
        exit(GetLastError());
    }

    if(!GetConsoleMode(termUtils::stdoutHandle, &outMode) || !GetConsoleMode(termUtils::stdinHandle, &inMode)) {
        exit(GetLastError());
    }

    termUtils::outModeInit = outMode;
    termUtils::inModeInit = inMode;

    // Enable ANSI escape codes
    outMode |= termUtils::ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    // Set stdin as no echo and unbuffered
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

    if(!SetConsoleMode(termUtils::stdoutHandle, outMode) || !SetConsoleMode(termUtils::stdinHandle, inMode)) {
        exit(GetLastError());
    }

    GetCurrentConsoleFontEx(termUtils::stdoutHandle, false, &termUtils::cfiOld);

    CONSOLE_FONT_INFOEX cfi = { sizeof(cfi) };
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;  // Width of each character in the font
    cfi.dwFontSize.Y = 18; // Height
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    std::wcscpy(cfi.FaceName, L"Cascadia Mono"); // Choose your font
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
    // SetConsoleOutputCP(CP_UTF8);
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::cout << "\x1b[2J" << std::flush;
}

/**
 * @brief Restore saved console
 */
void restoreConsole(void)
{
    std::cout << RESET << std::flush;

#ifdef _WIN32
    // Reset console mode
    if(!SetConsoleMode(termUtils::stdoutHandle, termUtils::outModeInit) ||
        !SetConsoleMode(termUtils::stdinHandle, termUtils::inModeInit)) {
        exit(GetLastError());
    }
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &termUtils::cfiOld);
#endif
#ifdef __linux__
    tcsetattr(STDIN_FILENO, TCSANOW, &termUtils::savedTerm);
#endif
}

namespace clear
{
    /** @brief Clear all the line code */
    constexpr std::string_view LINE("\x1b[2K");
    /** @brief Clear line to end of line code */
    constexpr std::string_view LINE_TO_RIGHT("\x1b[0K");
    /** @brief Clear line to start of line code */
    constexpr std::string_view LINE_TO_LEFT("\x1b[1K");
    /** @brief Clear screen code */
    constexpr std::string_view SCREEN("\x1b[J");
    /** @brief Clear all screen code */
    constexpr std::string_view ALL_SCREEN("\x1b[2J");
    /** @brief Clear screen from cursor to bottom */
    constexpr std::string_view SCREEN_TO_BOTTOM("\x1b[0J");
    /** @brief Clear screen from cursor to top */
    constexpr std::string_view SCREEN_TO_TOP("\x1b[1J");

    /**
     * @brief Clear line from cursor position
     */
    void line(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << LINE;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear line from cursor position to right
     */
    void lineToRight(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << LINE_TO_RIGHT;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear line from cursor position left
     */
    void lineToLeft(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << LINE_TO_LEFT;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear screen from cursor position
     */
    void screen(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << SCREEN;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear screen from cursor position
     */
    void allScreen(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << ALL_SCREEN;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear screen from cursor position to bottom of screen
     */
    void screenToBottom(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << SCREEN_TO_BOTTOM;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Clear screen from cursor position to top of screen
     */
    void screenToTop(std::ostream& os = std::cout, const bool flush = true)
    {
        std::cout << SCREEN_TO_TOP;
        if(flush) {
            os << std::flush;
        }
    }

}

namespace color
{

    constexpr std::string_view RESET("\x1b[0m");
    /** @brief Black background */
    constexpr std::string_view BG_BLACK("\x1b[40;1m");
    /** @brief Red background */
    constexpr std::string_view BG_RED("\x1b[41;1m");
    /** @brief Green background */
    constexpr std::string_view BG_GREEN("\x1b[42;1m");
    /** @brief Yellow background */
    constexpr std::string_view BG_YELLOW("\x1b[43;1m");
    /** @brief Blue background */
    constexpr std::string_view BG_BLUE("\x1b[44;1m");
    /** @brief Magenta background */
    constexpr std::string_view BG_MAGENTA("\x1b[45;1m");
    /** @brief Cyan background */
    constexpr std::string_view BG_CYAN("\x1b[46;1m");
    /** @brief White background */
    constexpr std::string_view BG_WHITE("\x1b[47;1m");

    /** @brief Black foreground */
    constexpr std::string_view FG_BLACK("\x1b[30;1m");
    /** @brief Red foreground */
    constexpr std::string_view FG_RED("\x1b[31;1m");
    /** @brief Green foreground */
    constexpr std::string_view FG_GREEN("\x1b[32;1m");
    /** @brief Yellow foreground */
    constexpr std::string_view FG_YELLOW("\x1b[33;1m");
    /** @brief Blue foreground */
    constexpr std::string_view FG_BLUE("\x1b[34;1m");
    /** @brief Magenta foreground */
    constexpr std::string_view FG_MAGENTA("\x1b[35;1m");
    /** @brief Cyan foreground */
    constexpr std::string_view FG_CYAN("\x1b[36;1m");
    /** @brief White foreground */
    constexpr std::string_view FG_WHITE("\x1b[37;1m");
    /** @brief Foreground color from 0 to 254 */
    template <unsigned char color> constexpr std::string FG("\x1b[38;5;" + std::to_string(color) + "m");
    /** @brief Background color from 0 to 254 */
    template <unsigned char color> constexpr std::string BG("\x1b[48;5;" + std::to_string(color) + "m");
    /** @brief Backgrouund RGB color from 0 to 254 for each componant*/
    template <unsigned char r, unsigned char g, unsigned char b>
    constexpr std::string BG_RGB(
        "\x1B[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m");
    /** @brief Foregrouund RGB color from 0 to 254 for each componant*/
    template <unsigned char r, unsigned char g, unsigned char b>
    constexpr std::string FG_RGB(
        "\x1B[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m");

    /**
     * @brief Set foreground color
     * @param color A short between 0 and 254
     */
    void fg(const unsigned char color, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[38;5;" + std::to_string(color) + "m";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Set background color
     * @param color A short between 0 and 254
     */
    void bg(const unsigned char color, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[48;5;" + std::to_string(color) + "m";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Set foreground color using RGB
     * @param r, g, b A short between 0 and 254
     */
    void fgRgb(const unsigned char r,
        const unsigned char g,
        const unsigned char b,
        std::ostream& os = std::cout,
        const bool flush = true)
    {
        os << "\x1B[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Set background color using RGB
     * @param r, g, b A short between 0 and 254
     */
    void bgRgb(const unsigned char r,
        const unsigned char g,
        const unsigned char b,
        std::ostream& os = std::cout,
        const bool flush = true)
    {
        os << "\x1B[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        if(flush) {
            os << std::flush;
        }
    }

    void reset(std::ostream& os = std::cout, const bool flush = true)
    {
        os << RESET;
        if(flush) {
            os << std::flush;
        }
    }

}

namespace cursor
{
    /** @brief Set cursor on */
    constexpr std::string_view ON("\x1b[?25h");
    /** @brief Set cursor off */
    constexpr std::string_view OFF("\x1b[?25l");
    /** @brief Move cursor to row and col */
    template <unsigned int row, unsigned int col>
    const std::string MOVE("\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H");
    /** @brief Move cursor up */
    template <unsigned int offset = 1> constexpr std::string MOVE_UP("\x1b[" + std::to_string(offset) + "A");
    /** @brief Move cursor down */
    template <unsigned int offset = 1> constexpr std::string MOVE_DOWN("\x1b[" + std::to_string(offset) + "B");
    /** @brief Move cursor right */
    template <unsigned int offset = 1> constexpr std::string MOVE_RIGHT("\x1b[" + std::to_string(offset) + "C");
    /** @brief Move cursor left */
    template <unsigned int offset = 1> constexpr std::string MOVE_LEFT("\x1b[" + std::to_string(offset) + "D");
    /** @brief Move cursor tol column col */
    template <unsigned int col> constexpr std::string MOVE_TO_COL("\x1b[" + std::to_string(col) + "G");
    /** @brief Move cursor to top left corner */
    const std::string_view ORIGIN("\x1b[H");
    /** @brief Save cursor position */
    const std::string_view SAVE("\x1b[s");
    /** @brief Restore a saved cursor position */
    const std::string_view RESTORE("\x1b[u");

    /**
     * @brief Show cursor
     */
    void on(std::ostream& os = std::cout, const bool flush = true)
    {
        os << ON;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Hide cursor
     */
    void off(std::ostream& os = std::cout, const bool flush = true)
    {
        os << OFF;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor to specified position
     */
    void move(const int row, const int col, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor up by offset
     */
    void moveUp(const int offset = 1, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(offset) + "A";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor down by offset
     */
    void moveDown(const int offset = 1, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(offset) + "B";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor right by offset
     */
    void moveRight(const int offset = 1, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(offset) + "C";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor left by offset
     */
    void moveLeft(const int offset = 1, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(offset) + "D";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor left by offset
     */
    void moveToCol(const int col, std::ostream& os = std::cout, const bool flush = true)
    {
        os << "\x1b[" + std::to_string(col) + "G";
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Move cursor to position 0,0
     */
    void origin(std::ostream& os = std::cout, const bool flush = true)
    {
        os << ORIGIN;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Save cursor position
     */
    void save(std::ostream& os = std::cout, const bool flush = true)
    {
        os << SAVE;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Restore cursor position
     */
    void restore(std::ostream& os = std::cout, const bool flush = true)
    {
        os << RESTORE;
        if(flush) {
            os << std::flush;
        }
    }

    /**
     * @brief Get position of cursor
     * @return @Pos
     */
    Pos position()
    {
        /*
                termios term;
                termios curTerm;
                tcgetattr(STDIN_FILENO, &curTerm);

                term = termUtils::savedTerm;
                term.c_lflag &= ~(termUtils::TERM_FLAGS);
                tcsetattr(0, TCSANOW, &term);
        */
        std::cout << "\x1b[6n" << std::flush;
        char buff[32] = { 0 };
        int indx = 0;
        while(true) {
            int cc = getchar();
            buff[indx] = static_cast<char>(cc);
            indx++;
            if(cc == 'R') {
                buff[indx + 1] = '\0';
                break;
            }
        }
        indx = 0;
        std::string str = "";
        char chr = buff[++indx];
        while(chr != 'R') {
            if(std::find(termUtils::CHARS.begin(), termUtils::CHARS.end(), chr) != termUtils::CHARS.end()) {
                str += chr;
            }
            chr = buff[++indx];
        }
        //        tcsetattr(0, TCSANOW, &curTerm);
        std::size_t pos = str.find(";");
        return Pos(std::stoi(str.substr(0, pos)), std::stoi(str.substr(pos + 1)));
    }
}

namespace style
{
    /** @brief Bright code */
    constexpr std::string_view BRIGHT("\x1b[1m");
    /** @brief Dim code */
    constexpr std::string_view DIM("\x1b[2m");
    /** @brief Underscore code */
    constexpr std::string_view UNDERSCORE("\x1b[4m");
    /** @brief Blink code */
    constexpr std::string_view BLINK("\x1b[5m");
    /** @brief Reverse code */
    constexpr std::string_view REVERSE("\x1b[7m");
}

}

#endif // TERM_HPP
