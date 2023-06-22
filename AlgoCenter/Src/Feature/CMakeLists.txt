aux_source_directory(. LIB_SRCS)
add_library(Feature STATIC ${LIB_SRCS})

TARGET_LINK_LIBRARIES(
    Feature
    RedisProtoData
    glog
)
