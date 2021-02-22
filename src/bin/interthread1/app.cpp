#include <boost/date_time/posix_time/ptime.hpp>

#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/time/convert.h>
#include <goby/time/system_clock.h>
#include <goby/zeromq/application/multi_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/health_status.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::MultiThreadApplication<goby3_course::config::InterThread1>;
using ThreadBase = goby::middleware::SimpleThread<goby3_course::config::InterThread1>;

namespace goby3_course
{
namespace apps
{
class Publisher : public ThreadBase
{
  public:
    Publisher(const goby3_course::config::InterThread1& config)
        : ThreadBase(config, 1.0 * si::hertz)
    {
    }

  private:
    void loop() override;
};

class Subscriber : public ThreadBase
{
  public:
    Subscriber(const goby3_course::config::InterThread1& config);
};

class InterThread1 : public ApplicationBase
{
  public:
    InterThread1();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::InterThread1>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::InterThread1>(argc, argv));
}

goby3_course::apps::InterThread1::InterThread1()
{
    launch_thread<Subscriber>(cfg());
    launch_thread<Publisher>(cfg());
}

void goby3_course::apps::Publisher::loop()
{
    auto health_status_msg_ptr = std::make_shared<goby3_course::protobuf::HealthStatus>();

    // in a real system we need to determine this from a variety of sources...
    health_status_msg_ptr->set_state(goby3_course::protobuf::HealthStatus::GOOD);
    health_status_msg_ptr->set_timestamp_with_units(
        goby::time::SystemClock::now<goby::time::MicroTime>());

    glog.is_verbose() && glog << "Publishing HealthStatus: "
                              << health_status_msg_ptr->ShortDebugString() << std::endl;
    interprocess().publish<goby3_course::groups::health_status>(health_status_msg_ptr);
}

goby3_course::apps::Subscriber::Subscriber(const goby3_course::config::InterThread1& config)
    : ThreadBase(config)
{
    auto on_health_status =
        [](std::shared_ptr<const goby3_course::protobuf::HealthStatus> health_status_msg_ptr) {
            glog.is_verbose() && glog << "Received HealthStatus: "
                                      << health_status_msg_ptr->ShortDebugString() << std::endl;
            auto timestamp = goby::time::convert<boost::posix_time::ptime>(
                health_status_msg_ptr->timestamp_with_units());
            glog.is_verbose() &&
                glog << "Timestamp as date: " << boost::posix_time::to_simple_string(timestamp)
                     << std::endl;

            auto microseconds_latency =
                std::chrono::microseconds(goby::time::SystemClock::now() -
                                          goby::time::convert<goby::time::SystemClock::time_point>(
                                              health_status_msg_ptr->timestamp_with_units()));
            glog.is_verbose() && glog << "Latency (microsec): " << microseconds_latency.count()
                                      << std::endl;
            // do whatever you need to with the message in real code ...
        };
    interthread().subscribe<goby3_course::groups::health_status>(on_health_status);
}
