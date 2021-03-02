#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/io/line_based/serial.h>
#include <goby/util/linebasedcomms/nmea_sentence.h>
#include <goby/zeromq/application/multi_thread.h>

#include "config.pb.h"
#include "goby3-course/groups.h"
#include "goby3-course/messages/ctd.pb.h"
#include "machine.h"

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
class CTDDriver : public zeromq::MultiThreadApplication<config::CTDDriver>
{
  public:
    CTDDriver();

  private:
    void loop() override;

    void initialize() override {}

    void finalize() override
    {
        machine_->terminate();
        machine_.reset();
    }

    void handle_incoming_serial(const goby::util::NMEASentence& nmea);

    template <typename State>
    friend void statechart::publish_entry(State& state, const std::string& name);
    template <typename State>
    friend void statechart::publish_exit(State& state, const std::string& name);
    template <typename State>
    friend void statechart::publish_ctd_out(State& state, const goby::util::NMEASentence& nmea);

  private:
    std::unique_ptr<statechart::CTDStateMachine> machine_;
    std::string current_state_;
};

} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::CTDDriver>(
        goby::middleware::ProtobufConfigurator<config::CTDDriver>(argc, argv));
}

// Main thread

goby3_course::apps::CTDDriver::CTDDriver()
    : zeromq::MultiThreadApplication<config::CTDDriver>(1 * si::hertz)
{
    glog.add_group("main", goby::util::Colors::yellow);
    glog.add_group("in", goby::util::Colors::lt_cyan);

    using SerialThread =
        goby::middleware::io::SerialThreadLineBased<goby3_course::groups::ctd_in,
                                                    goby3_course::groups::ctd_out,
                                                    goby::middleware::io::PubSubLayer::INTERTHREAD,
                                                    goby::middleware::io::PubSubLayer::INTERTHREAD>;
    launch_thread<SerialThread>(cfg().serial());

    interthread().subscribe<groups::state_entry>([this](const std::string& state_name) {
        glog.is_verbose() && glog << group("main") << "Entered: " << state_name << std::endl;
        current_state_ = state_name;
    });

    interthread().subscribe<groups::state_exit>([](const std::string& state_name) {
        glog.is_verbose() && glog << group("main") << "Exited: " << state_name << std::endl;
    });

    interthread().subscribe<groups::ctd_in>(
        [this](const goby::middleware::protobuf::IOStatus& status) {
            glog.is_verbose() && glog << group("main")
                                      << "Received I/O status: " << status.ShortDebugString()
                                      << std::endl;
            if (status.state() == goby::middleware::protobuf::IO__LINK_OPEN)
            {
                machine_.reset(new statechart::CTDStateMachine(*this));
                machine_->initiate();
                machine_->process_event(statechart::EvDoStartLogging());
            }
            else if (machine_)
            {
                machine_->terminate();
                machine_.reset();
            }
        });

    interthread().subscribe<groups::ctd_in>(
        [this](const goby::middleware::protobuf::IOData& io_msg) {
            try
            {
                goby::util::NMEASentence nmea(io_msg.data(), goby::util::NMEASentence::VALIDATE);
                handle_incoming_serial(nmea);
            }
            catch (const goby::util::bad_nmea_sentence& e)
            {
                glog.is_warn() && glog << group("in") << "Invalid NMEA sentence: " << e.what()
                                       << std::endl;
            }
        });

    interprocess().subscribe<groups::ctd_control>(
        [this](const goby3_course::protobuf::CTDControl& ctrl_msg) {
            switch (ctrl_msg.desired_state())
            {
                case goby3_course::protobuf::CTDControl::LOGGING:
                    machine_->process_event(statechart::EvDoStartLogging());

                    break;
                case goby3_course::protobuf::CTDControl::NOT_LOGGING:
                    machine_->process_event(statechart::EvDoStopLogging());
                    break;
            }
        });
}

void goby3_course::apps::CTDDriver::loop() {}

void goby3_course::apps::CTDDriver::handle_incoming_serial(const goby::util::NMEASentence& nmea)
{
    glog.is_verbose() && glog << group("in") << nmea.message() << std::endl;
    if (nmea.sentence_id() == "ACK")
    {
        if (nmea.size() >= 2)
        {
            if (nmea[1] == "START")
                machine_->process_event(statechart::EvLoggingStarted());
            else if (nmea[1] == "STOP")
                machine_->process_event(statechart::EvEnterSleep());
        }
    }
}
