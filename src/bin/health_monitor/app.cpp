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
    using goby3_course::protobuf::HealthStatus;

    HealthStatus health_status_msg;
    health_status_msg.set_timestamp_with_units(
        goby::time::SystemClock::now<goby::time::MicroTime>());

    auto set_health_group = [](HealthStatus& status, const goby::middleware::Group& group) {
        switch (group.numeric())
        {
            case goby3_course::groups::health_status_good.numeric():
                status.set_state(HealthStatus::GOOD);
                break;
            case goby3_course::groups::health_status_failed.numeric():
                status.set_state(HealthStatus::FAILED);
                break;
        }
    };

    switch (report.state())
    {
        case goby::middleware::protobuf::HEALTH__OK:
        {
            glog.is_verbose() && glog << "Publishing HealthStatus to health_status_good: "
                                      << health_status_msg.ShortDebugString() << std::endl;
            goby::middleware::protobuf::TransporterConfig good_publisher_cfg;
            auto& good_buffer_cfg = *good_publisher_cfg.mutable_intervehicle()->mutable_buffer();
            good_buffer_cfg.set_max_queue(1);
            good_buffer_cfg.set_value_base(50);
            intervehicle().publish<goby3_course::groups::health_status_good>(
                health_status_msg, {good_publisher_cfg, set_health_group});
        }
        break;
        default:
        {
            glog.is_verbose() && glog << "Publishing HealthStatus to health_status_failed: "
                                      << health_status_msg.ShortDebugString() << std::endl;
            goby::middleware::protobuf::TransporterConfig failed_publisher_cfg;
            auto& failed_buffer_cfg =
                *failed_publisher_cfg.mutable_intervehicle()->mutable_buffer();
            failed_buffer_cfg.set_max_queue(1);
            failed_buffer_cfg.set_value_base(500);
            intervehicle().publish<goby3_course::groups::health_status_failed>(
                health_status_msg, {failed_publisher_cfg, set_health_group});
        }
        break;
    }
}
