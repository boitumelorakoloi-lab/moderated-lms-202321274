// TenPercentPerDayPolicy.cpp
#include "domain/TenPercentPerDayPolicy.h"

#include <algorithm>
#include <chrono>

namespace lms {

int TenPercentPerDayPolicy::daysLate(TimePoint submittedAt, TimePoint deadline) const {
    if (submittedAt <= deadline) {
        return 0;  // exactly at the deadline counts as on time
    }
    const auto secondsLate =
        std::chrono::duration_cast<std::chrono::seconds>(submittedAt - deadline).count();
    const long long secondsPerDay = 24LL * 60 * 60;
    // Each started 24-hour period counts as one day: 1 minute late = 1 day.
    return static_cast<int>((secondsLate + secondsPerDay - 1) / secondsPerDay);
}

double TenPercentPerDayPolicy::finalMark(int raw, int maxMark, int daysLate) const {
    if (daysLate <= 0) {
        return raw;
    }
    if (daysLate > maxDays_) {
        return 0.0;  // more than 3 days late
    }
    const double penalty = ratePerDay_ * maxMark * daysLate;
    return std::max(0.0, raw - penalty);  // never below 0
}

}  // namespace lms
