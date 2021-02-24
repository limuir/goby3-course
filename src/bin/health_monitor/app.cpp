#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/coroner/groups.h>
#include <goby/middleware/protobuf/coroner.pb.h>
#include <goby/time/convert.h>
#include <goby/time/system_clock.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/health_status.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::HealthMonitor>;
using goby::middleware::protobuf::VehicleHealth;

namespace goby3_course
{
namespace apps
{
class HealthMonitor : public ApplicationBase
{
  public:
    HealthMonitor();

  private:
    void on_health_report(const VehicleHealth& report);
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::HealthMonitor>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::HealthMonitor>(argc, argv));
}

goby3_course::apps::HealthMonitor::HealthMonitor()
{
    interprocess().subscribe<goby::middleware::groups::health_report>(
        [this](const VehicleHealth& health_report) { on_health_report(health_report); });
}

void goby3_course::apps::HealthMonitor::on_health_report(const VehicleHealth& report)
{
    goby3_course::protobuf::HealthStatus health_status_msg;

    switch (report.state())
    {
        case goby::middleware::protobuf::HEALTH__OK:
            health_status_msg.set_state(goby3_course::protobuf::HealthStatus::GOOD);
            break;

        default: health_status_msg.set_state(goby3_course::protobuf::HealthStatus::FAILED); break;
    }
    health_status_msg.set_timestamp_with_units(
        goby::time::SystemClock::now<goby::time::MicroTime>());

    glog.is_verbose() && glog << "Publishing HealthStatus: " << health_status_msg.ShortDebugString()
                              << std::endl;
    intervehicle().publish<goby3_course::groups::health_status>(health_status_msg);
}
