#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/multi_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/example.pb.h"

using goby::glog;
namespace si = boost::units::si;
namespace config = goby3_course::config;
namespace groups = goby3_course::groups;
namespace zeromq = goby::zeromq;
namespace middleware = goby::middleware;

namespace goby3_course
{
namespace apps
{
class MultiThreadPattern : public zeromq::MultiThreadApplication<config::MultiThreadPattern>
{
  public:
    MultiThreadPattern();

  private:
    void timer0();
    void loop() override;
};

class SubThreadA : public middleware::SimpleThread<config::MultiThreadPattern>
{
  public:
    SubThreadA(const config::MultiThreadPattern& config);

  private:
    void loop() override;
};

class SubThreadB : public middleware::SimpleThread<config::MultiThreadPattern>
{
  public:
    SubThreadB(const config::MultiThreadPattern& config);

  private:
    void loop() override;
};

} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::MultiThreadPattern>(
        goby::middleware::ProtobufConfigurator<config::MultiThreadPattern>(argc, argv));
}

// Main thread

goby3_course::apps::MultiThreadPattern::MultiThreadPattern()
    : zeromq::MultiThreadApplication<config::MultiThreadPattern>(10 * si::hertz)
{
    glog.add_group("main", goby::util::Colors::yellow);

    // launch our child threads
    launch_thread<SubThreadA>(cfg());
    launch_thread<SubThreadB>(cfg());

    // create a separate timer that goes off every 10 seconds
    launch_thread<middleware::TimerThread<0>>(0.1 * si::hertz);
    interthread().subscribe_empty<middleware::TimerThread<0>::expire_group>(
        std::bind(&MultiThreadPattern::timer0, this));
}

void goby3_course::apps::MultiThreadPattern::loop()
{
    // called at frequency passed to MultiThreadApplication base class
    glog.is_verbose() && glog << group("main") << "Loop!" << std::endl;

    goby3_course::protobuf::Example example_msg;
    example_msg.set_b(5);
    interprocess().publish<groups::example>(example_msg);
}

void goby3_course::apps::MultiThreadPattern::timer0()
{
    glog.is_verbose() && glog << "Timer0" << std::endl;
}

// Subthread A
goby3_course::apps::SubThreadA::SubThreadA(const config::MultiThreadPattern& config)
    : middleware::SimpleThread<config::MultiThreadPattern>(config, 2.0 * si::hertz)
{
    glog.add_group("a", goby::util::Colors::blue);
}

void goby3_course::apps::SubThreadA::loop()
{
    // called at frequency passed to middleware::SimpleThread base class
    glog.is_verbose() && glog << group("a") << "Loop!" << std::endl;
}

// Subthread B
goby3_course::apps::SubThreadB::SubThreadB(const config::MultiThreadPattern& config)
    : middleware::SimpleThread<config::MultiThreadPattern>(config, 1.0 * si::hertz)
{
    glog.add_group("b", goby::util::Colors::magenta);
}

void goby3_course::apps::SubThreadB::loop()
{
    glog.is_verbose() && glog << group("b") << "Loop!" << std::endl;
}
