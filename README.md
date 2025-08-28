# Cro — TUI Gym Tracker

A fast, offline‑first terminal UI (TUI) to track workouts, body metrics, schedule, calories, protein, and
micronutrients—then analyze trends. Featuring an easy CSV export if needed, Cro aims to be your gym companion
everywhere and anywhere with not too much hassle.

## Features

- **Interactive TUI**: Navigate with keyboard shortcuts for fast data entry
- **Exercise Tracking**: Log sets with reps, weight, and notes
- **Excel Import**: Import workout data from Excel/CSV files (see format below)
- **Exercise History**: Track progress with suggested weight progression
- **Offline Database**: SQLite-based storage for reliable, offline-first operation

## Excel Import Format

The Excel import feature supports the following column format:

| Date | Exercise | Reps | Weight | Notes (optional) |
|------|----------|------|--------|------------------|
| 2024-08-20 | Deadlift | 5 | 225 | PR attempt |
| 2024-08-20 | Deadlift | 5 | 225 | |
| 2024-08-22 | Dumbbell chest press | 8 | 45 | Good form |

- **Date**: Workout date (groups sets into workouts)
- **Exercise**: Exercise name (will be added to catalog if new)
- **Reps**: Number of repetitions
- **Weight**: Weight in pounds
- **Notes**: Optional notes for the set

To import data:
1. Run `cro tui`
2. Navigate to the "Settings" tab
3. Enter the path to your Excel file
4. Click "Import from Excel"

AI integration is planned.
