cmake_minimum_required(VERSION 3.16)
project(LocalTranslate CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3)
FetchContent_MakeAvailable(json httplib)

add_executable(LocalTranslate WIN32 MACOSX_BUNDLE
  src/main.cpp
  src/MainFrame.cpp
  src/OllamaClient.cpp
  src/PromptBuilder.cpp
  src/AppConfig.cpp)

target_link_libraries(LocalTranslate PRIVATE
  ${wxWidgets_LIBRARIES}
  nlohmann_json::nlohmann_json
  httplib::httplib
  Threads::Threads)
