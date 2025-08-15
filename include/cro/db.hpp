#pragma once
#include <string>
struct sqlite3; // fwd decl from SQLite C API

namespace cro::db
{
  struct Handle
  {
    sqlite3 *raw{};
    explicit Handle(sqlite3 *p) : raw(p) {} // <-- new: ctor taking sqlite3*
    ~Handle();
    Handle() = default;
    Handle(Handle &&) noexcept;
    Handle &operator=(Handle &&) noexcept;
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
  };

  Handle open(const std::string &path);
  void migrate(sqlite3 *db);
} // namespace cro::db
