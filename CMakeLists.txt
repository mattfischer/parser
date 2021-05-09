project(parser)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    Regex/DFA.cpp
    Regex/Encoding.cpp
    Regex/Matcher.cpp
    Regex/NFA.cpp
    Regex/Parser.cpp
    Parser/Grammar.cpp
    Parser/ExtendedGrammar.cpp
    Parser/Earley.cpp
    Parser/LL.cpp
    Parser/SLR.cpp
    Parser/LR.cpp
    Parser/LALR.cpp
    DefReader.cpp
    Tokenizer.cpp
    Main.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
add_executable(parser ${SOURCES})