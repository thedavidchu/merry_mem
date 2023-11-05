find_package(Catch2 CONFIG REQUIRED)

add_executable(test
    main.cpp
)

target_link_libraries(test
    PRIVATE
    Catch2::Catch2
    Catch2::Catch2WithMain
)

add_test(NAME test COMMAND test)
