// ConsoleIO.cpp
#include "presentation/ConsoleIO.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "domain/Time.h"

#ifdef _WIN32
#define NOMINMAX
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

// Hides what is typed when the program runs in a real console window.
std::string readPassword(const std::string& prompt) {
#ifdef _WIN32
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode = 0;
    const bool isConsole = GetConsoleMode(in, &oldMode) != 0;
    if (isConsole) SetConsoleMode(in, oldMode & ~ENABLE_ECHO_INPUT);
#else
    termios oldTerm{};
    const bool isConsole = isatty(STDIN_FILENO) != 0 && tcgetattr(STDIN_FILENO, &oldTerm) == 0;
    if (isConsole) {
        termios noEcho = oldTerm;
        noEcho.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &noEcho);
    }
#endif
    std::string password;
    try {
        password = readLine(prompt);
    } catch (...) {
#ifdef _WIN32
        if (isConsole) SetConsoleMode(in, oldMode);
#else
        if (isConsole) tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
#endif
        throw;
    }
#ifdef _WIN32
    if (isConsole) SetConsoleMode(in, oldMode);
#else
    if (isConsole) tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
#endif
    if (isConsole) std::cout << '\n';
    return password;
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

std::string oneDecimal(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

}  // namespace lms::io
