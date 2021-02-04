#ifndef GOBY3_COURSE_SRC_LIB_NAV_CONVERT_H
#define GOBY3_COURSE_SRC_LIB_NAV_CONVERT_H

#include <goby/middleware/protobuf/frontseat_data.pb.h>
#include <goby/time/convert.h>
#include <goby/time/system_clock.h>

#include "goby3-course/nav_dccl.pb.h"

namespace goby3_course
{
inline goby3_course::dccl::NavigationReport
nav_convert(const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav, int vehicle,
            const goby::util::UTMGeodesy& geodesy)
{
    goby3_course::dccl::NavigationReport dccl_nav;
    dccl_nav.set_vehicle(vehicle);

    switch (frontseat_nav.type())
    {
        default: dccl_nav.set_type(goby3_course::dccl::NavigationReport::OTHER); break;
        case goby::middleware::frontseat::protobuf::GLIDER:
        case goby::middleware::frontseat::protobuf::AUV:
            dccl_nav.set_type(goby3_course::dccl::NavigationReport::AUV);
            break;

        case goby::middleware::frontseat::protobuf::USV:
        case goby::middleware::frontseat::protobuf::USV_POWERED:
        case goby::middleware::frontseat::protobuf::USV_SAILING:
            dccl_nav.set_type(goby3_course::dccl::NavigationReport::USV);
            break;

        case goby::middleware::frontseat::protobuf::SHIP:
            dccl_nav.set_type(goby3_course::dccl::NavigationReport::TOPSIDE);
            break;
    }

    auto local_fix = geodesy.convert(
        {frontseat_nav.global_fix().lat_with_units(), frontseat_nav.global_fix().lon_with_units()});
    dccl_nav.set_x_with_units(local_fix.x);
    dccl_nav.set_y_with_units(local_fix.y);
    dccl_nav.set_z_with_units(-frontseat_nav.global_fix.depth_with_units());

    dccl_nav.set_speed_over_ground_with_units(frontseat_nav.speed().over_ground_with_units());

    auto heading = frontseat_nav.pose().heading_with_units();

    const auto 360deg = 360 * boost::units::degree::degrees;
    while (heading >= 360deg) heading -= 360deg;
    while (heading < 0 * boost::units::degree::degrees) heading += 360deg;
    dccl_nav.set_heading_with_units(heading);

    return dccl_nav;
}

inline goby::middleware::frontseat::protobuf::NodeStatus
nav_convert(const dccl::NavigationReport& dccl_nav, const goby::util::UTMGeodesy& geodesy)
{
    goby::middleware::frontseat::protobuf::NodeStatus frontseat_nav;
    auto global_fix = geodesy.convert({dccl_nav.x_with_units(), dccl_nav.y_with_units()});

    frontseat_nav.mutable_global_fix()->set_lat_with_units(global_fix.lat);
    frontseat_nav.mutable_global_fix()->set_lon_with_units(global_fix.lon);
    frontseat_nav.mutable_global_fix()->set_depth_with_units(-dccl_nav.z_with_units());

    frontseat_nav.mutable_local_fix()->set_x_with_units(dccl_nav.x_with_units());
    frontseat_nav.mutable_local_fix()->set_y_with_units(dccl_nav.y_with_units());
    frontseat_nav.mutable_local_fix()->set_z_with_units(dccl_nav.z_with_units());

    frontseat_nav.mutable_pos()->set_heading_with_units(dccl_nav.heading_with_units());
    frontseat_nav.mutable_speed()->set_over_ground_with_units(
        dccl_nav.speed_over_ground_with_units());

    frontseat_nav.set_time_with_units(goby::time::SystemClock::now<goby::time::SITime>());
    frontseat_nav.set_name(goby3_course::dccl::NavigationReport::Type_Name(dccl_nav.type()) + "_" +
                           std::to_string(dccl_nav.vehicle()));

    switch (dccl_nav.type())
    {
        default: frontseat_nav.set_type(goby::middleware::frontseat::protobuf::OTHER); break;
        case goby3_course::dccl::NavigationReport::AUV:
            frontseat_nav.set_type(goby::middleware::frontseat::protobuf::AUV);
            break;
        case goby3_course::dccl::NavigationReport::USV:
            frontseat_nav.set_type(goby::middleware::frontseat::protobuf::USV);
            break;
        case goby3_course::dccl::NavigationReport::TOPSIDE:
            frontseat_nav.set_type(goby::middleware::frontseat::protobuf::SHIP);
            break;
    }

    return frontseat_nav;
}

} // namespace goby3_course

#endif
