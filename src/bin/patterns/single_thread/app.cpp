#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/example.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase =
    goby::zeromq::SingleThreadApplication<goby3::course::config::SingleThreadPattern>;


namespace goby3
{
namespace course
{
namespace apps
{
class SingleThreadPattern : public ApplicationBase
{
  public:
    SingleThreadPattern() : ApplicationBase(1.0 / (10.0 * si::seconds)) {}

  private:
    void loop() override;
};
} // namespace apps
} // namespace course
} // namespace goby3

int main(int argc, char* argv[])
{
    return goby::run<goby3::course::apps::SingleThreadPattern>(argc, argv);
}

void goby3::course::apps::SingleThreadPattern::loop()
{
    // called at frequency passed to SingleThreadApplication (ApplicationBase)
    glog.is_verbose() && glog << "Loop!" << std::endl;
}
