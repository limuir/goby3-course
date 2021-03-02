#include <goby/middleware/marshalling/protobuf.h>
// this space intentionally left blank
#include <goby/middleware/io/line_based/pty.h>
#include <goby/util/linebasedcomms/nmea_sentence.h>
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

constexpr goby::middleware::Group pty_in{"pty_in"};
constexpr goby::middleware::Group pty_out{"pty_out"};

namespace goby3_course
{
namespace apps
{
class CTDSimulator : public middleware::MultiThreadStandaloneApplication<config::CTDSimulator>
{
  public:
    CTDSimulator();

  private:
    void handle_incoming_serial(const goby::util::NMEASentence& nmea);
};

} // namespace apps
} // namespace goby3_course

int main(int argc, char* argv[])
{
    return goby::run<goby3_course::apps::CTDSimulator>(
        goby::middleware::ProtobufConfigurator<config::CTDSimulator>(argc, argv));
}

// Main thread

goby3_course::apps::CTDSimulator::CTDSimulator()
{
    glog.add_group("main", goby::util::Colors::yellow);
    glog.add_group("in", goby::util::Colors::lt_cyan);

    using PTYThread = goby::middleware::io::PTYThreadLineBased<pty_in, pty_out>;
    launch_thread<PTYThread>(cfg().serial());

    interthread().subscribe<pty_in>([this](const goby::middleware::protobuf::IOData& io_msg) {
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
}

void goby3_course::apps::CTDSimulator::handle_incoming_serial(const goby::util::NMEASentence& nmea)
{
    glog.is_verbose() && glog << group("in") << nmea.message() << std::endl;
    if (nmea.sentence_id() == "CMD")
    {
        if (nmea.size() >= 2)
        {
            if (nmea[1] == "START")
            {
                goby::middleware::protobuf::IOData io_msg;
                io_msg.set_data("$ZCACK,START\r\n");
                interthread().publish<pty_out>(io_msg);
            }
            else if (nmea[1] == "STOP")
            {
                goby::middleware::protobuf::IOData io_msg;
                io_msg.set_data("$ZCACK,STOP\r\n");
                interthread().publish<pty_out>(io_msg);
            }
        }
    }
}
