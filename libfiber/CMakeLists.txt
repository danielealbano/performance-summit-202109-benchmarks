file(GLOB SRC_FILES "*.c" "*.s")

add_library(
        fiber
        ${SRC_FILES})

set_target_properties(
        fiber
        PROPERTIES
        ENABLE_EXPORTS ON
)

