project(regex)

set(SOURCES
    Main.cpp
    Parser.cpp
    NFA.cpp
)
add_executable(regex ${SOURCES})