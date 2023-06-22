aux_source_directory(. LIB_SRCS)
add_library(Display STATIC ${LIB_SRCS})

TARGET_LINK_LIBRARIES(
    Display
    RedisProtoData
)
