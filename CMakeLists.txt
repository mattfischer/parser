project(regex)

set(SOURCES
    Main.cpp
    Parser.cpp
    NFA.cpp
    DFA.cpp
)
add_executable(regex ${SOURCES})