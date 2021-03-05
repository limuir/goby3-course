#include <boost/circular_buffer.hpp>
#include <boost/units/io.hpp>
#include <numeric>

#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/zeromq/application/single_thread.h>

#include "config.pb.h"
#include "goby/middleware/frontseat/groups.h"
#include "goby/middleware/protobuf/frontseat.pb.h"
#include "goby/time/convert.h"
#include "goby/time/system_clock.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/command_dccl.pb.h"

using goby::glog;
namespace si = boost::units::si;
using ApplicationBase = goby::zeromq::SingleThreadApplication<goby3_course::config::Helm>;

namespace goby3_course
{
namespace apps
{
class Helm : public ApplicationBase
{
  public:
    Helm();

  private:
    void loop() override;
    void update_leg();
    void send_auv_command();

  private:
    enum class Leg
    {
        EAST,
        SOUTH,
        NORTHWEST,
        RECOVER
    };
    goby::time::SystemClock::time_point leg_start_{goby::time::SystemClock::now()};
    Leg leg_{Leg::EAST};

    goby::middleware::frontseat::protobuf::NodeStatus start_position_;
    bool have_start_position_{false};
    goby::middleware::frontseat::protobuf::NodeStatus latest_position_;

    // closest distance to start
    boost::units::quantity<boost::units::si::length> min_dr_;
    // filter last 10 dr values
    boost::circular_buffer<decltype(min_dr_)> last_dr_{10};
};
} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::Helm>(
        goby::middleware::ProtobufConfigurator<goby3_course::config::Helm>(argc, argv));
}

goby3_course::apps::Helm::Helm() : ApplicationBase(1.0 * boost::units::si::hertz)
{
    interprocess().subscribe<goby::middleware::frontseat::groups::node_status>(
        [this](const goby::middleware::frontseat::protobuf::NodeStatus& status) {
            latest_position_ = status;
            if (!have_start_position_)
            {
                start_position_ = status;
                have_start_position_ = true;
            }
        });
}

void goby3_course::apps::Helm::loop()
{
    using boost::units::degree::degrees;
    const auto east = 90.0 * degrees;
    const auto south = 180.0 * degrees;
    const auto northwest = -45 * degrees;

    update_leg();

    // inform the backseat driver we're running and driving.
    goby::middleware::frontseat::protobuf::HelmStateReport state;
    state.set_state(goby::middleware::frontseat::protobuf::HELM_DRIVE);
    interprocess().publish<goby::middleware::frontseat::groups::helm_state>(state);

    // generate desired course command
    goby::middleware::frontseat::protobuf::DesiredCourse course;
    course.set_time_with_units(goby::time::SystemClock::now<goby::time::MicroTime>());
    course.set_speed_with_units(cfg().speed_with_units());
    switch (leg_)
    {
        case Leg::EAST: course.set_heading_with_units(east); break;
        case Leg::SOUTH: course.set_heading_with_units(south); break;
        case Leg::NORTHWEST: course.set_heading_with_units(northwest); break;
        case Leg::RECOVER:
            course.set_speed(0);
            course.set_heading(0);
            break;
    }

    interprocess().publish<goby::middleware::frontseat::groups::desired_course>(course);
}

void goby3_course::apps::Helm::update_leg()
{
    auto now = goby::time::SystemClock::now();
    auto time_on_leg = now - leg_start_;
    glog.is_debug1() &&
        glog << "Time on leg: " << goby::time::convert_duration<goby::time::SITime>(time_on_leg)
             << std::endl;
    switch (leg_)
    {
        case Leg::EAST:
        {
            auto leg_duration = goby::time::convert_duration<goby::time::SystemClock::duration>(
                cfg().east_duration_with_units());
            if (now > leg_start_ + leg_duration)
            {
                leg_start_ = now;
                leg_ = Leg::SOUTH;
                glog.is_verbose() && glog << "Switched to SOUTH leg" << std::endl;
            }
            else
            {
                glog.is_debug1() && glog << "Time left on EAST leg: "
                                         << goby::time::convert_duration<goby::time::SITime>(
                                                leg_duration - time_on_leg)
                                         << std::endl;
            }
        }
        break;
        case Leg::SOUTH:
        {
            auto leg_duration = goby::time::convert_duration<goby::time::SystemClock::duration>(
                cfg().south_duration_with_units());

            if (now > leg_start_ + leg_duration)
            {
                leg_start_ = now;
                leg_ = Leg::NORTHWEST;
                glog.is_verbose() && glog << "Switched to NORTHWEST leg" << std::endl;
            }
            else
            {
                glog.is_debug1() && glog << "Time left on SOUTH leg: "
                                         << goby::time::convert_duration<goby::time::SITime>(
                                                leg_duration - time_on_leg)
                                         << std::endl;
            }
        }
        break;

        case Leg::NORTHWEST:
        {
            // compute range to start
            auto x0 = start_position_.local_fix().x_with_units();
            auto y0 = start_position_.local_fix().y_with_units();
            auto x1 = latest_position_.local_fix().x_with_units();
            auto y1 = latest_position_.local_fix().y_with_units();

            using namespace boost::units;
            auto dr = sqrt((y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0));

            if (last_dr_.empty())
                min_dr_ = dr;
            last_dr_.push_back(dr);

            // wait until buffer is full to start making decisions.
            if (!last_dr_.full())
                return;
            auto dr_avg =
                std::accumulate(last_dr_.begin(), last_dr_.end(), 0.0 * boost::units::si::meters) /
                static_cast<decltype(dr)::value_type>(last_dr_.size());

            if (dr_avg < min_dr_)
            {
                glog.is_debug1() && glog << "Getting closer, dr avg: " << dr_avg << std::endl;
                min_dr_ = dr_avg;
            }
            else
            {
                glog.is_verbose() && glog << "Getting farther away, entering recovery, dr avg: "
                                          << dr_avg << std::endl;
                leg_ = Leg::RECOVER;
                send_auv_command();
            }
        }
        break;

        case Leg::RECOVER:
            break;
    }
}

void goby3_course::apps::Helm::send_auv_command()
{
    goby3_course::dccl::AUVCommand command;
    command.set_time_with_units(goby::time::SystemClock::now<goby::time::MicroTime>());
    command.set_desired_state(goby3_course::dccl::AUVCommand::RECOVER);
    intervehicle().publish<goby3_course::groups::auv_command>(command);
}
