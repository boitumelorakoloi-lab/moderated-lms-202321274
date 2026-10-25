// TenPercentPerDayPolicy.h - the current late rule agreed with the HoD (BR-01, BR-02).
#pragma once

#include "domain/ILatePenaltyPolicy.h"

namespace lms {

class TenPercentPerDayPolicy final : public ILatePenaltyPolicy {
public:
    int daysLate(TimePoint submittedAt, TimePoint deadline) const override;
    double finalMark(int raw, int maxMark, int daysLate) const override;
    std::string code() const override { return "TEN_PCT_PER_DAY_V1"; }

private:
    double ratePerDay_ = 0.10;
    int maxDays_ = 3;
};

}  // namespace lms
