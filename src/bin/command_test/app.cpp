#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/command_dccl.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::CommandTest>;
using goby::middleware::Publisher;
using goby3_course::dccl::USVCommand;

namespace goby3_course
{
namespace apps
{
class CommandTest : public ApplicationBase
{
  public:
    CommandTest();

  private:
    void loop() override;

    Publisher<USVCommand> make_command_publisher();

  private:
    Publisher<USVCommand> command_publisher_{make_command_publisher()};
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::CommandTest>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::CommandTest>(argc, argv));
}

goby3_course::apps::CommandTest::CommandTest() : ApplicationBase(1.0 / (60.0 * si::seconds)) {}

Publisher<USVCommand> goby3_course::apps::CommandTest::make_command_publisher()
{
    using goby::middleware::intervehicle::protobuf::AckData;
    using goby::middleware::intervehicle::protobuf::ExpireData;

    auto on_command_ack = [](const USVCommand& orig, const AckData& ack_data) {
        glog.is_verbose() && glog << "Our message was acknowledged: " << orig.ShortDebugString()
                                  << "; " << ack_data.ShortDebugString() << std::endl;
    };

    auto on_command_expire = [](const USVCommand& orig, const ExpireData& expire_data) {
        glog.is_warn() && glog << "Our data expired: " << orig.ShortDebugString() << " Why? "
                               << expire_data.ShortDebugString() << std::endl;
    };

    return {{}, on_command_ack, on_command_expire};
}

void goby3_course::apps::CommandTest::loop()
{
    // augment command line with current time
    auto command = cfg().command();
    command.set_time_with_units(goby::time::SystemClock::now<goby::time::MicroTime>());
    intervehicle().publish<goby3_course::groups::usv_command>(command, command_publisher_);

    glog.is_verbose() && glog << "Sent command: " << command.ShortDebugString() << std::endl;
}
