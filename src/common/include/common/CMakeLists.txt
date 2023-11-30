add_library(common
    INTERFACE
)

target_sources(common
    INTERFACE
    logger.hpp
    status.hpp
    types.hpp
)

# NOTE  I don't add the extra stuff Ga-Chun added in the other CMakeLists.txt
#       out of pure laziness. I'm also not sure how it would interact with an
#       INTERFACE library.
