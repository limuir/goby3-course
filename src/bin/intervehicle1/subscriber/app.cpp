#include <boost/date_time/posix_time/ptime.hpp>

#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/time/convert.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/health_status.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::Subscriber>;

namespace goby3_course
{
namespace apps
{
class Subscriber : public ApplicationBase
{
  public:
    Subscriber();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::Subscriber>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::Subscriber>(argc, argv));
}

goby3_course::apps::Subscriber::Subscriber()
{
    auto on_health_status = [](const goby3_course::protobuf::HealthStatus& health_status_msg) {
        glog.is_verbose() &&
            glog << "Received HealthStatus: " << health_status_msg.ShortDebugString() << std::endl;
        auto timestamp =
            goby::time::convert<boost::posix_time::ptime>(health_status_msg.timestamp_with_units());
        glog.is_verbose() && glog << "Timestamp as date: "
                                  << boost::posix_time::to_simple_string(timestamp) << std::endl;

        auto microseconds_latency =
            std::chrono::microseconds(goby::time::SystemClock::now() -
                                      goby::time::convert<goby::time::SystemClock::time_point>(
                                          health_status_msg.timestamp_with_units()));
        glog.is_verbose() && glog << "Latency (microsec): " << microseconds_latency.count()
                                  << std::endl;
        // do whatever you need to with the message in real code ...
    };

    auto on_subscribed = [](const goby::middleware::intervehicle::protobuf::Subscription& sub,
                            const goby::middleware::intervehicle::protobuf::AckData& ack) {
        glog.is_verbose() && glog << "Received acknowledgment: " << ack.ShortDebugString()
                                  << " for subscription: " << sub.ShortDebugString() << std::endl;
    };

    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    subscriber_cfg.mutable_intervehicle()->add_publisher_id(1);
    auto& buffer_cfg = *subscriber_cfg.mutable_intervehicle()->mutable_buffer();
    buffer_cfg.set_ttl_with_units(5 * boost::units::si::seconds);
    goby::middleware::Subscriber<goby3_course::protobuf::HealthStatus> health_status_subscriber(
        subscriber_cfg, on_subscribed);
    intervehicle()
        .subscribe<goby3_course::groups::health_status, goby3_course::protobuf::HealthStatus>(
            on_health_status, health_status_subscriber);
}
