# NOTE: We include header files to make them visible to IDEs.
add_executable(performance_test_exe
    main.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(performance_test_exe
    PRIVATE
    #parallel_lib
    naive_parallel_lib
    sequential_lib
    trace_lib
    utility_lib
    Threads::Threads
)



target_compile_options(performance_test_exe
    PRIVATE
    ${MM_REQUIRED_WARN_FLAGS}
    ${MM_EXTRA_WARN_FLAGS}
    #-lpthread
)

# CMake flags for Release builds are suboptimal.
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/20812.
# See: https://stackoverflow.com/questions/28178978/how-to-generate-pdb-files-for-release-build-with-cmake-flags.
# TODO(glin): Can this be refactored into a function?
if(MSVC)
    target_compile_options(performance_test_exe
        PRIVATE
        $<$<CONFIG:Release>:/Zc:inline>
        $<$<CONFIG:Release>:/Zi>
        $<$<CONFIG:Release>:/Gy>
    )
    target_link_options(performance_test_exe
        PRIVATE
        $<$<CONFIG:Release>:/DEBUG>
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
    target_compile_options(performance_test_exe
        PRIVATE
        $<$<CONFIG:Release>:-g>
    )
    target_link_options(performance_test_exe
        PRIVATE
        $<$<CONFIG:Release>:-g>
    )
    if(WIN32)
        target_compile_options(performance_test_exe
            PRIVATE
            $<$<CONFIG:Release>:-gcodeview>
        )
    endif()
endif()
