#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/frontseat/groups.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/command_dccl.pb.h"
#include "goby3-course/messages/nav_dccl.pb.h"
#include "goby3-course/nav/convert.h"
#include "goby3-course/nav/intervehicle.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::USVManager>;
using goby3_course::dccl::USVCommand;

namespace goby3_course
{
namespace apps
{
class USVManager : public ApplicationBase
{
  public:
    USVManager();

  private:
    void subscribe_our_nav();
    void subscribe_auv_nav();
    void subscribe_commands();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[]) { return goby::run<goby3_course::apps::USVManager>(argc, argv); }

goby3_course::apps::USVManager::USVManager()
{
    glog.add_group("auv_nav", goby::util::Colors::lt_green);
    glog.add_group("usv_nav", goby::util::Colors::lt_blue);
    glog.add_group("commands", goby::util::Colors::red);

    subscribe_our_nav();
    subscribe_auv_nav();
    subscribe_commands();
}

void goby3_course::apps::USVManager::subscribe_our_nav()
{
    interprocess().subscribe<goby::middleware::frontseat::groups::node_status>(
        [this](const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav) {
            glog.is_verbose() && glog << group("usv_nav") << "Received frontseat NodeStatus: "
                                      << frontseat_nav.ShortDebugString() << std::endl;

            goby3_course::dccl::NavigationReport dccl_nav =
                nav_convert(frontseat_nav, cfg().vehicle_id(), this->geodesy());
            glog.is_verbose() && glog << group("usv_nav")
                                      << "^^ Converts to DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            intervehicle().publish<goby3_course::groups::usv_nav>(dccl_nav,
                                                                  goby3_course::nav_publisher());
        });
}

void goby3_course::apps::USVManager::subscribe_auv_nav()
{
    for (int v : cfg().auv_modem_id())
    {
        goby::middleware::intervehicle::protobuf::TransporterConfig intervehicle_cfg;
        intervehicle_cfg.add_publisher_id(v);
        auto& buffer = *intervehicle_cfg.mutable_buffer();
        buffer.set_ack_required(false);
        buffer.set_max_queue(1);
        buffer.set_newest_first(true);

        auto handle_auv_nav = [this](const goby3_course::dccl::NavigationReport& dccl_nav) {
            glog.is_verbose() && glog << group("auv_nav")
                                      << "Received DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            // forward these topside
            intervehicle().publish<goby3_course::groups::auv_nav>(dccl_nav,
                                                                  goby3_course::nav_publisher());
        };

        intervehicle()
            .subscribe<goby3_course::groups::auv_nav, goby3_course::dccl::NavigationReport>(
                handle_auv_nav, goby3_course::nav_subscriber(intervehicle_cfg));
    }
}

void goby3_course::apps::USVManager::subscribe_commands()
{
    using goby3_course::groups::usv_command;
    auto on_command = [this](const USVCommand& command_msg) {
        glog.is_verbose() && glog << group("commands")
                                  << "Received USVCommand: " << command_msg.ShortDebugString()
                                  << std::endl;
        interprocess().publish<usv_command>(command_msg);
    };

    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    subscriber_cfg.mutable_intervehicle()->add_publisher_id(1);
    auto& buffer_cfg = *subscriber_cfg.mutable_intervehicle()->mutable_buffer();
    buffer_cfg.set_ack_required(true);
    buffer_cfg.set_max_queue(1);

    intervehicle().subscribe<usv_command, USVCommand>(on_command, {subscriber_cfg});
}
