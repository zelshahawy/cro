#pragma once
#include <string>
struct sqlite3; // fwd decl from SQLite C API

namespace cro::db
{
	struct Handle
	{
		sqlite3 *raw{};
		~Handle();
		Handle() = default;
		Handle(Handle &&) noexcept;
		Handle &operator=(Handle &&) noexcept;
		Handle(const Handle &) = delete;
		Handle &operator=(const Handle &) = delete;
	};

	Handle open(const std::string &path);
	void migrate(sqlite3 *db);
}
