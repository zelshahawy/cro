#include "cro/exercise.hpp"
#include <sqlite3.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	std::string iso_now_local()
	{
		using namespace std::chrono;
		auto now = system_clock::now();
		std::time_t t = system_clock::to_time_t(now);
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		std::ostringstream os;
		os << std::setfill('0')
			 << std::setw(4) << (tm.tm_year + 1900) << '-'
			 << std::setw(2) << (tm.tm_mon + 1) << '-'
			 << std::setw(2) << tm.tm_mday << 'T'
			 << std::setw(2) << tm.tm_hour << ':'
			 << std::setw(2) << tm.tm_min << ':'
			 << std::setw(2) << tm.tm_sec;
		return os.str();
	}
}

namespace cro::exercise
{

	History get_history(sqlite3 *db, const std::string &name)
	{
		History h;
		{
			const char *sql = "SELECT default_increment_lb FROM exercises WHERE name = ?1 LIMIT 1;";
			sqlite3_stmt *st = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
				throw std::runtime_error("prepare get_history(increment) failed");
			sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(st) == SQLITE_ROW)
				h.increment_lb = sqlite3_column_double(st, 0);
			sqlite3_finalize(st);
		}

		{
			const char *sql =
					"SELECT s.weight_lb FROM sets s "
					"JOIN workouts w ON w.id = s.workout_id "
					"WHERE s.exercise = ?1 AND s.weight_lb IS NOT NULL "
					"ORDER BY w.started_at DESC, s.set_index DESC LIMIT 1;";
			sqlite3_stmt *st = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
				throw std::runtime_error("prepare get_history(last) failed");
			sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(st) == SQLITE_ROW)
				h.last_weight_lb = sqlite3_column_double(st, 0);
			sqlite3_finalize(st);
		}
		return h;
	}

	double suggest_next_weight(const History &h)
	{
		if (h.last_weight_lb)
			return *h.last_weight_lb + h.increment_lb;
		// baseline suggestion if no history yet
		return std::max(5.0, h.increment_lb * 2.0);
	}

	int log_set(sqlite3 *db, const std::string &name, int reps, double weight_lb,
							std::optional<double> rir, const std::string &notes)
	{
		int workout_id = 0;
		{
			const char *sql = "INSERT INTO workouts(started_at, notes) VALUES (?1, ?2);";
			sqlite3_stmt *st = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
				throw std::runtime_error("prepare insert workout failed");
			auto ts = iso_now_local();
			sqlite3_bind_text(st, 1, ts.c_str(), -1, SQLITE_TRANSIENT);
			if (!notes.empty())
				sqlite3_bind_text(st, 2, notes.c_str(), -1, SQLITE_TRANSIENT);
			else
				sqlite3_bind_null(st, 2);
			if (sqlite3_step(st) != SQLITE_DONE)
				throw std::runtime_error("insert workout failed");
			sqlite3_finalize(st);
			workout_id = static_cast<int>(sqlite3_last_insert_rowid(db));
		}

		int set_index = 1;
		{
			const char *sql = "SELECT COALESCE(MAX(set_index)+1,1) FROM sets WHERE workout_id=?1 AND exercise=?2;";
			sqlite3_stmt *st = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
				throw std::runtime_error("prepare next set_index failed");
			sqlite3_bind_int(st, 1, workout_id);
			sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(st) == SQLITE_ROW)
				set_index = sqlite3_column_int(st, 0);
			sqlite3_finalize(st);
		}

		int set_id = 0;
		{
			const char *sql = "INSERT INTO sets(workout_id, exercise, set_index, reps, weight_lb, rir) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
			sqlite3_stmt *st = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
				throw std::runtime_error("prepare insert set failed");
			sqlite3_bind_int(st, 1, workout_id);
			sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(st, 3, set_index);
			sqlite3_bind_int(st, 4, reps);
			sqlite3_bind_double(st, 5, weight_lb);
			if (rir)
				sqlite3_bind_double(st, 6, *rir);
			else
				sqlite3_bind_null(st, 6);
			if (sqlite3_step(st) != SQLITE_DONE)
				throw std::runtime_error("insert set failed");
			sqlite3_finalize(st);
			set_id = static_cast<int>(sqlite3_last_insert_rowid(db));
		}
		return set_id;
	}

	std::vector<std::string> all_exercises(sqlite3 *db)
	{
		std::vector<std::string> out;
		const char *sql = "SELECT name FROM exercises ORDER BY name ASC;";
		sqlite3_stmt *st = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
			throw std::runtime_error("prepare all_exercises failed");
		while (sqlite3_step(st) == SQLITE_ROW)
		{
			const unsigned char *txt = sqlite3_column_text(st, 0);
			out.emplace_back(reinterpret_cast<const char *>(txt ? txt : reinterpret_cast<const unsigned char *>("")));
		}
		sqlite3_finalize(st);
		return out;
	}

} // namespace cro::exercise
