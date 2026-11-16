cmake_minimum_required(VERSION 3.24)
project(GeoTerminal VERSION 2.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32 AND EXISTS "C:/msys64/ucrt64/lib/cmake/wxWidgets-3.2")
    list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
endif()
find_package(wxWidgets 3.2 CONFIG REQUIRED COMPONENTS core base gl)
find_package(OpenGL REQUIRED)

add_executable(GeoTerminal WIN32
    src/main.cpp
    src/geometry.cpp
    src/viewport.cpp
    src/trig_circle.cpp
    src/main_frame.cpp
)
target_include_directories(GeoTerminal PRIVATE src)
target_link_libraries(GeoTerminal PRIVATE wx::core wx::base wx::gl OpenGL::GL)
target_compile_definitions(GeoTerminal PRIVATE UNICODE _UNICODE)
if(MSVC)
    target_compile_options(GeoTerminal PRIVATE /W4 /permissive-)
else()
    target_compile_options(GeoTerminal PRIVATE -Wall -Wextra -Wpedantic)
    if(MINGW)
        target_link_options(GeoTerminal PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

install(TARGETS GeoTerminal RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
    add_executable(GeoTerminalTests tests/geometry_tests.cpp src/geometry.cpp)
    target_include_directories(GeoTerminalTests PRIVATE src)
    add_test(NAME geometry-core COMMAND GeoTerminalTests)
endif()
