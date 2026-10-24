// Time.h - helpers for converting and showing TimePoints.
// The database stores times as INTEGER Unix seconds (SDD 5.3).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "domain/Types.h"

namespace lms {

std::int64_t toUnix(TimePoint t);
TimePoint fromUnix(std::int64_t seconds);

// "2026-10-24 21:30" in the host computer's local time.
std::string formatTime(TimePoint t);

// Reads "YYYY-MM-DD HH:MM" (local time). Returns nullopt if the text is not valid.
std::optional<TimePoint> parseDateTime(const std::string& text);

}  // namespace lms
