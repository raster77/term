#ifndef TERMUTILS_HPP
#define TERMUTILS_HPP

#include <unordered_map>
#include <vector>

#ifdef __linux__
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "keys.hpp"

namespace termUtils
{
#ifdef __linux__
//static constexpr tcflag_t TERM_FLAGS = ICANON | ECHO | ISIG | IEXTEN | ICRNL | IXON | IUTF8;
static constexpr tcflag_t TERM_FLAGS = ICANON | ECHO | ISIG | IEXTEN | ICRNL | IXON | IUTF8;
#endif
static const std::vector<char> CHARS({ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ';' });

#ifdef _WIN32
static constexpr unsigned char ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004;
static HANDLE stdoutHandle;
static HANDLE stdinHandle;
static DWORD outModeInit;
static DWORD inModeInit;
static CONSOLE_FONT_INFOEX cfiOld = { sizeof(cfiOld) };
#endif

#ifdef __linux__
termios savedTerm;
static constexpr int STDIN = 0;
static bool initialized = false;
#endif

/**
 * @brief Get key pressed
 * @return bool if a key was pressed or noti
 */
int kbHit()
{
#ifdef _WIN32
    return _kbhit();
#endif

#ifdef __linux__
    if(!initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
#endif
}

/** @brief Key code when key pressed */
term::Key getKeyCode(const std::vector<int>& buf)
{
    if(buf.empty()) {
        return term::Key::None;
    }
    if(buf.size() == 1) {
        switch(buf[0]) {
#ifdef _WIN32
        case 8:
            return term::Key::Backspace;
#endif
        case 9:
            return term::Key::Tab;
        case 10:
            return term::Key::Enter;
        case 27:
            return term::Key::Escape;
        case 32:
            return term::Key::Space;
        case 43:
            return term::Key::Add;
        case 45:
            return term::Key::Subtsract;
        case 46:
            return term::Key::Point;
        case 47:
            return term::Key::Slash;
        case 48:
            return term::Key::Num0;
        case 49:
            return term::Key::Num1;
        case 50:
            return term::Key::Num2;
        case 51:
            return term::Key::Num3;
        case 52:
            return term::Key::Num4;
        case 53:
            return term::Key::Num5;
        case 54:
            return term::Key::Num6;
        case 55:
            return term::Key::Num7;
        case 56:
            return term::Key::Num8;
        case 57:
            return term::Key::Num9;
        case 58:
            return term::Key::Slash;
        case 97:
        case 65:
            return term::Key::A;
        case 98:
        case 66:
            return term::Key::B;
        case 99:
        case 67:
            return term::Key::C;
        case 100:
        case 68:
            return term::Key::D;
        case 101:
        case 69:
            return term::Key::E;
        case 102:
        case 70:
            return term::Key::F;
        case 103:
        case 71:
            return term::Key::G;
        case 104:
        case 72:
            return term::Key::H;
        case 105:
        case 73:
            return term::Key::I;
        case 106:
        case 74:
            return term::Key::J;
        case 107:
        case 75:
            return term::Key::K;
        case 108:
        case 76:
            return term::Key::L;
        case 109:
        case 77:
            return term::Key::M;
        case 110:
        case 78:
            return term::Key::N;
        case 111:
        case 79:
            return term::Key::O;
        case 112:
        case 80:
            return term::Key::P;
        case 113:
        case 81:
            return term::Key::Q;
        case 114:
        case 82:
            return term::Key::R;
        case 115:
        case 83:
            return term::Key::S;
        case 116:
        case 84:
            return term::Key::T;
        case 117:
        case 85:
            return term::Key::U;
        case 118:
        case 86:
            return term::Key::V;
        case 119:
        case 87:
            return term::Key::W;
        case 120:
        case 88:
            return term::Key::X;
        case 121:
        case 89:
            return term::Key::Y;
        case 122:
        case 90:
            return term::Key::Z;

#ifdef __linux__
        case 127:
            return term::Key::Backspace;
#endif
        }
    } else if(buf.size() == 2) {
#ifdef _WIN32
        if(buf[0] == 0) {
            switch(buf[1]) {

            case 59:
                return term::Key::F1;
            case 60:
                return term::Key::F2;
            case 61:
                return term::Key::F3;
            case 62:
                return term::Key::F4;
            case 63:
                return term::Key::F5;
            case 64:
                return term::Key::F6;
            case 65:
                return term::Key::F7;
            case 66:
                return term::Key::F8;
            case 67:
                return term::Key::F9;
            case 68:
                return term::Key::F10;
            case 69:
                return term::Key::F11;
            }
        }
        if(buf[0] == 224) {
            switch(buf[1]) {
            case 72:
                return term::Key::Up;
            case 80:
                return term::Key::Down;
            case 77:
                return term::Key::Right;
            case 75:
                return term::Key::Left;
            case 134:
                return term::Key::F12;
            }
        }
#endif
    } else if(buf.size() == 3) {
#ifdef __linux__
        if(buf[0] == 27) {
            if(buf[1] == 91) {
                switch(buf[2]) {
                case 65:
                    return term::Key::Up;
                case 66:
                    return term::Key::Down;
                case 67:
                    return term::Key::Right;
                case 68:
                    return term::Key::Left;
                }
            } else if(buf[1] == 79) {
                switch(buf[2]) {
                case 80:
                    return term::Key::F1;
                case 81:
                    return term::Key::F2;
                case 82:
                    return term::Key::F3;
                case 83:
                    return term::Key::F4;
                }
            }
        }
#endif
    } else if(buf.size() == 5) {
#ifdef __linux__
        if(buf[0] == 27 && buf[1] == 91 && buf[4] == 126) {
            if(buf[2] == 49) {
                switch(buf[3]) {
                case 53:
                    return term::Key::F5;
                case 55:
                    return term::Key::F6;
                case 56:
                    return term::Key::F7;
                case 57:
                    return term::Key::F8;
                }
            } else if(buf[2] == 50) {
                switch(buf[3]) {
                case 48:
                    return term::Key::F9;
                case 49:
                    return term::Key::F10;
                case 51:
                    return term::Key::F11;
                case 52:
                    return term::Key::F12;
                }
            }
        }
#endif
    }

    return term::Key::Unknown;
}

};

#endif // TERMUTILS_HPP
