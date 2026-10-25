// MarkView.cpp
#include "services/MarkView.h"

namespace lms {

MarkView buildMarkView(const Submission& submission, const Assignment& assignment,
                       const std::optional<Extension>& extension, const std::optional<Mark>& mark,
                       const ILatePenaltyPolicy& policy) {
    MarkView view{submission, mark, extension, assignment.effectiveDeadline(extension)};
    view.maxMark = assignment.maxMark();
    if (!submission.versions().empty()) {
        view.daysLate = policy.daysLate(submission.latest().submittedAt(), view.effectiveDeadline);
    }
    if (mark.has_value()) {
        view.finalMark = policy.finalMark(mark->rawMark(), assignment.maxMark(), view.daysLate);
        view.penalty = mark->rawMark() - view.finalMark;
    }
    return view;
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool isBlank(const std::string& text) {
    return trim(text).empty();
}

}  // namespace lms
