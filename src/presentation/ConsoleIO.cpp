// ConsoleIO.cpp
#include "presentation/ConsoleIO.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "domain/Time.h"

#ifdef _WIN32
#ifndef NOMINMAX  // MinGW already defines it
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace lms::io {

std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosed();
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();  // Windows line ending in redirected input
    }
    return line;
}

std::string readNonEmpty(const std::string& prompt) {
    while (true) {
        std::string line = readLine(prompt);
        if (line.find_first_not_of(" \t") != std::string::npos) {
            return line;
        }
        error("This cannot be blank.");
    }
}

namespace {
bool parseInt(const std::string& text, int& value) {
    std::istringstream in(text);
    in >> value;
    if (in.fail()) {
        return false;
    }
    in >> std::ws;
    return in.eof();  // no extra characters such as "12abc"
}
}  // namespace

int readInt(const std::string& prompt, int min, int max) {
    while (true) {
        int value = 0;
        if (parseInt(readLine(prompt), value) && value >= min && value <= max) {
            return value;
        }
        error("Please enter a whole number from " + std::to_string(min) + " to " +
              std::to_string(max) + ".");
    }
}

std::optional<int> readOptionalInt(const std::string& prompt, int min, int max) {
    while (true) {
        const std::string line = readLine(prompt);
        if (line.find_first_not_of(" \t") == std::string::npos) {
            return std::nullopt;
        }
        int value = 0;
        if (parseInt(line, value) && value >= min && value <= max) {
            return value;
        }
        error("Please enter a whole number from " + std::to_string(min) + " to " +
              std::to_string(max) + ", or leave it blank.");
    }
}

bool readYesNo(const std::string& prompt) {
    while (true) {
        const std::string answer = readLine(prompt + " (y/n): ");
        if (answer == "y" || answer == "Y") return true;
        if (answer == "n" || answer == "N") return false;
        error("Please type y or n.");
    }
}

namespace {
// Turns off echo while a password is typed and always turns it back on, even if an
// exception is thrown (RAII, Coding Standard D6). Does nothing when input is not a console.
class EchoOff {
public:
    EchoOff()
#ifdef _WIN32
        : in_(GetStdHandle(STD_INPUT_HANDLE))
#endif
    {
#ifdef _WIN32
        active_ = GetConsoleMode(in_, &oldMode_) != 0;
        if (active_) SetConsoleMode(in_, oldMode_ & ~ENABLE_ECHO_INPUT);
#else
        active_ = isatty(STDIN_FILENO) != 0 && tcgetattr(STDIN_FILENO, &oldTerm_) == 0;
        if (active_) {
            termios noEcho = oldTerm_;
            noEcho.c_lflag &= ~static_cast<tcflag_t>(ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &noEcho);
        }
#endif
    }
    ~EchoOff() {
        if (!active_) return;
#ifdef _WIN32
        SetConsoleMode(in_, oldMode_);
#else
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm_);
#endif
        std::cout << '\n';  // the Enter key was not echoed either
    }
    EchoOff(const EchoOff&) = delete;
    EchoOff& operator=(const EchoOff&) = delete;

private:
    bool active_ = false;
#ifdef _WIN32
    HANDLE in_ = nullptr;
    DWORD oldMode_ = 0;
#else
    termios oldTerm_{};
#endif
};
}  // namespace

// Hides what is typed when the program runs in a real console window.
std::string readPassword(const std::string& prompt) {
    EchoOff hidden;
    return readLine(prompt);
}

TimePoint readDateTime(const std::string& prompt) {
    while (true) {
        const auto t = parseDateTime(readLine(prompt + " (YYYY-MM-DD HH:MM): "));
        if (t.has_value()) {
            return *t;
        }
        error("That is not a valid date and time. Example: 2026-10-30 14:00");
    }
}

void heading(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void info(const std::string& message) {
    std::cout << message << '\n';
}

void error(const std::string& message) {
    // Marked with words, not only colour (NFR-10).
    std::cout << "ERROR: " << message << '\n';
}

std::string col(const std::string& text, std::size_t width) {
    if (text.size() >= width) {
        return text.substr(0, width - 1) + " ";
    }
    return text + std::string(width - text.size(), ' ');
}

std::string orDash(const std::optional<int>& value) {
    return value.has_value() ? std::to_string(*value) : "-";
}

std::string oneDecimal(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

}  // namespace lms::io
