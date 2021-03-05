#ifndef GOBY3_COURSE_SRC_LIB_NAV_CONVERT_H
#define GOBY3_COURSE_SRC_LIB_NAV_CONVERT_H

#include <goby/middleware/protobuf/frontseat_data.pb.h>
#include <goby/time/convert.h>
#include <goby/time/system_clock.h>

#include "goby3-course/messages/nav_dccl.pb.h"

namespace goby3_course
{
inline std::string vehicle_name(const dccl::NavigationReport& dccl_nav,
                                int auv_index_to_modem_id_offset = 2 // modem id of AUV 0
)
{
    std::string type = goby3_course::dccl::NavigationReport::VehicleClass_Name(dccl_nav.type());
    switch (dccl_nav.type())
    {
        case goby3_course::dccl::NavigationReport::AUV:
            return type + "_" + std::to_string(dccl_nav.vehicle() - auv_index_to_modem_id_offset);

            // assume only one of the other types for this course
        default: return type;
    }
}

inline goby3_course::dccl::NavigationReport
nav_convert(const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav, int vehicle,
            const goby::util::UTMGeodesy& geodesy)
{
    goby3_course::dccl::NavigationReport dccl_nav;
    dccl_nav.set_vehicle(vehicle);

    // DCCL uses the real system clock to encode time, so "unwarp" the time first
    dccl_nav.set_time_with_units(goby::time::convert<goby::time::MicroTime>(
        goby::time::SystemClock::unwarp(goby::time::convert<goby::time::SystemClock::time_point>(
            frontseat_nav.time_with_units()))));

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
    dccl_nav.set_z_with_units(-frontseat_nav.global_fix().depth_with_units());

    dccl_nav.set_speed_over_ground_with_units(frontseat_nav.speed().over_ground_with_units());

    auto heading = frontseat_nav.pose().heading_with_units();

    const auto revolution = 360 * boost::units::degree::degrees;
    while (heading >= revolution) heading -= revolution;
    while (heading < 0 * boost::units::degree::degrees) heading += revolution;
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

    frontseat_nav.mutable_pose()->set_heading_with_units(dccl_nav.heading_with_units());
    frontseat_nav.mutable_speed()->set_over_ground_with_units(
        dccl_nav.speed_over_ground_with_units());

    // rewarp the time
    frontseat_nav.set_time_with_units(goby::time::convert<goby::time::MicroTime>(
        goby::time::SystemClock::warp(goby::time::convert<std::chrono::system_clock::time_point>(
            dccl_nav.time_with_units()))));
    frontseat_nav.set_name(vehicle_name(dccl_nav));

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
