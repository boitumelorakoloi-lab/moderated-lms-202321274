// Assignment.h
#pragma once

#include <optional>
#include <string>

#include "domain/Entities.h"
#include "domain/Types.h"

namespace lms {

class Assignment {
public:
    Assignment(int id, int courseId, std::string title, TimePoint deadline, int maxMark,
               int createdBy, std::optional<TimePoint> releasedAt = std::nullopt);

    // The student's extended deadline if they have one, otherwise the base deadline.
    TimePoint effectiveDeadline(const std::optional<Extension>& ext) const;

    int id() const { return id_; }
    int courseId() const { return courseId_; }
    const std::string& title() const { return title_; }
    TimePoint deadline() const { return deadline_; }
    int maxMark() const { return maxMark_; }
    int createdBy() const { return createdBy_; }
    const std::optional<TimePoint>& releasedAt() const { return releasedAt_; }

private:
    int id_;
    int courseId_;
    std::string title_;
    TimePoint deadline_;
    int maxMark_;
    int createdBy_;
    std::optional<TimePoint> releasedAt_;
};

}  // namespace lms
