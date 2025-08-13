#include "cro/db.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <string>

namespace cro::db
{
	static void exec(sqlite3 *db, const char *sql)
	{
		char *err = nullptr;
		if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK)
		{
			std::string msg = err ? err : "unknown sqlite error";
			sqlite3_free(err);
			throw std::runtime_error("sqlite exec failed: " + msg);
		}
	}

	Handle::~Handle()
	{
		if (raw)
			sqlite3_close(raw);
	}
	Handle::Handle(Handle &&o) noexcept
	{
		raw = o.raw;
		o.raw = nullptr;
	}
	Handle &Handle::operator=(Handle &&o) noexcept
	{
		if (this != &o)
		{
			if (raw)
				sqlite3_close(raw);
			raw = o.raw;
			o.raw = nullptr;
		}
		return *this;
	}

	Handle open(const std::string &path)
	{
		sqlite3 *db = nullptr;
		if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
			throw std::runtime_error("sqlite3_open failed");
		return Handle{db};
	}

	void migrate(sqlite3 *db)
	{
		const char *sql =
				"PRAGMA foreign_keys = ON;"
				"CREATE TABLE IF NOT EXISTS foods("
				"id INTEGER PRIMARY KEY,"
				"name TEXT NOT NULL,"
				"kcal_per_100g REAL NOT NULL,"
				"protein_g_100g REAL NOT NULL,"
				"carbs_g_100g REAL NOT NULL,"
				"fat_g_100g REAL NOT NULL);"
				"CREATE TABLE IF NOT EXISTS intakes("
				"id INTEGER PRIMARY KEY,"
				"eaten_at TEXT NOT NULL,"
				"food_id INTEGER NOT NULL REFERENCES foods(id),"
				"grams REAL NOT NULL,"
				"notes TEXT);"
				"CREATE TABLE IF NOT EXISTS workouts("
				"id INTEGER PRIMARY KEY,"
				"started_at TEXT NOT NULL,"
				"notes TEXT);"
				"CREATE TABLE IF NOT EXISTS sets("
				"id INTEGER PRIMARY KEY,"
				"workout_id INTEGER NOT NULL REFERENCES workouts(id) ON DELETE CASCADE,"
				"exercise TEXT NOT NULL,"
				"set_index INTEGER NOT NULL,"
				"reps INTEGER,"
				"weight_kg REAL,"
				"rir REAL);";
		exec(db, sql);
	}

} // namespace cro::db
