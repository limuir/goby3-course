#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/time/convert.h>
#include <goby/time/system_clock.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/health_status.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::Publisher>;

namespace goby3_course
{
namespace apps
{
class Publisher : public ApplicationBase
{
  public:
    Publisher() : ApplicationBase(1.0 * si::hertz) {}

  private:
    void loop() override;
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::Publisher>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::Publisher>(argc, argv));
}

void goby3_course::apps::Publisher::loop()
{
    goby3_course::protobuf::HealthStatus health_status_msg;

    // in a real system we need to determine this from a variety of sources...
    health_status_msg.set_state(goby3_course::protobuf::HealthStatus::GOOD);
    health_status_msg.set_timestamp_with_units(
        goby::time::SystemClock::now<goby::time::MicroTime>());

    glog.is_verbose() && glog << "Publishing HealthStatus: " << health_status_msg.ShortDebugString()
                              << std::endl;
    interprocess().publish<goby3_course::groups::health_status>(health_status_msg);
}
