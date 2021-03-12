#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/command_dccl.pb.h"
#include "goby3-course/messages/example.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::USVCommandTest>;

namespace goby3_course
{
namespace apps
{
class USVCommandTest : public ApplicationBase
{
  public:
    USVCommandTest() ;

  private:
    void loop() override;
    goby::middleware::Publisher<goby3_course::dccl::USVCommand> make_command_publisher();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::USVCommandTest>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::USVCommandTest>(argc, argv));
}
goby3_course::apps::USVCommandTest::USVCommandTest() : ApplicationBase(1.0 / (1.0 * si::seconds)) {}

goby::middleware::Publisher<goby3_course::dccl::USVCommand> goby3_course::apps::USVCommandTest::make_command_publisher()
{
    using goby::middleware::intervehicle::protobuf::AckData;
    using goby::middleware::intervehicle::protobuf::ExpireData;

    auto on_command_ack = [](const goby3_course::dccl::USVCommand& orig, const AckData& ack_data) {
        glog.is_verbose() && glog << "Our message was acknowledged: " << orig.ShortDebugString()
                                  << "; " << ack_data.ShortDebugString() << std::endl;
    };

    auto on_command_expire = [](const goby3_course::dccl::USVCommand& orig, const ExpireData& expire_data) {
        glog.is_warn() && glog << "Our data expired: " << orig.ShortDebugString() << " Why? "
                               << expire_data.ShortDebugString() << std::endl;
    };

    return {{}, on_command_ack, on_command_expire};
}

void goby3_course::apps::USVCommandTest::loop()
{
    //auto usv_cmd_msg = cfg().command();
    goby3_course::dccl::USVCommand usv_cmd_msg; 
    usv_cmd_msg.set_timestamp_with_units(goby::time::SystemClock::now<goby::time::MicroTime>());
    usv_cmd_msg.set_state(goby3_course::dccl::USVCommand::WAYPOINTS);
    glog.is_verbose() && glog << "Publishing USVCommand Message: " << usv_cmd_msg.ShortDebugString()
                              << std::endl;

    intervehicle().publish<goby3_course::groups::usv_command>(usv_cmd_msg);

    // called at frequency passed to SingleThreadApplication (ApplicationBase)
    // glog.is_verbose() && glog << "Loop!" << std::endl;
}
