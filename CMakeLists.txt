cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(MerryMem
    VERSION 0.1.0
    LANGUAGES CXX
)

# Use C++20 without compiler-specific extensions.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

# Enable LTO for Release builds, if available.
include(CheckIPOSupported)
check_ipo_supported(RESULT MM_IPO_SUPPORTED)
message(STATUS "IPO supported: ${MM_IPO_SUPPORTED}")
if(MM_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)
endif()

# Set properties for a more natural development environment in Visual Studio
# (and other IDEs).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT sequential
)
set(CMAKE_VS_JUST_MY_CODE_DEBUGGING ON)

# Collect baseline recommended warning flags.
if(MSVC)
    string(REGEX REPLACE "[-/]W[1-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(MM_REQUIRED_WARN_FLAGS ${MM_REQUIRED_WARN_FLAGS} /W4 /permissive-)
else()
    set(MM_REQUIRED_WARN_FLAGS -Wall -Wextra -Wpedantic -Wnon-virtual-dtor)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC `missing-field-initializers` produces nonsense warnings about
    # `<struct>::<anonymous>`.
    set(MM_REQUIRED_WARN_FLAGS
        ${MM_REQUIRED_WARN_FLAGS}
        -Wno-missing-field-initializers
    )
endif()

# See: https://github.com/cpp-best-practices/cppbestpractices.
option(MM_USE_EXTRA_WARNINGS
    "Enable more warnings recommended by C++ Best Practices." TRUE
)
if(MM_USE_EXTRA_WARNINGS)
    if(MSVC)
        set(MM_EXTRA_WARN_FLAGS
            /w14242 # possible loss of data: conversion from `type1` to `type2`
            /w14254 # possible loss of data: conversion from `type1:field_bits`
                    # to `type2:field_bits`
            /w14263 # member function does not override any base class virtual
                    # member function
            /w14265 # class has virtual functions, but destructor is not
                    # virtual; instances of this class may not be destructed
                    # correctly
            /w14287 # unsigned/negative constant mismatch
            /we4289 # loop control variable declared in the for-loop is used
                    # outside the for-loop scope
            /w14296 # expression is always [true | false]
            /w14311 # pointer truncation from `type1` to `type2`
            /w14545 # expression before comma evaluates to a function which is
                    # missing an argument list
            /w14546 # function call before comma missing argument list
            /w14547 # operator before comma has no effect; expected operator
                    # with side-effect
            /w14549 # operator before comma has no effect; did you intend
                    # 'operator'?
            /w14555 # expression has no effect; expected expression with
                    # side-effect
            /w14619 # pragma warning: there is no warning number `number`
            /w14640 # thread un-safe static member initialization
            /w14826 # conversion from 'type1' to 'type_2' is sign-extended which
                    # may cause unexpected runtime behavior
            /w14928 # illegal copy-initialization; more than one user-defined
                    # conversion has been implicitly applied
        )
    else()
        set(MM_EXTRA_WARN_FLAGS
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wmisleading-indentation
            -Wformat=2
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND MM_EXTRA_WARN_FLAGS
            -Wduplicated-cond
            -Wduplicated-branches
            -Wuseless-cast
            -Wlogical-op
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        list(APPEND MM_EXTRA_WARN_FLAGS
            -Wimplicit-fallthrough
            -Wshorten-64-to-32
        )
    endif()
endif()

add_subdirectory(src)
