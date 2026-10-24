// ILatePenaltyPolicy.h - Strategy pattern interface for the late-penalty rule (FR-15).
// A new rule can be added as a new class without changing any service (open/closed).
#pragma once

#include <string>

#include "domain/Types.h"

namespace lms {

class ILatePenaltyPolicy {
public:
    virtual ~ILatePenaltyPolicy() = default;

    // Number of days late (BR-02). 0 means on time.
    virtual int daysLate(TimePoint submittedAt, TimePoint deadline) const = 0;

    // Final mark after the penalty (BR-01).
    virtual double finalMark(int raw, int maxMark, int daysLate) const = 0;

    // Stored in mark.policy_code so old marks keep the rule they were worked out with.
    virtual std::string code() const = 0;
};

}  // namespace lms
