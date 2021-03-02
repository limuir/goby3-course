#ifndef GOBY3_COURSE_SRC_BIN_CTD_MACHINE_H
#define GOBY3_COURSE_SRC_BIN_CTD_MACHINE_H

#include <boost/mpl/list.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>

#include <goby/util/debug_logger.h>
#include <goby/util/linebasedcomms/nmea_sentence.h>

#include "goby3-course/groups.h"

namespace goby3_course
{
namespace groups
{
constexpr goby::middleware::Group state_entry{"state_entry"};
constexpr goby::middleware::Group state_exit{"state_exit"};
} // namespace groups

namespace apps
{
class CTDDriver;
}

namespace statechart
{
struct CTDStateMachine;

struct StartLogging;
struct Logging;
struct StopLogging;
struct Sleep;

// events
struct EvDoStartLogging : boost::statechart::event<EvDoStartLogging>
{
};
struct EvLoggingStarted : boost::statechart::event<EvLoggingStarted>
{
};
struct EvDoStopLogging : boost::statechart::event<EvDoStopLogging>
{
};
struct EvEnterSleep : boost::statechart::event<EvEnterSleep>
{
};

template <typename State> void publish_entry(State& state, const std::string& name)
{
    auto& interthread = state.outermost_context().app.interthread();

    goby::middleware::protobuf::TransporterConfig pub_cfg;
    // required since we're publishing in and subscribing to the group within the same thread
    pub_cfg.set_echo(true);
    interthread.template publish<groups::state_entry>(name, {pub_cfg});
}

template <typename State> void publish_exit(State& state, const std::string& name)
{
    auto& interthread = state.outermost_context().app.interthread();
    goby::middleware::protobuf::TransporterConfig pub_cfg;
    pub_cfg.set_echo(true);
    interthread.template publish<groups::state_exit>(name, {pub_cfg});
}

template <typename State> void publish_ctd_out(State& state, const goby::util::NMEASentence& nmea)
{
    auto& interthread = state.outermost_context().app.interthread();
    goby::middleware::protobuf::IOData io_msg;
    io_msg.set_data(nmea.message_cr_nl());
    interthread.template publish<groups::ctd_out>(io_msg);
}

struct CTDStateMachine : boost::statechart::state_machine<CTDStateMachine, Sleep>
{
    CTDStateMachine(apps::CTDDriver& a) : app(a) {}
    apps::CTDDriver& app;
};

struct StartLogging : boost::statechart::state<StartLogging,   // (CRTP)
                                               CTDStateMachine // Parent state (or machine)
                                               >
{
    using StateBase = boost::statechart::state<StartLogging, CTDStateMachine>;

    // entry action
    StartLogging(typename StateBase::my_context c) : StateBase(c)
    {
        publish_entry(*this, "StartLogging");

        goby::util::NMEASentence nmea;
        nmea.push_back("$ZCCMD");
        nmea.push_back("START");
        publish_ctd_out(*this, nmea);
    }

    // exit action
    ~StartLogging() { publish_exit(*this, "StartLogging"); }

    // can have multiple reactions in a list
    typedef boost::mpl::list<
        // when event EvLoggingStarted, transition to Logging
        boost::statechart::transition<EvLoggingStarted, Logging>>
        reactions;
};

struct Logging : boost::statechart::state<Logging, CTDStateMachine>
{
    using StateBase = boost::statechart::state<Logging, CTDStateMachine>;
    Logging(typename StateBase::my_context c) : StateBase(c) { publish_entry(*this, "Logging"); }
    ~Logging() { publish_exit(*this, "StartLogging"); }

    typedef boost::mpl::list<boost::statechart::transition<EvDoStopLogging, StopLogging>> reactions;
};

struct StopLogging : boost::statechart::state<StopLogging, CTDStateMachine>
{
    using StateBase = boost::statechart::state<StopLogging, CTDStateMachine>;
    StopLogging(typename StateBase::my_context c) : StateBase(c)
    {
        publish_entry(*this, "StopLogging");

        goby::util::NMEASentence nmea;
        nmea.push_back("$ZCCMD");
        nmea.push_back("STOP");
        publish_ctd_out(*this, nmea);
    }
    ~StopLogging() { publish_exit(*this, "StopLogging"); }

    typedef boost::mpl::list<boost::statechart::transition<EvEnterSleep, Sleep>> reactions;
};

struct Sleep : boost::statechart::state<Sleep, CTDStateMachine>
{
    using StateBase = boost::statechart::state<Sleep, CTDStateMachine>;
    Sleep(typename StateBase::my_context c) : StateBase(c)
    {
        publish_entry(*this, "Sleep");

        goby::util::NMEASentence nmea;
        nmea.push_back("$ZCCMD");
        nmea.push_back("SLEEP");
        publish_ctd_out(*this, nmea);
    }
    ~Sleep() { publish_exit(*this, "Sleep"); }

    typedef boost::mpl::list<boost::statechart::transition<EvDoStartLogging, StartLogging>>
        reactions;
};

} // namespace statechart
} // namespace goby3_course
#endif
