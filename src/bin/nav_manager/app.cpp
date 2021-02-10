#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/frontseat/groups.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/nav_dccl.pb.h"
#include "goby3-course/nav/convert.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::NavManager>;

namespace goby3_course
{
namespace apps
{
class NavManager : public ApplicationBase
{
  public:
    NavManager();

  private:
    void subscribe_topside_role();
    void subscribe_usv_role();
    void subscribe_auv_role();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[]) { return goby::run<goby3_course::apps::NavManager>(argc, argv); }

goby3_course::apps::NavManager::NavManager()
{
    switch (cfg().role())
    {
        case goby3_course::config::NavManager::AUV: subscribe_auv_role(); break;
        case goby3_course::config::NavManager::USV: subscribe_usv_role(); break;
        case goby3_course::config::NavManager::TOPSIDE: subscribe_topside_role(); break;
    }
}


void goby3_course::apps::NavManager::subscribe_auv_role()
{
}

void goby3_course::apps::NavManager::subscribe_usv_role()
{
    interprocess().subscribe<goby::middleware::frontseat::groups::node_status>(
        [this](const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav) {
            glog.is_verbose() && glog << "Received frontseat NodeStatus: "
                                      << frontseat_nav.ShortDebugString() << std::endl;

            goby3_course::dccl::NavigationReport dccl_nav =
                nav_convert(frontseat_nav, cfg().vehicle_id(), this->geodesy());
            glog.is_verbose() && glog << "^^ Converts to DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            intervehicle().publish<goby3_course::groups::nav>(dccl_nav);
        });
}

void goby3_course::apps::NavManager::subscribe_topside_role()
{
    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    auto& intervehicle_cfg = *subscriber_cfg.mutable_intervehicle();
    for (int v : cfg().subscribe_to_vehicle_id()) intervehicle_cfg.add_publisher_id(v);
    auto& buffer = *intervehicle_cfg.mutable_buffer();
    buffer.set_ack_required(false);
    buffer.set_max_queue(1);
    buffer.set_newest_first(true);

    goby::middleware::Subscriber<goby3_course::dccl::NavigationReport> nav_subscriber(
        subscriber_cfg);

    intervehicle().subscribe<goby3_course::groups::nav, goby3_course::dccl::NavigationReport>(
        [this](const goby3_course::dccl::NavigationReport& dccl_nav) {
            glog.is_verbose() && glog << "Received DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            goby::middleware::frontseat::protobuf::NodeStatus frontseat_nav =
                nav_convert(dccl_nav, this->geodesy());
            glog.is_verbose() && glog << "^^ Converts to frontseat NodeStatus: "
                                      << frontseat_nav.ShortDebugString() << std::endl;

            interprocess().publish<goby::middleware::frontseat::groups::node_status>(frontseat_nav);
        },
        nav_subscriber);
}
