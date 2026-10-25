// Time.cpp - TimePoint conversion and formatting.
#include "domain/Time.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace lms {

std::int64_t toUnix(TimePoint t) {
    return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
}

TimePoint fromUnix(std::int64_t seconds) {
    return TimePoint(std::chrono::seconds(seconds));
}

namespace {
std::tm toLocalTm(std::time_t t) {
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &t);
#else
    localtime_r(&t, &result);
#endif
    return result;
}
}  // namespace

std::string formatTime(TimePoint t) {
    std::tm local = toLocalTm(std::chrono::system_clock::to_time_t(t));
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M");
    return out.str();
}

std::optional<TimePoint> parseDateTime(const std::string& text) {
    std::tm tm{};
    std::istringstream in(text);
    in >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    if (in.fail()) {
        return std::nullopt;
    }
    // Reject extra characters after the time, e.g. "2026-10-30 10:00abc".
    in >> std::ws;
    if (!in.eof()) {
        return std::nullopt;
    }
    // Basic range checks, because get_time accepts some odd values.
    if (tm.tm_mon < 0 || tm.tm_mon > 11 || tm.tm_mday < 1 || tm.tm_mday > 31 || tm.tm_hour > 23 ||
        tm.tm_min > 59) {
        return std::nullopt;
    }
    const int requestedDay = tm.tm_mday;
    tm.tm_isdst = -1;  // let the C library work out daylight saving
    std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1) || tm.tm_mday != requestedDay) {
        return std::nullopt;  // e.g. 31 February rolls over to March
    }
    return std::chrono::system_clock::from_time_t(t);
}

}  // namespace lms
