project(parser)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    Regex/DFA.cpp
    Regex/Encoding.cpp
    Regex/Matcher.cpp
    Regex/NFA.cpp
    Regex/Parser.cpp
    Parser/Base.cpp
    Parser/Grammar.cpp
    Parser/ExtendedGrammar.cpp
    Parser/DefReader.cpp
    Parser/Tokenizer.cpp
    Parser/Impl/Earley.cpp
    Parser/Impl/LL.cpp
    Parser/Impl/SLR.cpp
    Parser/Impl/LR.cpp
    Parser/Impl/LALR.cpp
    Parser/Impl/LRMulti.cpp
    Parser/Impl/LRSingle.cpp
    Parser/Impl/GLR.cpp
    Main.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
add_executable(parser ${SOURCES})