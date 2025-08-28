#include "cro/exercise.hpp"
#include "cro/db.hpp"
#include <iostream>
#include <cassert>

int main()
{
	try
	{
		// Create a temporary database
		auto db_handle = cro::db::open(":memory:");
		auto db = db_handle.raw;
		cro::db::migrate(db);

		// Test import
		auto result = cro::exercise::import_from_excel(db, "/Users/ziadelshahawy/personal-projects/cro/sample_workout_data.xlsx");

		std::cout << "Import Results:" << std::endl;
		std::cout << "Workouts imported: " << result.workouts_imported << std::endl;
		std::cout << "Sets imported: " << result.sets_imported << std::endl;
		std::cout << "Errors: " << result.errors.size() << std::endl;

		for (const auto &error : result.errors)
		{
			std::cout << "Error: " << error << std::endl;
		}

		// Check that exercises were imported
		auto exercises = cro::exercise::all_exercises(db);
		std::cout << "\nExercises in database:" << std::endl;
		for (const auto &ex : exercises)
		{
			std::cout << "- " << ex << std::endl;
		}

		// Check history for an exercise
		auto deadlift_history = cro::exercise::get_history(db, "Deadlift");
		if (deadlift_history.last_weight_lb)
		{
			std::cout << "\nDeadlift last weight: " << *deadlift_history.last_weight_lb << " lb" << std::endl;
		}

		std::cout << "\nImport test completed successfully!" << std::endl;
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
