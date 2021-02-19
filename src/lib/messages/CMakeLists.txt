protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}/goby3-course/messages
  example.proto
  nav_dccl.proto
  health_status.proto
  )

add_library(goby3_course_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby3_course_messages ${PROTOBUF_LIBRARIES})
