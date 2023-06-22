aux_source_directory(src LIB_SRCS)
aux_source_directory(src/RedisLocalCache Cache_SRCS)
add_library(RedisProtoData STATIC ${LIB_SRCS} ${Cache_SRCS})

# include directories
INCLUDE_DIRECTORIES(
  ./
)

TARGET_LINK_LIBRARIES(
  RedisProtoData
  Protobuf
  glog
  TDRedis
  pthread
  ${_PROTOBUF_LIBPROTOBUF}
)
