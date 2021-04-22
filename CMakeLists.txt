project(regex)

set(SOURCES
    Regex/DFA.cpp
    Regex/Encoding.cpp
    Regex/Matcher.cpp
    Regex/NFA.cpp
    Regex/Parser.cpp
    DefReader.cpp
    Grammar.cpp
    LLParser.cpp
    Tokenizer.cpp
    Main.cpp
)

add_executable(regex ${SOURCES})