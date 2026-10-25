// Assignment.cpp
#include "domain/Assignment.h"

#include <utility>

namespace lms {

Assignment::Assignment(int id, int courseId, std::string title, TimePoint deadline, int maxMark,
                       int createdBy, std::optional<TimePoint> releasedAt)
    : id_(id),
      courseId_(courseId),
      title_(std::move(title)),
      deadline_(deadline),
      maxMark_(maxMark),
      createdBy_(createdBy),
      releasedAt_(releasedAt) {}

TimePoint Assignment::effectiveDeadline(const std::optional<Extension>& ext) const {
    if (ext.has_value()) {
        return ext->newDeadline();
    }
    return deadline_;
}

}  // namespace lms
