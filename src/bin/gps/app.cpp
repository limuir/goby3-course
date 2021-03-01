#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/gpsd/groups.h>
#include <goby/middleware/io/line_based/serial.h>
#include <goby/middleware/io/line_based/tcp_client.h>
#include <goby/middleware/protobuf/gpsd.pb.h>
#include <goby/zeromq/application/multi_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/example.pb.h"

using goby::glog;
namespace si = boost::units::si;
namespace config = goby3_course::config;
namespace groups = goby3_course::groups;
namespace zeromq = goby::zeromq;
namespace middleware = goby::middleware;

namespace goby3_course
{
namespace apps
{
class GPSDriver : public zeromq::MultiThreadApplication<config::GPSDriver>
{
  public:
    GPSDriver();
};

} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::GPSDriver>(
        goby::middleware::ProtobufConfigurator<config::GPSDriver>(argc, argv));
}

// Main thread

goby3_course::apps::GPSDriver::GPSDriver()
{
    glog.add_group("main", goby::util::Colors::yellow);
    glog.add_group("status", goby::util::Colors::lt_green);

    using SerialThread = goby::middleware::io::SerialThreadLineBased<
        goby3_course::groups::gps_in, goby3_course::groups::gps_out,
        goby::middleware::io::PubSubLayer::INTERTHREAD, // publish layer
        goby::middleware::io::PubSubLayer::INTERTHREAD  // subscribe layer
        >;

    using TCPThread = goby::middleware::io::TCPClientThreadLineBased<
        goby3_course::groups::gps_in, goby3_course::groups::gps_out,
        goby::middleware::io::PubSubLayer::INTERTHREAD, // publish layer
        goby::middleware::io::PubSubLayer::INTERTHREAD  // subscribe layer
        >;

    if (cfg().has_serial())
        launch_thread<SerialThread>(cfg().serial());
    else if (cfg().has_tcp_client())
        launch_thread<TCPThread>(cfg().tcp_client());
    else if (cfg().use_gpsd())
        interprocess().subscribe<goby::middleware::groups::gpsd::tpv>(
            [](const goby::middleware::protobuf::gpsd::TimePositionVelocity& tpv) {
                glog.is_verbose() &&
                    glog << group("main") << "Time: "
                         << goby::time::convert<boost::posix_time::ptime>(tpv.time_with_units())
                         << ", location: " << tpv.location().ShortDebugString() << std::endl;
            });
    else
        glog.is_die() && glog << "Must specify serial, tcp_client, or use_gpsd configuration."
                              << std::endl;

    interthread().subscribe<goby3_course::groups::gps_in>(
        [](const goby::middleware::protobuf::IOData& data) {
            glog.is_verbose() && glog << group("main")
                                      << "Received serial line:" << data.ShortDebugString()
                                      << std::endl;
        });

    interthread().subscribe<goby3_course::groups::gps_in>(
        [](const goby::middleware::protobuf::IOStatus& status) {
            glog.is_verbose() && glog << group("status")
                                      << "Received I/O status: " << status.ShortDebugString()
                                      << std::endl;
        });
}
