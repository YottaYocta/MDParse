#include <algorithm>
#include <cstddef>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

struct rule
{
  rule() : term {}, definition {} {}
  rule(const std::string& t, const std::string& d) : term {t}, definition {d} {}
  std::string term, definition;
};

struct parse_options
{
  parse_options(const std::vector<rule>& r, const std::string& term, const std::string& def) :
    rules {r}, 
    term_delimiter {term}, 
    definition_delimiter {def}, 
    depth {rules.size()},
    parent {""} {}
  std::vector<rule> rules;
  std::string definition_delimiter;
  std::string term_delimiter;
  std::string parent;
  std::size_t depth;
};

const std::vector<std::string> delimiters {
  "#",
  "##",
  "###",
  "####",
  "#####",
  "######",
  "*",
  "-",
  "+",
};

const std::vector<std::string> file_delimiters {
  "tab",
  "comma",
  "newline"
};

const std::vector<rule> file_delimiter_values {
  {"\n", "\t"},
  {"\n", ","},
  {"\n\n", "\n"}
};

ftxui::Element generate_rule_element(const rule& r)
{
  return ftxui::vbox({
    ftxui::text("Term delimiter: " + r.term),
    ftxui::text("Definition delimiter: " + r.definition)
  }) | ftxui::border;
}

void parse_markdown(const std::vector<std::string>& lines, std::ofstream& fout, parse_options& options)
{
  if (options.depth == 0 || lines.size() <= 0) 
    return;

  rule cur_rule {options.rules[std::min(options.rules.size() - 1, std::max(std::size_t {0}, options.rules.size() - options.depth))]};

  for (int i {0}; i < lines.size(); i++)
  {
    std::stringstream ss {lines[i]};    
    std::string temp {};
    ss >> temp;
    if (temp == cur_rule.term)
    {
      std::string term {};
      std::string definition {};
      std::vector<std::string> current_block {};

      if (ss >> temp)
        term += (options.parent.size() == 0 ? options.parent : options.parent + " :: ") + temp;
      while (ss >> temp)
        term += " " + temp; 

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
            definition += options.definition_delimiter + temp;
          while (defstream >> temp)
            definition += std::string {" "} + temp; 
        }
        current_block.push_back(lines[j]);
      }

      if (definition.size() > 0)
        fout << term << definition << options.term_delimiter;

      options.depth--;
      std::string temp_parent {options.parent};
      options.parent = term;
      parse_markdown(current_block, fout, options);
      options.depth++;
      options.parent = temp_parent;

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

  std::string txt_file {md_file.substr(0, file_ext_ind) + ".txt"};

  fin.open(md_file);

  if (!fin.is_open())
  {
    std::cerr << "error: file not found" << '\n';
    return -1;
  }

  fout.open(txt_file);

  // data

  std::vector<std::string> lines {};

  std::string newline {};
  while (getline(fin, newline))
    lines.push_back(newline);

  int selected_rule {};
  std::vector<rule> rules {};
  std::vector<std::string> rule_strings {};

  // application TUI
  
  bool parsed {false};

  ftxui::ScreenInteractive scr {ftxui::ScreenInteractive::Fullscreen()};

  int term_delim_ind {};
  ftxui::Component term_delim_menu {ftxui::Menu(&delimiters, &term_delim_ind)};

  int definition_delim_ind {};
  ftxui::Component definition_delim_menu {ftxui::Menu(&delimiters, &definition_delim_ind)};

  int file_delim_ind {};
  ftxui::Component file_delim_menu {ftxui::Menu(&file_delimiters, &file_delim_ind)};

  ftxui::Component add_rule {ftxui::Button("Add new rule >>", [&](){
    rule r {delimiters[term_delim_ind], delimiters[definition_delim_ind]};
    std::string rule_string {r.term + "::" + r.definition};
    rules.push_back(r);
    rule_strings.push_back(rule_string);
  })};

  ftxui::Component remove_rule {ftxui::Button("Remove Rule", [&](){
    if (rules.size() > 0)
    {
      rules.erase(rules.begin() + selected_rule);
      rule_strings.erase(rule_strings.begin() + selected_rule);
      selected_rule = std::max(selected_rule - 1, 0);
    }
  })};

  ftxui::Component rule_gen_container {ftxui::Container::Vertical({
    term_delim_menu,
    definition_delim_menu,
    file_delim_menu,
    add_rule,
    remove_rule
  })};

  ftxui::Component rule_gen_renderer {ftxui::Renderer(rule_gen_container, [&](){
    return ftxui::vbox({
      ftxui::hbox({
        ftxui::window(
          ftxui::text("Choose card delimiter:"),
          term_delim_menu->Render() | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 5)
        ) | ftxui::flex,
        ftxui::window(
          ftxui::text("Choose definition delimiter:"),
          definition_delim_menu->Render() | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 5)
        ) | ftxui::flex
      }),
      ftxui::window(
        ftxui::text("Choose output delimiter:"),
        file_delim_menu->Render() | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 5)
      ),
      add_rule->Render() | ftxui::color(ftxui::Color::Green),
      (rules.size() > 0 ? remove_rule->Render() | ftxui::color(ftxui::Color::Red) : ftxui::emptyElement())
    });
  })};

  ftxui::Component rule_list {ftxui::Menu(&rule_strings, &selected_rule)};

  ftxui::Component parse_button {ftxui::Button("Parse Values", [&](){
    if (rules.size() == 0)
      return;

    parse_options options {rules, file_delimiter_values[file_delim_ind].term, file_delimiter_values[file_delim_ind].definition};

    parse_markdown(lines, fout, options);
    parsed = true;
    scr.ExitLoopClosure()(); 
  })};

  ftxui::Component main {ftxui::Container::Vertical({
    rule_gen_renderer,
    rule_list,
    parse_button
  })};

  ftxui::Component main_renderer {ftxui::Renderer(main, [&](){
    return ftxui::vbox({
      rule_gen_renderer->Render(),
      
      (rules.size() > 0 ? 
        ftxui::vbox({
          ftxui::window(ftxui::text("Rules:"), rule_list->Render() | ftxui::frame | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 7)), 
          parse_button->Render() | ftxui::color(ftxui::Color::White) 
        })
        : ftxui::emptyElement()
      )
    });
  })};

  scr.Loop(main_renderer);

  if (parsed)
    std::clog << "File successfully parsed!" << '\n' << '\n';

  return 0;
}
