#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp> // Toggle, Menu, Input, Button, Container, Renderer, CatchEvent
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include "cro/exercise.hpp"

using namespace ftxui;

namespace cro
{
  int run_tui(sqlite3 *db)
  {
    auto screen = ScreenInteractive::Fullscreen();

    // --- Navbar tabs ---------------------------------------------------------
    int tab = 1; // 0:Dashboard, 1:Training (default), 2:Nutrition, 3:Trends, 4:Settings
    std::vector<std::string> tabs = {"Dashboard", "Training", "Nutrition", "Trends", "Settings"};
    auto tabs_toggle = Toggle(&tabs, &tab);

    // --- Training page (exercise selector + details) -------------------------
    auto names = cro::exercise::all_exercises(db);
    if (names.empty())
      names = {"Deadlift"};
    int selected = 0;

    std::string reps_str = "8";
    std::string weight_str;
    auto history = cro::exercise::History{};

    auto recompute = [&](int idx)
    {
      history = cro::exercise::get_history(db, names[static_cast<size_t>(idx)]);
      if (weight_str.empty())
      {
        weight_str = std::to_string(cro::exercise::suggest_next_weight(history));
      }
    };
    recompute(selected);

    // Left: exercise list
    auto menu = Menu(&names, &selected);

    // Right: inputs + actions
    auto reps_input = Input(&reps_str, "reps");
    auto weight_input = Input(&weight_str, "lb");

    auto minus_btn = Button("-", [&]
                            {
    auto h = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    double w = std::stod(weight_str);
    w -= h.increment_lb; if (w < 0) w = 0; weight_str = std::to_string(w); });
    auto plus_btn = Button("+", [&]
                           {
    auto h = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    double w = std::stod(weight_str);
    w += h.increment_lb; weight_str = std::to_string(w); });

    std::string flash;
    auto log_btn = Button("Log set", [&]
                          {
    try {
      int reps = std::stoi(reps_str);
      double w = std::stod(weight_str);
      cro::exercise::log_set(db, names[static_cast<size_t>(selected)], reps, w);
      flash = "Logged: " + names[static_cast<size_t>(selected)] + " — " + std::to_string(reps) + " x " + std::to_string(w) + " lb";
      history = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    } catch (const std::exception& e) {
      flash = std::string("Error: ") + e.what();
    } });

    auto detail_form = Container::Vertical({weight_input, reps_input, minus_btn, plus_btn, log_btn});
    auto detail = Renderer(detail_form, [&]
                           {
    auto last_text = history.last_weight_lb ? (std::to_string(*history.last_weight_lb) + " lb") : std::string("—");
    auto suggested = cro::exercise::suggest_next_weight(history);
    return vbox({
      text(names[static_cast<size_t>(selected)]) | bold,
      separator(),
      hbox({ text("Last:"), text(last_text) | color(Color::Green) }),
      hbox({ text("Suggest:"), text(std::to_string(suggested) + " lb") | color(Color::Yellow) }),
      separator(),
      hbox({ text("Weight (lb):"), weight_input->Render(), separatorLight(), minus_btn->Render(), plus_btn->Render() }) | border,
      hbox({ text("Reps:"), reps_input->Render() }) | border,
      separator(),
      log_btn->Render(),
      separator(),
      text(flash) | color(Color::GrayLight)
    }) | flex; });

    auto training_split = Container::Horizontal({menu, detail});
    auto training_page = Renderer(training_split, [&]
                                  {
    static int prev = -1;
    if (prev != selected) { prev = selected; weight_str.clear(); recompute(selected); }

    return hbox({
      vbox({ text("Exercises") | bold, separator(), menu->Render() | vscroll_indicator | frame }) | flex,
      separator(),
      detail->Render() | flex
    }); });

    // Stub pages
    auto dashboard_page = Renderer([&]
                                   { return vbox({text("Dashboard (stub)") | dim, text("Upcoming: daily summary, PRs, weekly volume") | dim}) | center; });
    auto nutrition_page = Renderer([&]
                                   { return vbox({text("Nutrition (stub)") | dim, text("Foods, meals, macros, targets") | dim}) | center; });
    auto trends_page = Renderer([&]
                                { return vbox({text("Trends (stub)") | dim, text("Progress charts, bodyweight, volume") | dim}) | center; });

    // Settings page with Excel import
    std::string import_file_path;
    std::string import_status;
    auto import_path_input = Input(&import_file_path, "path/to/workout_data.xlsx");
    auto import_btn = Button("Import from Excel", [&]
                             {
      if (import_file_path.empty()) {
        import_status = "Please enter a file path";
        return;
      }

      try {
        auto result = cro::exercise::import_from_excel(db, import_file_path);
        import_status = "Imported " + std::to_string(result.workouts_imported) +
                       " workouts, " + std::to_string(result.sets_imported) + " sets";
        if (!result.errors.empty()) {
          import_status += " (with " + std::to_string(result.errors.size()) + " errors)";
        }
        // Refresh exercise list
        names = cro::exercise::all_exercises(db);
        if (names.empty()) names = {"Deadlift"};
        if (selected >= static_cast<int>(names.size())) selected = 0;
        recompute(selected);
      } catch (const std::exception& e) {
        import_status = std::string("Import failed: ") + e.what();
      } });

    auto settings_form = Container::Vertical({import_path_input, import_btn});
    auto settings_page = Renderer(settings_form, [&]
                                  { return vbox({text("Settings") | bold,
                                                 separator(),
                                                 text("Import Data from Excel"),
                                                 text("Expected format: Date, Exercise, Reps, Weight, Notes (optional)") | dim,
                                                 hbox({text("File path:"), import_path_input->Render()}) | border,
                                                 separator(),
                                                 import_btn->Render(),
                                                 separator(),
                                                 text(import_status) | (import_status.find("failed") != std::string::npos ||
                                                                                import_status.find("error") != std::string::npos
                                                                            ? color(Color::Red)
                                                                            : color(Color::Green)),
                                                 separator(),
                                                 text("Other Settings (stub)") | dim,
                                                 text("Units, DB path, increments per exercise") | dim}) |
                                           flex; });

    // Switch page by tab index
    auto pages = Container::Tab({dashboard_page, training_page, nutrition_page, trends_page, settings_page}, &tab);

    auto root = Container::Vertical({tabs_toggle, pages});
    auto app = Renderer(root, [&]
                        { return vbox({hbox({text("Cro") | bold, filler(), text("q: quit")}),
                                       separator(),
                                       tabs_toggle->Render() | border,
                                       separator(),
                                       pages->Render() | flex}) |
                                 border; });

    auto keymap = CatchEvent(app, [&](Event e)
                             {
    if (e == Event::Character('q') || e == Event::Escape) { screen.ExitLoopClosure()(); return true; }
    return false; });

    screen.Loop(keymap);
    return 0;
  }
} // namespace cro
