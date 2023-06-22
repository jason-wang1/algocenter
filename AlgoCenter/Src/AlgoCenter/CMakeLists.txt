aux_source_directory(. ALL_FILES)
ADD_EXECUTABLE(AlgoCenter ${ALL_FILES})

TARGET_LINK_LIBRARIES( 
    AlgoCenter 
    Protobuf
    Feature
    TDPredict
    glog
    TDRedis
    RedisProtoData
    CSignal
    Recall
    Display
    RegisterCenter
    MurmurHash3
    Filter
    KafkaClient
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)
