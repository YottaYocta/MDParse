cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(MDParse)

include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG cecd54df42dd66fdf8386ed461e16b725bffc827
)

FetchContent_MakeAvailable(ftxui)

set(SRCS
  src/main.cpp
)
add_executable(MDParse ${SRCS})
target_include_directories(MDParse PRIVATE include)
target_link_libraries(MDParse
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)
