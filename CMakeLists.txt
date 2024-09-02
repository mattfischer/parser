cmake_minimum_required(VERSION 3.27)
project(parser)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES
    Regex/DFA.cpp
    Regex/Encoding.cpp
    Regex/Matcher.cpp
    Regex/NFA.cpp
    Regex/Parser.cpp
    Parser/Grammar.cpp
    Parser/ExtendedGrammar.cpp
    Parser/DefReader.cpp
    Parser/Tokenizer.cpp
    Parser/Impl/Base.cpp
    Parser/Impl/Earley.cpp
    Parser/Impl/GLR.cpp
    Parser/Impl/LL.cpp
    Parser/Impl/LR.cpp
    Parser/Impl/LRTable/Base.cpp
    Parser/Impl/LRTable/LALR.cpp
    Parser/Impl/LRTable/Multi.cpp
    Parser/Impl/LRTable/Single.cpp
    Parser/Impl/LRTable/SLR.cpp
    Main.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
add_executable(parser ${SOURCES})