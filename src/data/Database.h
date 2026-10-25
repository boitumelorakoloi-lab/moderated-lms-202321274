// Database.h - thin RAII wrapper around the SQLite C API.
// This folder (src/data) is the ONLY place in the program that contains SQL (NFR-11).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "data/Repositories.h"
#include "domain/Types.h"

struct sqlite3;
struct sqlite3_stmt;

namespace lms {

class Database final : public IBackupStore {
public:
    // Opens (or creates) the database file and creates the tables on first use.
    explicit Database(const std::string& path);
    ~Database() override;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() { return db_; }
    std::int64_t lastInsertId() const;

    // Only for fixed SQL with no user input (schema and PRAGMAs).
    void executeScript(const std::string& sql);

    void backupTo(const std::string& path) override;
    void restoreFrom(const std::string& path) override;

private:
    void configure();
    void ensureSchema();

    sqlite3* db_ = nullptr;
};

// A prepared statement. Values are always bound as parameters (?1, ?2 ...), never
// pasted into the SQL text, so SQL injection is not possible (NFR-05).
class Statement {
public:
    Statement(Database& db, const char* sql);
    ~Statement();
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement& bind(int index, int value);
    Statement& bind(int index, std::int64_t value);
    Statement& bind(int index, const std::string& value);
    Statement& bind(int index, const char* value);
    Statement& bindNull(int index);
    Statement& bindOptional(int index, const std::optional<int>& value);
    Statement& bindOptional(int index, const std::optional<std::string>& value);
    Statement& bindTime(int index, TimePoint value);
    Statement& bindOptionalTime(int index, const std::optional<TimePoint>& value);

    bool step();  // true when a row is ready to read
    void run();   // runs a statement that returns no rows

    int getInt(int col) const;
    std::int64_t getInt64(int col) const;
    std::string getText(int col) const;
    bool isNull(int col) const;
    std::optional<int> getOptionalInt(int col) const;
    std::optional<std::string> getOptionalText(int col) const;
    TimePoint getTime(int col) const;
    std::optional<TimePoint> getOptionalTime(int col) const;

private:
    Database& db_;
    sqlite3_stmt* stmt_ = nullptr;
};

// BEGIN IMMEDIATE ... COMMIT. If commit() is not reached (an exception was thrown),
// the destructor rolls everything back, so a failed save leaves no half-written data (NFR-07).
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit();

private:
    Database& db_;
    bool done_ = false;
};

}  // namespace lms
