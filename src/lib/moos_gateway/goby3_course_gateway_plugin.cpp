#include <goby/zeromq/application/multi_thread.h>

#include "goby3-course/messages/command_dccl.pb.h"
#include "goby3-course/nav/convert.h"
#include "goby3_course_gateway_plugin.h"

namespace goby
{
namespace apps
{
namespace moos
{
namespace protobuf
{
class GobyMOOSGatewayConfig;
} // namespace protobuf
} // namespace moos
} // namespace apps
} // namespace goby

using goby::glog;

extern "C"
{
    void goby3_moos_gateway_load(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
        handler->launch_thread<goby3_course::moos::IvPHelmTranslation>();
        handler->launch_thread<goby3_course::moos::CommandTranslation>();
    }

    void goby3_moos_gateway_unload(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
        handler->join_thread<goby3_course::moos::IvPHelmTranslation>();
        handler->join_thread<goby3_course::moos::CommandTranslation>();
    }
}

void goby3_course::moos::IvPHelmTranslation::publish_contact_nav_to_moos(
    const goby3_course::dccl::NavigationReport& nav_report)
{
    glog.is_verbose() && glog << "Posting to MOOS: Contact NAV: " << nav_report.DebugString()
                              << std::endl;

    using boost::units::quantity;
    namespace si = boost::units::si;

    // rewarp time since we send it "unwarped" over acomms
    // moos uses seconds since UNIX
    double moos_time = goby::time::convert<goby::time::SITime>(
                           goby::time::SystemClock::warp(
                               goby::time::convert<std::chrono::system_clock::time_point>(
                                   nav_report.time_with_units())))
                           .value();

    std::stringstream node_report;
    node_report
        << "NAME=" << goby3_course::vehicle_name(nav_report) << ","
        << "TYPE=" << goby3_course::dccl::NavigationReport::VehicleClass_Name(nav_report.type())
        << ","
        << "TIME=" << std::setprecision(std::numeric_limits<double>::digits10) << moos_time << ","
        << "X=" << nav_report.x_with_units<quantity<si::length>>().value() << ","
        << "Y=" << nav_report.y_with_units<quantity<si::length>>().value() << ","
        << "DEPTH=" << -nav_report.z_with_units<quantity<si::length>>().value() << ","
        << "HDG="
        << nav_report.heading_with_units<quantity<boost::units::degree::plane_angle>>().value()
        << ","
        << "SPD=" << nav_report.speed_over_ground_with_units<quantity<si::velocity>>().value();

    glog.is_verbose() && glog << "NODE_REPORT: " << node_report.str() << std::endl;
    moos().comms().Notify("NODE_REPORT", node_report.str());
}

goby3_course::moos::CommandTranslation::CommandTranslation(
    const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
    : goby::moos::Translator(cfg)
{
    using goby3_course::dccl::USVCommand;
    using goby3_course::groups::usv_command;

    auto on_usv_command = [this](const USVCommand& command) {
        // send update first
        if (command.desired_state() == USVCommand::POLYGON)
        {
            std::stringstream update_ss;
            update_ss << "polygon=radial::x=0,y=0,radius=" << command.polygon_radius()
                      << ",pts=" << command.polygon_sides();
            glog.is_verbose() && glog << "POLYGON_UPDATES: " << update_ss.str() << std::endl;
            moos().comms().Notify("POLYGON_UPDATES", update_ss.str());
        }
        glog.is_verbose() &&
            glog << "DEPLOY_STATE: " << USVCommand::AutonomyState_Name(command.desired_state())
                 << std::endl;
        moos().comms().Notify("DEPLOY_STATE",
                              USVCommand::AutonomyState_Name(command.desired_state()));
    };
    glog.is_verbose() && glog << "Command Translation starting up" << std::endl;
    goby().interprocess().subscribe<usv_command>(on_usv_command);
}