#ifndef GOBY3_COURSE_SRC_LIB_NAV_INTERVEHICLE_H
#define GOBY3_COURSE_SRC_LIB_NAV_INTERVEHICLE_H

#include "goby3-course/groups.h"
#include "goby3-course/messages/nav_dccl.pb.h"

namespace goby3_course
{
// use "type" field in NavigationReport to set the correct group
inline void nav_set_group_function(goby3_course::dccl::NavigationReport& report,
                                   const goby::middleware::Group& group)
{
    if (group == goby3_course::groups::usv_nav)
        report.set_type(goby3_course::dccl::NavigationReport::USV);
    else if (group == goby3_course::groups::auv_nav)
        report.set_type(goby3_course::dccl::NavigationReport::AUV);
    else
        throw(std::runtime_error("Unsupported Group " + std::string(group) +
                                 " for use with navigation publications"));
}

// use "type" field in NavigationReport to determine the correct group
goby::middleware::Group nav_group_function(const goby3_course::dccl::NavigationReport& report)
{
    switch (report.type())
    {
        default:
            throw(std::runtime_error(
                "Unsupported VehicleClass " +
                goby3_course::dccl::NavigationReport::VehicleClass_Name(report.type()) +
                " for use with navigation publications"));

        case goby3_course::dccl::NavigationReport::AUV: return goby3_course::groups::auv_nav;
        case goby3_course::dccl::NavigationReport::USV: return goby3_course::groups::usv_nav;
    }
}

inline goby::middleware::Publisher<goby3_course::dccl::NavigationReport> nav_publisher()
{
    return goby::middleware::Publisher<goby3_course::dccl::NavigationReport>(
        {{}, // empty config
         std::bind(nav_set_group_function, std::placeholders::_1, std::placeholders::_2)});
}

inline goby::middleware::Subscriber<goby3_course::dccl::NavigationReport>
nav_subscriber(const goby::middleware::intervehicle::protobuf::TransporterConfig& intervehicle_cfg)
{
    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    *subscriber_cfg.mutable_intervehicle() = intervehicle_cfg;

    return goby::middleware::Subscriber<goby3_course::dccl::NavigationReport>(
        {subscriber_cfg, std::bind(nav_group_function, std::placeholders::_1)});
}

} // namespace goby3_course

#endif
