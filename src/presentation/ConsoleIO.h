// ConsoleIO.h - safe keyboard input for the menus. Every read checks the input and asks
// again on bad values, so typing rubbish never crashes the program (NFR-07, NFR-10).
#pragma once

#include <optional>
#include <stdexcept>
#include <string>

#include "domain/Types.h"

namespace lms::io {

// Thrown when the input stream ends (e.g. Ctrl+D / Ctrl+Z). ConsoleApp exits cleanly.
class InputClosed : public std::runtime_error {
public:
    InputClosed() : std::runtime_error("input closed") {}
};

std::string readLine(const std::string& prompt);
std::string readNonEmpty(const std::string& prompt);
int readInt(const std::string& prompt, int min, int max);
std::optional<int> readOptionalInt(const std::string& prompt, int min, int max);  // blank = none
bool readYesNo(const std::string& prompt);
std::string readPassword(const std::string& prompt);
TimePoint readDateTime(const std::string& prompt);

void heading(const std::string& title);
void info(const std::string& message);
void error(const std::string& message);

// Fixed-width text column, used by the list screens.
std::string col(const std::string& text, std::size_t width);
std::string oneDecimal(double value);

}  // namespace lms::io
