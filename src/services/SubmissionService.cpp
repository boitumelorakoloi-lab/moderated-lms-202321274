// SubmissionService.cpp - follows the "Submit assignment" sequence diagram (SDD Figure 5).
#include "services/SubmissionService.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <utility>

#include "domain/Exceptions.h"
#include "domain/Time.h"

namespace fs = std::filesystem;

namespace lms {

SubmissionService::SubmissionService(ICourseRepository& courses, ISubmissionRepository& submissions,
                                     IMarkRepository& marks, const ILatePenaltyPolicy& policy,
                                     std::string storageFolder)
    : courses_(courses),
      submissions_(submissions),
      marks_(marks),
      policy_(policy),
      storageFolder_(std::move(storageFolder)) {}

// UC-18 / FR-13: the file must exist, be 1 byte to 10 MB, and be .pdf, .docx, .txt or .zip.
void SubmissionService::validateFile(const std::string& path) const {
    const fs::path file(trim(path));
    std::error_code ec;
    if (trim(path).empty() || !fs::is_regular_file(file, ec)) {
        throw ValidationException("The file was not found: " + trim(path));
    }
    const auto size = fs::file_size(file, ec);
    if (ec) {
        throw ValidationException("The file could not be read.");
    }
    if (size == 0) {
        throw ValidationException("The file is empty.");
    }
    if (size > static_cast<std::uintmax_t>(kMaxFileBytes)) {
        throw ValidationException("The file is bigger than 10 MB.");
    }
    std::string ext = file.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext != ".pdf" && ext != ".docx" && ext != ".txt" && ext != ".zip") {
        throw ValidationException("Only .pdf, .docx, .txt and .zip files are accepted.");
    }
}

CheckResult SubmissionService::check(const Student& s, int assignmentId, const std::string& path,
                                     TimePoint now) {
    if (!s.can(Permission::SubmitWork)) {
        throw AccessDeniedException("Only students can submit work.");
    }
    const auto assignment = courses_.findAssignment(assignmentId);
    if (!assignment) {
        throw ValidationException("There is no assignment with ID " + std::to_string(assignmentId) + ".");
    }
    if (!courses_.isEnrolled(s.id(), assignment->courseId())) {
        throw AccessDeniedException("You are not enrolled in the course for this assignment.");
    }
    const auto ext = courses_.findExtension(assignmentId, s.id());

    CheckResult r;
    r.deadline = assignment->effectiveDeadline(ext);

    validateFile(path);  // E1: throws, nothing saved

    const auto existing = submissions_.find(assignmentId, s.id());
    if (existing) {
        r.nextVersion = existing->latest().versionNo() + 1;
    }

    if (assignment->releasedAt().has_value()) {
        r.result = SubmissionCheck::Rejected;
        r.message = "Marks for this assignment have already been released.";
    } else if (existing && existing->status() != SubmissionStatus::Submitted) {
        r.result = SubmissionCheck::Rejected;
        r.message = existing->status() == SubmissionStatus::Late
                        ? "Late work can only be handed in once - resubmission not allowed."
                        : "This work has already been marked - resubmission not allowed.";
    } else if (now <= r.deadline && !existing) {
        r.result = SubmissionCheck::OnTime;
        r.message = "On time.";
    } else if (now <= r.deadline && existing) {
        r.result = SubmissionCheck::Resubmit;  // A1: new version replaces the old one for marking
        r.message = "This will replace your earlier file (version " +
                    std::to_string(r.nextVersion - 1) + ") for marking.";
    } else if (!existing) {
        r.result = SubmissionCheck::Late;  // A2: late work is accepted (C3)
        r.daysLate = policy_.daysLate(now, r.deadline);
        r.message = "The deadline has passed. This work will be " + std::to_string(r.daysLate) +
                    " day(s) late and will lose marks (10% of the maximum per day, 0 after 3 days).";
    } else {
        r.result = SubmissionCheck::Rejected;  // E2
        r.message = "Deadline passed - resubmission not allowed.";
    }
    return r;
}

int SubmissionService::submit(const Student& s, int assignmentId, const std::string& path,
                              TimePoint now) {
    // Check again with the time of confirmation, so a slow confirm cannot turn late work
    // into on-time work (SDD 4.3).
    const CheckResult r = check(s, assignmentId, path, now);
    if (r.result == SubmissionCheck::Rejected) {
        throw ValidationException(r.message);
    }

    // Copy the file into the submissions/ folder with a name that cannot clash.
    const fs::path source(trim(path));
    const std::string storedName = "a" + std::to_string(assignmentId) + "_s" + std::to_string(s.id()) +
                                   "_v" + std::to_string(r.nextVersion) + "_" +
                                   std::to_string(toUnix(now)) + source.extension().string();
    const fs::path target = fs::path(storageFolder_) / storedName;
    std::error_code ec;
    fs::create_directories(storageFolder_, ec);
    if (!fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec)) {
        throw ValidationException("The file could not be copied: " + ec.message());
    }
    const auto size = static_cast<std::int64_t>(fs::file_size(target, ec));
    const SubmissionVersion version(r.nextVersion, target.generic_string(),
                                    source.filename().string(), size, now);

    try {
        if (r.result == SubmissionCheck::Resubmit) {
            const auto existing = submissions_.find(assignmentId, s.id());
            submissions_.addVersion(existing->id(), version);
            return existing->id();
        }
        Submission sub(0, assignmentId, s.id(),
                       r.result == SubmissionCheck::Late ? SubmissionStatus::Late
                                                         : SubmissionStatus::Submitted);
        sub.addVersion(version);
        return submissions_.addSubmission(sub);
    } catch (const DatabaseException&) {
        fs::remove(target, ec);  // E3: undo the copy as well, then tell the student
        throw;
    }
}

std::vector<StudentResult> SubmissionService::myResults(const Student& s) {
    if (!s.can(Permission::ViewOwnMarks)) {
        throw AccessDeniedException("Only students can view their own marks.");
    }
    std::vector<StudentResult> results;
    for (const Assignment& a : courses_.listAssignmentsFor(s.id())) {
        const auto sub = submissions_.find(a.id(), s.id());
        // FR-20: only released marks are shown, and only the student's own (NFR-08).
        if (!sub || sub->status() != SubmissionStatus::Released) {
            continue;
        }
        results.push_back({a, buildMarkView(*sub, a, courses_.findExtension(a.id(), s.id()),
                                            marks_.find(sub->id()), policy_)});
    }
    return results;
}

}  // namespace lms
