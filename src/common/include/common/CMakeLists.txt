add_library(common
    INTERFACE
)

target_sources(common
    INTERFACE
    logger.hpp
    status.hpp
    types.hpp
)

# Forward this directory to dependents.
target_include_directories(common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# NOTE  I don't add the extra stuff Ga-Chun added in the other CMakeLists.txt
#       out of pure laziness. I'm also not sure how it would interact with an
#       INTERFACE library.
