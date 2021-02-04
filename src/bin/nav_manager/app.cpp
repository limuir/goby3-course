#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/frontseat/groups.h>
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby3-course/nav/convert.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/nav_dccl.pb.h"

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
    void subscribe_auv_role();
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[]) { return goby::run<goby3_course::apps::NavManager>(argc, argv); }

void goby3_course::apps::NavManager::NavManager()
{
    switch (cfg().role())
    {
        case goby3_course::config::NavManager::AUV: subscribe_auv_role(); break;
        case goby3_course::config::NavManager::TOPSIDE: subscribe_topside_role(); break;
    }
}

void goby3_course::apps::NavManager::subscribe_topside_role() {}

void goby3_course::apps::NavManager::subscribe_auv_role()
{
    interprocess().subscribe<goby::middleware::frontseat::groups::node_status>(
        [this](const goby::middleware::frontseat::protobuf::NodeStatus& frontseat_nav) {
            glog.is_verbose() && glog << "Received frontseat NodeStatus: "
                                      << frontseat_nav.ShortDebugString() << std::endl;

            auto dccl_nav = nav_convert(frontseat_nav, cfg().vehicle_id(), this->geodesy());
            glog.is_verbose() && glog << "^^ Converts to DCCL nav: " << dccl_nav.ShortDebugString()
                                      << std::endl;

            // intervehicle().publish<>;
        });
}
