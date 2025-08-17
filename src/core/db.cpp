#include "cro/db.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

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

	static void seed_exercises(sqlite3 *db)
	{
		static const char *names[] = {
				"Dumbbell chest press",
				"Dumbbell Incline Chest Press",
				"Skull Crushers",
				"Fly Machine",
				"Triceps pull down",
				"Deadlift",
				"Dumbbell row",
				"Wide Grip Pull",
				"Dumbbell Preacher Curls",
				"Inclined Dumbbell Curls",
				"Dumbbell shoulder press",
				"Seated Leg Press",
				"Reverse Fly Machine",
				"Dumbbell lateral raises",
		};

		const char *ins = "INSERT OR IGNORE INTO exercises(name, default_increment_lb) VALUES (?1, 2.5);";
		sqlite3_stmt *st = nullptr;
		if (sqlite3_prepare_v2(db, ins, -1, &st, nullptr) != SQLITE_OK)
			throw std::runtime_error("prepare seed_exercises failed");
		for (auto *n : names)
		{
			sqlite3_reset(st);
			sqlite3_clear_bindings(st);
			sqlite3_bind_text(st, 1, n, -1, SQLITE_STATIC);
			if (sqlite3_step(st) != SQLITE_DONE)
				throw std::runtime_error("seed_exercises step failed");
		}
		sqlite3_finalize(st);
	}

	void migrate(sqlite3 *db)
	{
		const char *sql =
				"PRAGMA foreign_keys = ON;"
				// Master catalog of exercises
				"CREATE TABLE IF NOT EXISTS exercises("
				"id INTEGER PRIMARY KEY,"
				"name TEXT NOT NULL UNIQUE,"
				"default_increment_lb REAL NOT NULL DEFAULT 2.5);"
				// Foods and nutrition
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
				// Workouts + sets
				"CREATE TABLE IF NOT EXISTS workouts("
				"id INTEGER PRIMARY KEY,"
				"started_at TEXT NOT NULL,"
				"notes TEXT);"
				"CREATE TABLE IF NOT EXISTS sets("
				"id INTEGER PRIMARY KEY,"
				"workout_id INTEGER NOT NULL REFERENCES workouts(id) ON DELETE CASCADE,"
				"exercise TEXT NOT NULL," // canonical name string
				"set_index INTEGER NOT NULL,"
				"reps INTEGER,"
				"weight_lb REAL,"
				"rir REAL);";

		exec(db, sql);
		seed_exercises(db);
	}

} // namespace cro::db
