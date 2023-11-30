add_library(common INTERFACE)

target_sources(common INTERFACE
    logger.hpp status.hpp types.hpp
)
