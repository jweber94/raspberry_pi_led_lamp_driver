set(TEST_SRCS
    test1.cpp
    ${SOURCE}
)

add_executable(unit_tests
    start_tests.cpp    
    ${TEST_SRCS}
)

target_include_directories(unit_tests
    PUBLIC  ../include
)

target_link_libraries(unit_tests ${CONAN_LIBS})