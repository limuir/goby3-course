#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/frontseat/groups.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/nav_dccl.pb.h"
#include "goby3-course/nav/convert.h"
#include "goby3-course/nav/intervehicle.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::AUVManager>;

namespace goby3_course
{
namespace apps
{
class AUVManager : public ApplicationBase
{
  public:
    AUVManager();

  private:
    void subscribe_our_nav();
    void subscribe_usv_nav();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[]) { return goby::run<goby3_course::apps::AUVManager>(argc, argv); }

goby3_course::apps::AUVManager::AUVManager()
{
    glog.add_group("auv_nav", goby::util::Colors::lt_green);
    glog.add_group("usv_nav", goby::util::Colors::lt_blue);

    subscribe_our_nav();
    subscribe_usv_nav();
}

void goby3_course::apps::AUVManager::subscribe_our_nav()
{
    interprocess().subscribe<goby::middleware::frontseat::groups::node_status>(
        [this](const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav) {
            glog.is_verbose() && glog << group("auv_nav") << "Received frontseat NodeStatus: "
                                      << frontseat_nav.ShortDebugString() << std::endl;

            goby3_course::dccl::NavigationReport dccl_nav =
                nav_convert(frontseat_nav, cfg().vehicle_id(), this->geodesy());
            glog.is_verbose() && glog << group("auv_nav")
                                      << "^^ Converts to DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            intervehicle().publish<goby3_course::groups::auv_nav>(dccl_nav,
                                                                  goby3_course::nav_publisher());
        });
}

void goby3_course::apps::AUVManager::subscribe_usv_nav()
{
    goby::middleware::intervehicle::protobuf::TransporterConfig intervehicle_cfg;
    intervehicle_cfg.add_publisher_id(cfg().usv_modem_id());

    // no need for the USV to unicast its navigation to all the AUVs
    // since we assume they are within broadcast reception of the acoustic modem
    // while trailing the USV
    intervehicle_cfg.set_broadcast(true);

    auto& buffer = *intervehicle_cfg.mutable_buffer();
    buffer.set_ack_required(false);
    buffer.set_max_queue(1);
    buffer.set_newest_first(true);

    auto handle_usv_nav = [this](const goby3_course::dccl::NavigationReport& dccl_nav) {
        glog.is_verbose() && glog << group("usv_nav")
                                  << "Received USV DCCL nav: " << dccl_nav.ShortDebugString()
                                  << std::endl;

        // republish internally on interprocess as Protobuf
        interprocess()
            .publish<goby3_course::groups::usv_nav, goby3_course::dccl::NavigationReport,
                     goby::middleware::MarshallingScheme::PROTOBUF>(dccl_nav);
    };

    intervehicle().subscribe<goby3_course::groups::usv_nav, goby3_course::dccl::NavigationReport>(
        handle_usv_nav, goby3_course::nav_subscriber(intervehicle_cfg));
}
