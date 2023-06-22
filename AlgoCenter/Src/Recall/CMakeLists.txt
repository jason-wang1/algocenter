aux_source_directory(. LIB_SRCS)
aux_source_directory(AnnoyRecall Annoy_SRCS)
add_library(Recall STATIC ${LIB_SRCS} ${Annoy_SRCS})

TARGET_LINK_LIBRARIES(
    Recall
    RedisProtoData
)
