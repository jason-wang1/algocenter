aux_source_directory(. LIB_SRCS)
add_library(Filter STATIC ${LIB_SRCS})

TARGET_LINK_LIBRARIES(
    Filter
    RedisProtoData
)
