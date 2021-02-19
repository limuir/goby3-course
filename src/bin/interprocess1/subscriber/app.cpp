#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/example.pb.h"

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
    Subscriber() : ApplicationBase(1.0 / (10.0 * si::seconds)) {}

  private:
    void loop() override;
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::Subscriber>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::Subscriber>(argc, argv));
}

void goby3_course::apps::Subscriber::loop()
{
    // called at frequency passed to SingleThreadApplication (ApplicationBase)
    glog.is_verbose() && glog << "Loop!" << std::endl;
}
