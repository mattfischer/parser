project(regex)

set(SOURCES
    Main.cpp
    Parser.cpp
    NFA.cpp
    DFA.cpp
    Encoding.cpp
    Matcher.cpp
)
add_executable(regex ${SOURCES})