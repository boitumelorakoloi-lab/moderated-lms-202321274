// Exceptions.h - the exception classes used across the layers (SDD 3.1).
// All derive from LmsException so the menus can catch them in one place.
#pragma once

#include <stdexcept>
#include <string>

namespace lms {

class LmsException : public std::runtime_error {
public:
    explicit LmsException(const std::string& message) : std::runtime_error(message) {}
};

// Bad input from the user: blank text, number out of range, wrong file type...
class ValidationException : public LmsException {
public:
    using LmsException::LmsException;
};

// The user's role (or department) does not allow the action (FR-02, NFR-08).
class AccessDeniedException : public LmsException {
public:
    using LmsException::LmsException;
};

// A Submission was asked to move to a state the life cycle does not allow.
class InvalidTransitionException : public LmsException {
public:
    using LmsException::LmsException;
};

// Anything that went wrong inside SQLite. The transaction has been rolled back.
class DatabaseException : public LmsException {
public:
    using LmsException::LmsException;
};

}  // namespace lms
