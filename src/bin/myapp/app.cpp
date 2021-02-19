#include <goby/util/debug_logger.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"

using goby::glog;

class MyApp : public goby::zeromq::SingleThreadApplication<config::MyApp>
{
  public:
    MyApp() : goby::zeromq::SingleThreadApplication<config::MyApp>(10 * boost::units::si::hertz)
    {
        glog.is_verbose() && glog << "My configuration value a is: " << cfg().value_a()
                                  << std::endl;
    }

  private:
    void loop() override
    {
        glog.is_verbose() && glog << "This is called 10 times per second" << std::endl;
    }
};

int main(int argc, char* argv[])
{
    return goby::run<MyApp>(goby::middleware::ProtobufConfigurator<config::MyApp>(argc, argv));
}
