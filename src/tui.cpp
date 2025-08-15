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

    // The detail panel is its own container. Because we put menu + detail inside
    // a **Horizontal** container, pressing → on the menu jumps straight here.
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

    // Split: left list, right details. **Right arrow** switches between them.
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

    // --- Stub pages ----------------------------------------------------------
    auto dashboard_page = Renderer([&]
                                   { return vbox({text("Dashboard (stub)") | dim, text("Upcoming: daily summary, PRs, weekly volume") | dim}) | center; });
    auto nutrition_page = Renderer([&]
                                   { return vbox({text("Nutrition (stub)") | dim, text("Foods, meals, macros, targets") | dim}) | center; });
    auto trends_page = Renderer([&]
                                { return vbox({text("Trends (stub)") | dim, text("Progress charts, bodyweight, volume") | dim}) | center; });
    auto settings_page = Renderer([&]
                                  { return vbox({text("Settings (stub)") | dim, text("Units, DB path, increments per exercise") | dim}) | center; });

    // Switch page by tab index
    auto pages = Container::Tab({dashboard_page, training_page, nutrition_page, trends_page, settings_page}, &tab);

    // Root: navbar on top, page underneath
    auto root = Container::Vertical({tabs_toggle, pages});
    auto app = Renderer(root, [&]
                        { return vbox({hbox({text("Cro") | bold, filler(), text("q: quit")}),
                                       separator(),
                                       tabs_toggle->Render() | border,
                                       separator(),
                                       pages->Render() | flex}) |
                                 border; });

    // Global shortcuts
    auto keymap = CatchEvent(app, [&](Event e)
                             {
    if (e == Event::Character('q') || e == Event::Escape) { screen.ExitLoopClosure()(); return true; }
    return false; });

    screen.Loop(keymap);
    return 0;
  }
} // namespace cro
