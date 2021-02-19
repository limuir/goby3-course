# change for your new application - this is name the binary will be caleed
set(APP goby3_course_my_app)

# turn the config.proto into C++ code: config.pb.cc and config.pb.h
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

# create an executable (binary)
add_executable(${APP}
  app.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

# link it to the appropriate goby libraries and course messages library
target_link_libraries(${APP}
  goby
  goby_zeromq
  goby3_course_messages)

# generate the interfaces file using goby_clang_tool for later visualization
if(export_goby_interfaces)
  generate_interfaces(${APP})
endif()