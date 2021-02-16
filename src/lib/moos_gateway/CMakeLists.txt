set(LIB goby3_course_moos_gateway_plugin)
add_library(${LIB} SHARED goby3_course_gateway_plugin.cpp)
target_link_libraries(${LIB}
  goby goby_zeromq goby_moos goby3_course_messages)

