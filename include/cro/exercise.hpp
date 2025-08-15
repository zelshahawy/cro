#pragma once
#include <optional>
#include <string>
#include <vector>
struct sqlite3;

namespace cro::exercise
{
	struct History
	{
		std::optional<double> last_weight_lb;
		double increment_lb = 2.5;
	};

	History get_history(sqlite3 *db, const std::string &name);

	// Simple progression: last + increment.
	double suggest_next_weight(const History &);

	// Log a single set. Creates a new workout row with current timestamp.
	// Returns the inserted set id.
	int log_set(sqlite3 *db, const std::string &name, int reps, double weight_lb,
							std::optional<double> rir = std::nullopt, const std::string &notes = "");

	// List all exercises from the catalog (alphabetical)
	std::vector<std::string> all_exercises(sqlite3 *db);
} // namespace cro::exercise
