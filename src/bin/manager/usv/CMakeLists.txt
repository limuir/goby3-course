set(APP goby3_course_usv_manager)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(${APP}
  app.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(${APP}
  goby
  goby_zeromq
  goby3_course_messages)

if(export_goby_interfaces)
  generate_interfaces(${APP})
endif()
