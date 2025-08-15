#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp> // Menu, Input, Button, Renderer, CatchEvent
#include <ftxui/dom/elements.hpp>
#include "cro/exercise.hpp"
using namespace ftxui;

namespace cro
{
  int run_tui(sqlite3 *db)
  {
    auto screen = ScreenInteractive::Fullscreen();

    // Load catalog
    auto names = cro::exercise::all_exercises(db);
    if (names.empty())
      names = {"Deadlift"};
    int selected = 0;

    // Inputs
    std::string reps_str = "8";
    std::string weight_str;

    auto recompute = [&](int idx)
    {
      auto h = cro::exercise::get_history(db, names[static_cast<size_t>(idx)]);
      double next = cro::exercise::suggest_next_weight(h);
      if (weight_str.empty())
      {
        // init weight to suggestion on first open, keep user edits thereafter
        weight_str = std::to_string(next);
      }
      return h;
    };

    auto history = recompute(selected);

    // Components
    auto menu = Menu(&names, &selected);
    auto reps_input = Input(&reps_str, "reps");
    auto weight_input = Input(&weight_str, "kg");

    auto minus_btn = Button("-", [&]
                            {
    auto h = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    double w = std::stod(weight_str);
    w -= h.increment_kg; if (w < 0) w = 0; weight_str = std::to_string(w); });
    auto plus_btn = Button("+", [&]
                           {
    auto h = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    double w = std::stod(weight_str);
    w += h.increment_kg; weight_str = std::to_string(w); });
    std::string flash;
    auto log_btn = Button("Log set", [&]
                          {
    try {
      int reps = std::stoi(reps_str);
      double w = std::stod(weight_str);
      cro::exercise::log_set(db, names[static_cast<size_t>(selected)], reps, w);
      flash = "Logged: " + names[static_cast<size_t>(selected)] + " — " + std::to_string(reps) + " x " + std::to_string(w) + " kg";
      history = cro::exercise::get_history(db, names[static_cast<size_t>(selected)]);
    } catch (const std::exception& e) {
      flash = std::string("Error: ") + e.what();
    } });

    auto ui = Renderer(Container::Vertical({menu, reps_input, weight_input, minus_btn, plus_btn, log_btn}), [&]
                       {
    // refresh history if selection changed
    static int prev = selected;
    if (prev != selected) {
      prev = selected; weight_str.clear();
      history = recompute(selected);
    }
    auto last_text = history.last_weight_kg ? (std::to_string(*history.last_weight_kg) + " kg") : std::string("—");
    auto suggested = cro::exercise::suggest_next_weight(history);

    return vbox({
      hbox({ text("Cro — Worksets") | bold, filler(), text("q: quit") }) | border,
      hbox({
        vbox({ text("Exercises") | bold, separator(), menu->Render() | vscroll_indicator | frame }) | flex,
        separator(),
        vbox({
          text(names[static_cast<size_t>(selected)]) | bold,
          separator(),
          hbox({ text("Last:"), text(last_text) | color(Color::Green) }),
          hbox({ text("Suggest:"), text(std::to_string(suggested) + " kg") | color(Color::Yellow) }),
          separator(),
          hbox({ text("Reps:"), reps_input->Render() }) | border,
          hbox({ text("Weight (kg):"), weight_input->Render(), separatorLight(), minus_btn->Render(), plus_btn->Render() }) | border,
          separator(),
          log_btn->Render(),
          separator(),
          text(flash) | color(Color::GrayLight)
        }) | flex
      })
    }); });

    auto app = CatchEvent(ui, [&](Event e)
                          {
    if (e == Event::Character('q') || e == Event::Escape) { screen.ExitLoopClosure()(); return true; }
    return false; });

    screen.Loop(app);
    return 0;
  }
} // namespace cro
