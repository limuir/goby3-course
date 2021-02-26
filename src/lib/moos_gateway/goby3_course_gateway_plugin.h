#ifndef GOBY3_COURSE_LIB_MOOS_GATEWAY_GOBY3_COURSE_GATEWAY_PLUGIN_H
#define GOBY3_COURSE_LIB_MOOS_GATEWAY_GOBY3_COURSE_GATEWAY_PLUGIN_H

#include <goby/moos/middleware/moos_plugin_translator.h>

#include "goby3-course/groups.h"
#include "goby3-course/messages/nav_dccl.pb.h"

namespace goby3_course
{
namespace moos
{
class IvPHelmTranslation : public goby::moos::Translator
{
  public:
    IvPHelmTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
    {
        goby()
            .interprocess()
            .subscribe<goby3_course::groups::usv_nav, goby3_course::dccl::NavigationReport,
                       goby::middleware::MarshallingScheme::PROTOBUF>(
                [this](const goby3_course::dccl::NavigationReport& usv_nav) {
                    publish_contact_nav_to_moos(usv_nav);
                });
    }

  private:
    void publish_contact_nav_to_moos(const goby3_course::dccl::NavigationReport& nav_report);
};
class CommandTranslation : public goby::moos::Translator
{
  public:
    CommandTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg);
};
} // namespace moos
} // namespace goby3_course

#endif
