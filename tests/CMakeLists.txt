add_executable(clipd_tests
    test_main.cpp
    test_detector.cpp
    test_cleaners.cpp
)
target_link_libraries(clipd_tests PRIVATE clipd_core)
target_include_directories(clipd_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME clipd_tests COMMAND clipd_tests)
