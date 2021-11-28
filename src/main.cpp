#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

int main()
{
  ftxui::ScreenInteractive scr {ftxui::ScreenInteractive::Fullscreen()};
  ftxui::Element main {ftxui::vbox(ftxui::text("Welcome to MDParse") | ftxui::border | ftxui::color(ftxui::Color::Magenta))};
  ftxui::Component renderer {ftxui::Renderer([&]{
    return ftxui::window(ftxui::text("MDParse"), main);
  })};

  scr.Loop(renderer);
  return 0;
}
