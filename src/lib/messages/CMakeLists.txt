protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}
  goby3-course/messages/example.proto
  goby3-course/messages/nav_dccl.proto
  goby3-course/messages/health_status.proto
  goby3-course/messages/command_dccl.proto
  )

add_library(goby3_course_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby3_course_messages ${PROTOBUF_LIBRARIES})
