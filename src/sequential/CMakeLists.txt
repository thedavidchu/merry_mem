# NOTE: We include header files to make them visible to IDEs.
add_library(sequential
    sequential.cpp
    include/sequential/sequential.hpp
)

target_link_libraries(sequential
    PRIVATE
    common
)

    message(${CMAKE_SOURCE_DIR}/common/include)
# Forward this directory to dependents.
target_include_directories(sequential
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/common/include
)

target_compile_options(sequential
    PRIVATE
    ${MM_REQUIRED_WARN_FLAGS}
    ${MM_EXTRA_WARN_FLAGS}
)

add_executable(sequential_exe
    main.cpp
)

target_link_libraries(sequential_exe
    PRIVATE
    sequential
)

target_compile_options(sequential_exe
    PRIVATE
    ${MM_REQUIRED_WARN_FLAGS}
    ${MM_EXTRA_WARN_FLAGS}
)

# CMake flags for Release builds are suboptimal.
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/20812.
# See: https://stackoverflow.com/questions/28178978/how-to-generate-pdb-files-for-release-build-with-cmake-flags.
# TODO(glin): Can this be refactored into a function?
if(MSVC)
    target_compile_options(sequential
        PRIVATE
        $<$<CONFIG:Release>:/Zc:inline>
        $<$<CONFIG:Release>:/Zi>
        $<$<CONFIG:Release>:/Gy>
    )
    target_link_options(sequential
        PRIVATE
        $<$<CONFIG:Release>:/DEBUG>
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
    target_compile_options(sequential
        PRIVATE
        $<$<CONFIG:Release>:-g>
    )
    target_link_options(sequential
        PRIVATE
        $<$<CONFIG:Release>:-g>
    )
    if(WIN32)
        target_compile_options(sequential
            PRIVATE
            $<$<CONFIG:Release>:-gcodeview>
        )
    endif()
endif()
