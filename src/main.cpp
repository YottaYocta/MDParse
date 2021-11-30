#include <string>
#include <iostream>
#include <fstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

const std::vector<std::string> delims {
  "#",
  "*"
};

std::ifstream fin;
std::ofstream fout;

int main()
{
  ftxui::ScreenInteractive scr {ftxui::ScreenInteractive::Fullscreen()};

  int selected_front {0};
  int selected_back {0};

  ftxui::Component card_delim_selector {ftxui::Menu(&delims, &selected_front)};
  ftxui::Component definition_delim_selector {ftxui::Menu(&delims, &selected_back)};

  std::string fn {};
  std::string placeholder {"somefile (without .md)"};
  ftxui::InputOption file_opt;

  file_opt.on_enter = [&](){
    fin.open(fn + ".md");
    if (fin.is_open())
    {
      fout.open(fn + ".csv");
      std::string line {};
      while (std::getline(fin, line))
        fout << line << ' ' << "edited" << '\n';
    }
    fn = "";
  };

  ftxui::Component file_input {ftxui::Input(&fn, &placeholder, file_opt)};
  
  ftxui::Component rule {ftxui::Container::Vertical({
    file_input,
    card_delim_selector,
    definition_delim_selector
  })};

  ftxui::Component rule_renderer {ftxui::Renderer(rule, [&](){
    return ftxui::vbox({
      file_input->Render() | ftxui::border,
      ftxui::text("card deliminator") | ftxui::border,
      card_delim_selector->Render() | ftxui::border,
      ftxui::text("definition deliminator") | ftxui::border,
      definition_delim_selector->Render() | ftxui::border
    });
  })};

  ftxui::Component renderer {ftxui::Renderer(rule_renderer, [&]{
    return ftxui::window(ftxui::text("MDParse"), ftxui::vbox({
      rule_renderer->Render()
    }));
  })};

  scr.Loop(renderer);
  return 0;
}
