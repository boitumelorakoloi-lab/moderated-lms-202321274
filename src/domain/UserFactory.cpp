// UserFactory.cpp
#include "domain/UserFactory.h"

namespace lms {

std::unique_ptr<User> UserFactory::create(Role role, int id, const std::string& username,
                                          const std::string& fullName, const std::string& hash,
                                          int deptId) {
    switch (role) {
        case Role::Student:
            return std::make_unique<Student>(id, username, fullName, hash);
        case Role::Lecturer:
            return std::make_unique<Lecturer>(id, username, fullName, hash);
        case Role::HeadOfDepartment:
            return std::make_unique<HeadOfDepartment>(id, username, fullName, hash, deptId);
        case Role::Administrator:
            return std::make_unique<Administrator>(id, username, fullName, hash);
    }
    return nullptr;
}

}  // namespace lms
