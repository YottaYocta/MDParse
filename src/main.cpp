#include <algorithm>
#include <cstddef>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

class debug
{
  public:
    debug() : dlog {}, db {false} {}
    debug(std::string fn, bool d) : dlog {fn}, db {d} {}
    template <class T>
    void log(const T& v)
    {
      if (db)
        dlog << v;
    }
  private:
    std::ofstream dlog;
    bool db;
};

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
    parent {""},
    depth {static_cast<int>(rules.size())},
    dlog {} {}
  parse_options(const std::vector<rule>& r, const std::string& term, const std::string& def, bool d) :
    rules {r}, 
    term_delimiter {term}, 
    definition_delimiter {def}, 
    parent {""},
    depth {static_cast<int>(rules.size())},
    dlog {"parselog.txt", d} {}
  std::vector<rule> rules;
  std::string definition_delimiter;
  std::string term_delimiter;
  std::string parent;
  int depth;
  debug dlog;
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

void parse_markdown(const std::vector<std::string>& lines, std::vector<std::string>& buffer, parse_options& options)
{
  if (lines.size() <= 0 || options.depth <= 0)
    return;

  rule cur_rule {options.rules[options.rules.size() - options.depth]};

  options.dlog.log("\n{\n\nRULES:\n");
  for (int i {0}; i < options.rules.size(); i++)
    options.dlog.log(options.rules[i].term + ' ' + options.rules[i].definition + '\n');
  options.dlog.log('\n');

  options.dlog.log("CURRENT RULE:\n");
  options.dlog.log(cur_rule.term + ' ' + cur_rule.definition + '\n');
  options.dlog.log("\nLINES:\n\n");

  for (int i {0}; i < lines.size(); i++)
  {
    options.dlog.log(lines[i] + '\n');    
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

      for (int j {i + 1}; j < lines.size(); j++)
      {
        std::stringstream defstream {lines[j]};
        defstream >> temp;

        if (temp == cur_rule.term)
          break;

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
      {
        buffer.push_back(term + definition + options.term_delimiter);
      }
      options.dlog.log("\nterm: " + term + "\ndefinition: " + definition + "\n\n");

      options.dlog.log("\ncurrent block: \n");
      for (int i {0}; i < current_block.size(); i++)
        options.dlog.log(current_block[i] + '\n');

      std::string temp_parent {options.parent};
      options.parent = term;
      options.depth--;
      parse_markdown(current_block, buffer, options);
      options.depth++;
      options.parent = temp_parent;

      i += current_block.size();
    }
  }
  options.dlog.log("\n}\n\n");
}


int main(int argc, char* argv[])
{
  std::ifstream fin {};
  std::ofstream fout {};

  bool logging {false};

  if (argc <= 1)
  {
    std::cerr << "usage: MDParse [some-markdown-file].md [tags]" << '\n' << '\t' << "-d, --debug: log errors and processes" << '\n';
    return -1;
  }

  for (int i {0}; i < argc; i++)
  {
    std::string arg {argv[i]};

    if (arg == "--debug" || arg == "-d")
      logging = true;
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

    std::vector<rule> rule_copy {rules.begin(), rules.end()};
    parse_options options {rule_copy, file_delimiter_values[file_delim_ind].term, file_delimiter_values[file_delim_ind].definition, logging};

    options.dlog.log("rules:\n");
    for (int i {0}; i < options.rules.size(); i++)
      options.dlog.log("term: " + options.rules[i].term + "\ndefinition: " + options.rules[i].definition + '\n');
    options.dlog.log('\n');

    std::vector<std::string> buffer {};

    parse_markdown(lines, buffer, options);
    for (const std::string& line : buffer)
      fout << line;
    parsed = true;
    options.dlog.log("values written from buffer");
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
