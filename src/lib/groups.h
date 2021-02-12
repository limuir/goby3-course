#ifndef GOBY3_COURSE_SRC_LIB_GROUPS_H
#define GOBY3_COURSE_SRC_LIB_GROUPS_H

#include "goby/middleware/group.h"

namespace goby3_course
{
namespace groups
{
constexpr goby::middleware::Group example{"goby3_course::example"};
constexpr goby::middleware::Group usv_nav{"goby3_course::usv_nav", 1};
constexpr goby::middleware::Group auv_nav{"goby3_course::auv_nav", 2};
} // namespace groups
} // namespace goby3_course

#endif
