#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/command_dccl.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::CommandTest>;

namespace goby3_course
{
namespace apps
{
class CommandTest : public ApplicationBase
{
  public:
    CommandTest() : ApplicationBase(1.0 / (60.0 * si::seconds)) {}

  private:
    void loop() override;
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::CommandTest>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::CommandTest>(argc, argv));
}

void goby3_course::apps::CommandTest::loop()
{
    glog.is_verbose() && glog << "Command: " << cfg().command().ShortDebugString() << std::endl;
}
