#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
using namespace ftxui;

namespace cro {
int run_tui() {
  auto screen = ScreenInteractive::Fullscreen();
  int tab = 0;
  std::vector<std::string> tabs = {"Dashboard", "Workouts", "Nutrition",
                                   "Trends", "Settings"};

  auto toggle = Toggle(&tabs, &tab);
  auto ui = Renderer(toggle, [&] {
    Element body;
    switch (tab) {
    case 0:
      body = paragraph("Dashboard — press q to quit");
      break;
    case 1:
      body = paragraph("Workouts");
      break;
    case 2:
      body = paragraph("Nutrition");
      break;
    case 3:
      body = paragraph("Trends");
      break;
    default:
      body = paragraph("Settings");
      break;
    }
    return vbox({
               hbox({text("Cro") | bold, filler(), text("TUI MVP")}) | border,
               separator(),
               toggle->Render() | border,
               separator(),
               body | border | flex,
           }) |
           border;
  });

  auto app = CatchEvent(ui, [&](Event e) {
    if (e == Event::Character('q') || e == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(app);
  return 0;
}
} // namespace cro
