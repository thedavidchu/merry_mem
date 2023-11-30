add_library(common
    INTERFACE
)

target_sources(common
    INTERFACE
    include/common/logger.hpp
    include/common/status.hpp
    include/common/types.hpp
)

# Forward this directory to dependents.
target_include_directories(common
    # I am not sure why PUBLIC INTERFACE works because according to the docs,
    # PUBLIC and INTERFACE act on the items immediately after it. Anyways, we
    # want libraries that include this one to be able to see the common include
    # directory.
    PUBLIC
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# NOTE  I don't add the extra stuff Ga-Chun added in the other CMakeLists.txt
#       out of pure laziness. I'm also not sure how it would interact with an
#       INTERFACE library.
