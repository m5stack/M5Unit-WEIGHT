# ESP-IDF component manifest for M5Unit-WEIGHT.
# Upstream: https://github.com/m5stack/M5Unit-WEIGHT
#
# The public headers pull in both M5UnitUnified and M5Utility,
# because the periodic measurement data is held in m5::container::CircularBuffer.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
)
