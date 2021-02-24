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

constexpr goby::middleware::Group health_status{"goby3_course::health_status",
                                                goby::middleware::Group::broadcast_group};
constexpr goby::middleware::Group health_status_good{"goby3_course::health_status_good", 1};
constexpr goby::middleware::Group health_status_failed{"goby3_course::health_status_failed", 2};

constexpr goby::middleware::Group usv_command{"goby3_course::usv_command",
                                              goby::middleware::Group::broadcast_group};

} // namespace groups
} // namespace goby3_course

#endif
