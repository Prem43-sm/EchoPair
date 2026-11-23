cmake_minimum_required(VERSION 3.20)

project(EchoPair VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "EchoPair currently supports Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "EchoPair requires a 64-bit build.")
endif()

add_executable(EchoPair
    src/app/main.cpp
)

target_compile_features(EchoPair PRIVATE cxx_std_20)
target_compile_definitions(EchoPair PRIVATE
    UNICODE
    _UNICODE
    ECHO_PAIR_VERSION="${PROJECT_VERSION}"
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

if(MSVC)
    target_compile_options(EchoPair PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME EchoPair_launch_smoke COMMAND EchoPair)
endif()
