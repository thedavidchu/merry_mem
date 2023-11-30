add_library(common
    INTERFACE
)

target_sources(common
    INTERFACE
    include/common/logger.hpp
    include/common/status.hpp
    include/common/types.hpp
)

# NOTE  I don't add the extra stuff Ga-Chun added in the other CMakeLists.txt
#       out of pure laziness. I'm also not sure how it would interact with an
#       INTERFACE library.
