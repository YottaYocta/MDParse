#include <algorithm>
#include <cstddef>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

const std::vector<std::string> delimiters {
  "#",
  "##",
  "###",
  "####",
  "*",
  "-"
};

struct rule
{
  rule() : term {}, definition {} {}
  rule(const std::string& t, const std::string& d) : term {t}, definition {d} {}
  std::string term, definition;
};

ftxui::Element generate_rule_element(const rule& r)
{
  return ftxui::vbox({
    ftxui::text("Term delimiter: " + r.term),
    ftxui::text("Definition delimiter: " + r.definition)
  }) | ftxui::border;
}

void parse_to_tsv(const std::string& parent, const std::vector<std::string>& lines, const std::vector<rule>& rules, std::ofstream& fout, int depth)
{
  if (depth == 0 && lines.size() > 0) 
    return;

  rule cur_rule {rules[std::min(rules.size() - 1, std::max(std::size_t {0}, rules.size() - depth))]};

  for (int i {0}; i < lines.size(); i++)
  {
    std::stringstream ss {lines[i]};    
    std::string temp {};
    ss >> temp;
    if (temp == cur_rule.term)
    {
      std::vector<std::string> current_block {};

      std::string term {};

      if (ss >> temp)
      {
        term += (parent.size() == 0 ? parent : parent + " - ") + temp;
        fout << '\n' << (parent.size() == 0 ? parent : parent + " - ") << temp;
      }
      while (ss >> temp)
      {
        term += " " + temp; 
        fout << ' ' << temp; 
      }

      int count {0};

      for (int j {i + 1}; j < lines.size(); j++)
      {
        std::stringstream defstream {lines[j]};
        defstream >> temp;

        if (temp == cur_rule.term)
          break;

        count++;

        if (temp == cur_rule.definition)
        {
          
          if (defstream >> temp)
            fout << '\t' << ' ' << temp;
          while (defstream >> temp)
            fout << ' ' << temp; 

        }
        current_block.push_back(lines[j]);
      }

      parse_to_tsv(term, current_block, rules, fout, depth - 1);

      i += count;
    }
  }
}

std::ifstream fin {};
std::ofstream fout {};

int main(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cerr << "usage: MDParse [some-markdown-file].md" << '\n';
    return -1;
  }
  
  // file I/O

  std::string md_file {argv[1]};
  std::size_t file_ext_ind {md_file.find_last_of('.')};
  
  if (file_ext_ind == std::string::npos || md_file.substr(file_ext_ind, 3) != ".md")
  {
    std::cerr << "specified file is not markdown" << '\n';
    return -1;
  }

  std::string tsv_file {md_file.substr(0, file_ext_ind) + ".tsv"};

  fin.open(md_file);

  if (!fin.is_open())
  {
    std::cerr << "error: file not found" << '\n';
    return -1;
  }

  fout.open(tsv_file);

  // data

  std::vector<std::string> lines {};

  std::string newline {};
  while (getline(fin, newline))
    lines.push_back(newline);

  std::vector<rule> rules {};

  // application TUI
  
  ftxui::ScreenInteractive scr {ftxui::ScreenInteractive::Fullscreen()};

  int term_delim_ind {};
  ftxui::Component term_delim_menu {ftxui::Menu(&delimiters, &term_delim_ind)};

  int definition_delim_ind {};
  ftxui::Component definition_delim_menu {ftxui::Menu(&delimiters, &definition_delim_ind)};

  ftxui::Component add_rule {ftxui::Button("Add new rule >>", [&](){
    rules.push_back(rule {delimiters[term_delim_ind], delimiters[definition_delim_ind]});
  })};

  ftxui::Component rule_gen_container {ftxui::Container::Vertical({
    term_delim_menu,
    definition_delim_menu,
    add_rule
  })};

  ftxui::Component rule_gen_renderer {ftxui::Renderer(rule_gen_container, [&](){
    return ftxui::vbox({
      ftxui::hbox({
        ftxui::window(
          ftxui::text("Choose card delimiter:"),
          term_delim_menu->Render()
        ) | ftxui::flex,
        ftxui::window(
          ftxui::text("Choose definition delimiter:"),
          definition_delim_menu->Render()
        ) | ftxui::flex
      }),
      add_rule->Render() | ftxui::color(ftxui::Color::Green)
    }) | ftxui::border;
  })};

  ftxui::Component rule_renderer {ftxui::Renderer([&](){
    ftxui::Elements rule_elements {};

    for (int i {0}; i < rules.size(); i++)
      rule_elements.push_back(generate_rule_element(rules[i]));

    return ftxui::vbox(rule_elements);
  })};

  ftxui::Component parse_button {ftxui::Button("Parse Values", [&](){
    parse_to_tsv("", lines, rules, fout, rules.size());
    scr.ExitLoopClosure()(); 
  })};

  ftxui::Component main {ftxui::Container::Vertical({
    rule_gen_renderer,
    rule_renderer,
    parse_button
  })};

  ftxui::Component main_renderer {ftxui::Renderer(main, [&](){
    return ftxui::vbox({
      rule_gen_renderer->Render(),
      rule_renderer->Render(),
      (rules.size() > 0 ? parse_button->Render() : ftxui::emptyElement())
    });
  })};

  scr.Loop(main_renderer);

  std::clog << '\n' << "\nvalues succesfully parsed!" << '\n';

  return 0;
}
