#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/frontseat/groups.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/health_status.pb.h"
#include "goby3-course/messages/nav_dccl.pb.h"
#include "goby3-course/nav/convert.h"
#include "goby3-course/nav/intervehicle.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::TopsideManager>;

namespace goby3_course
{
namespace apps
{
class TopsideManager : public ApplicationBase
{
  public:
    TopsideManager();

  private:
    void subscribe_nav_from_usv();
    void subscribe_usv_health();
    void handle_incoming_nav(const goby3_course::dccl::NavigationReport& dccl_nav);
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::TopsideManager>(argc, argv);
}

goby3_course::apps::TopsideManager::TopsideManager()
{
    glog.add_group("nav", goby::util::Colors::lt_green);
    glog.add_group("health", goby::util::Colors::lt_cyan);
    subscribe_nav_from_usv();
    subscribe_usv_health();
}

void goby3_course::apps::TopsideManager::subscribe_nav_from_usv()
{
    goby::middleware::intervehicle::protobuf::TransporterConfig intervehicle_cfg;
    intervehicle_cfg.add_publisher_id(cfg().usv_modem_id());
    {
        auto& buffer = *intervehicle_cfg.mutable_buffer();
        buffer.set_ack_required(false);
        buffer.set_max_queue(1);
        buffer.set_newest_first(true);

        intervehicle()
            .subscribe<goby3_course::groups::usv_nav, goby3_course::dccl::NavigationReport>(
                [this](const goby3_course::dccl::NavigationReport& dccl_nav) {
                    handle_incoming_nav(dccl_nav);
                },
                goby3_course::nav_subscriber(intervehicle_cfg));
    }

    {
        auto& buffer = *intervehicle_cfg.mutable_buffer();
        buffer.set_ack_required(true);
        buffer.set_max_queue(10);
        buffer.set_newest_first(true);

        intervehicle()
            .subscribe<goby3_course::groups::auv_nav, goby3_course::dccl::NavigationReport>(
                [this](const goby3_course::dccl::NavigationReport& dccl_nav) {
                    handle_incoming_nav(dccl_nav);
                },
                goby3_course::nav_subscriber(intervehicle_cfg));
    }
}

void goby3_course::apps::TopsideManager::subscribe_usv_health()
{
    using goby3_course::groups::health_status_failed;
    using goby3_course::groups::health_status_good;
    using goby3_course::protobuf::HealthStatus;

    auto on_health_status = [](const HealthStatus& health_status_msg) {
        glog.is_verbose() && glog << group("health") << "Received HealthStatus: "
                                  << health_status_msg.ShortDebugString() << std::endl;
    };

    auto on_subscribed = [](const goby::middleware::intervehicle::protobuf::Subscription& sub,
                            const goby::middleware::intervehicle::protobuf::AckData& ack) {
        glog.is_verbose() && glog << group("health")
                                  << "Received acknowledgment: " << ack.ShortDebugString()
                                  << " for subscription: " << sub.ShortDebugString() << std::endl;
    };

    auto get_health_group = [](const HealthStatus& status) -> const goby::middleware::Group& {
        return status.state() == goby3_course::protobuf::HealthStatus::GOOD ? health_status_good
                                                                            : health_status_failed;
    };

    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    subscriber_cfg.mutable_intervehicle()->add_publisher_id(cfg().usv_modem_id());

    intervehicle().subscribe<health_status_good, HealthStatus>(
        on_health_status, {subscriber_cfg, get_health_group, on_subscribed});
    intervehicle().subscribe<health_status_failed, HealthStatus>(
        on_health_status, {subscriber_cfg, get_health_group, on_subscribed});
}

void goby3_course::apps::TopsideManager::handle_incoming_nav(
    const goby3_course::dccl::NavigationReport& dccl_nav)
{
    glog.is_verbose() && glog << group("nav")
                              << "Received DCCL nav: " << dccl_nav.ShortDebugString() << std::endl;

    goby::middleware::frontseat::protobuf::NodeStatus frontseat_nav =
        nav_convert(dccl_nav, this->geodesy());
    if (cfg().has_vehicle_name_prefix())
        frontseat_nav.set_name(cfg().vehicle_name_prefix() + frontseat_nav.name());
    glog.is_verbose() && glog << group("nav") << "^^ Converts to frontseat NodeStatus: "
                              << frontseat_nav.ShortDebugString() << std::endl;

    interprocess().publish<goby::middleware::frontseat::groups::node_status>(frontseat_nav);
}
