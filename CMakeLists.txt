cmake_minimum_required(VERSION 3.20)

project(EchoPair VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "EchoPair currently supports Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "EchoPair requires a 64-bit build.")
endif()

add_library(EchoPairCore
    src/devices/audio_endpoint.cpp
    src/devices/endpoint_change_watcher.cpp
    src/audio/wasapi_output.cpp
)

target_include_directories(EchoPairCore PUBLIC include)
target_compile_features(EchoPairCore PUBLIC cxx_std_20)
target_compile_definitions(EchoPairCore PUBLIC
    UNICODE
    _UNICODE
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)

target_link_libraries(EchoPairCore PRIVATE ole32 propsys uuid)

add_executable(EchoPair
    src/app/main.cpp
)
target_link_libraries(EchoPair PRIVATE EchoPairCore)
target_compile_definitions(EchoPair PRIVATE ECHO_PAIR_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(EchoPairCore PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_compile_options(EchoPair PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(EchoPairEndpointTests
        tests/integration/endpoint_enumeration_test.cpp
    )
    target_link_libraries(EchoPairEndpointTests PRIVATE EchoPairCore)
    if(MSVC)
        target_compile_options(EchoPairEndpointTests PRIVATE /W4 /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME EchoPair_endpoint_enumeration COMMAND EchoPairEndpointTests)

    add_executable(EchoPairAudioValidationTests
        tests/unit/wasapi_output_validation_test.cpp
    )
    target_link_libraries(EchoPairAudioValidationTests PRIVATE EchoPairCore)
    if(MSVC)
        target_compile_options(EchoPairAudioValidationTests PRIVATE /W4 /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME EchoPair_audio_output_validation COMMAND EchoPairAudioValidationTests)

    add_executable(EchoPairRingBufferTests
        tests/unit/spsc_ring_buffer_test.cpp
    )
    target_include_directories(EchoPairRingBufferTests PRIVATE include)
    target_compile_features(EchoPairRingBufferTests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(EchoPairRingBufferTests PRIVATE /W4 /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME EchoPair_spsc_ring_buffer COMMAND EchoPairRingBufferTests)
endif()
