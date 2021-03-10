# Day 3: Autonomy

Before we begin:

- Revisit any substantial issues related to the communications homework.
- I have pushed my solutions to the branch `post-homework2`

While Goby is primarily a communications project, we want to be able to build autonomous systems with it. This is the second point of the robotics triad as I showed on the first day of the course.

(Switch to slides).

## Frontseat/Backseat State machines

The state of the `goby_frontseat_interface` is effectively governed by the inferred state of the Helm (initially `pHelmIvP` from the MOOS-IvP project, but you could change this to your "helm" which we'll do in the homework), and the state of the frontseat system. Each driver must implement its interpretation of the frontseat states based on the manufacturer's interface.

Helm:
```mermaid
stateDiagram-v2
    [*] --> park
    state running {
        park-->drive
        drive-->park
    }
```

- park: Helm is running but not actively producing desired setpoints
- drive: Helm is in control

FrontSeat:
```mermaid
stateDiagram-v2
    [*] --> idle
    idle-->accepting_commands
    accepting_commands-->idle
    accepting_commands-->frontseat_mission
    frontseat_mission-->accepting_commands
    frontseat_mission-->idle
    idle-->frontseat_mission
```

- idle: frontseat is on but not in a mission
- frontseat_mission: frontseat is running under manufacturer control (payload may be able to listen in on data feeds)
- accepting_commands: payload (Helm) is in control

goby_frontseat_interface:
```mermaid
stateDiagram-v2
    [*] --> idle
    state idle {
        [*] --> standby
        standby --> listen        
    }
    state running{
        listen --> command
        command --> listen
    }
    running--> helm
    running--> fs
    idle --> helm
    idle --> fs
    state error {
        helm
        fs
    }
```

- idle:standby: waiting on data from frontseat
- running:listen: not commanding frontseat (either frontseat is in control via frontseat_mission or helm is in park)
- running:command: payload is commanding frontseat (this is the typical operating state)
- error:helm: Helm has an error
- error:fs: Frontseat has an error.

## Hands-on with goby_frontseat_interface

Let's turn the time warp off to see what's happening at a more reasonable pace.

```python
# launch/trail/config/common/sim.py
warp=1
```

Then launch it

```bash
cd launch/trail
./all.launch
```

### Publish / subscribe interface

If we open the `goby_liaison` scope on the USV <http://localhost:50001/?_=/scope>, we can see all the frontseat messages (`goby::middleware::frontseat::*` groups).

Let's look at these one at a time:

- `data_from_frontseat`: This is a message that encompasses all the data coming from the frontseat. As we're running the `basic_simulator` plugin, this just includes the node_status field (navigation), which is the minimum amount of data the frontseat needs to provide.
- `desired_course`: The desired heading and speed for the surface vehicle (from pHelmIvP). This is converted into a full `command_request` which we'll look at in a bit.
- `helm_state`: The state of the Helm (DRIVE or PARK).
- `node_status`: The `node_status` field (when set) of `data_from_frontseat`is also published on this variable as a convenience.
- `raw_in`: The raw data in to the payload from the frontseat in whatever format the frontseat/backseat interface uses (e.g. NMEA-0183, custom protocol, etc.).
- `raw_out`: The raw data sent from the payload to the frontseat.
- `status`: The state of goby_frontseat_interface, the frontseat state, and the helm state aggregated into a single message.

 The full API (`goby3/build/share/goby/interfaces/goby_frontseat_interface_interface.yml`) contains a few more messages:

```yaml
# all scheme: PROTOBUF so that's removed for clarity here.
# also namespaces removed
application: goby_frontseat_interface
interprocess:
  publishes:
    # response (ack) to command_request messages that have response_requested: true
    - group: command_response
      type: protobuf::CommandResponse
      thread: FrontSeatInterface

    - group: data_from_frontseat
      type: protobuf::InterfaceData
      thread: FrontSeatInterface

    - group: node_status
      type: protobuf::NodeStatus
      thread: FrontSeatInterface

    - group: raw_in
      type: protobuf::Raw
      thread: FrontSeatInterface

    - group: raw_out
      type: protobuf::Raw
      thread: FrontSeatInterface

    - group: status
      type: protobuf::InterfaceStatus
      thread: FrontSeatInterface

    - group: helm_state
      type: HelmStateReport
      thread: FrontSeatTranslation
  subscribes:
    # full command message (can include vehicle-specific extensions)
    - group: command_request
      type: protobuf::CommandRequest
      thread: FrontSeatInterface

    - group: data_to_frontseat
      type: protobuf::InterfaceData
      thread: FrontSeatInterface

    - group: desired_course
      type: protobuf::DesiredCourse
      thread: FrontSeatInterface

    - group: helm_state
      type: protobuf::HelmStateReport
      thread: FrontSeatInterface

    # "backdoor" for applications to send raw commands directly through to the frontseat
    # (generally for debugging or temporary new application development)
    - group: raw_send_request
      type: protobuf::Raw
      thread: FrontSeatInterface

    - group: node_status
      type: protobuf::NodeStatus
      thread: FrontSeatTranslation
```

### Basic Simulator driver

Now let's take a look at a simple implementation of a driver. 

Each driver is an implementation of `goby::middleware::frontseat::InterfaceBase` in `middleware/frontseat/interface.h`. (See also, https://goby.software/3.0/classgoby_1_1middleware_1_1frontseat_1_1InterfaceBase.html)

The driver must implement six virtual methods:

```cpp
// goby3/src/middleware/frontseat.h
    virtual void send_command_to_frontseat(const protobuf::CommandRequest& command) = 0;
    virtual void send_data_to_frontseat(const protobuf::InterfaceData& data) = 0;
    virtual void send_raw_to_frontseat(const protobuf::Raw& data) = 0;

    virtual protobuf::FrontSeatState frontseat_state() const = 0;
    virtual bool frontseat_providing_data() const = 0;
    virtual void loop() = 0;
```

- `send_command_to_frontseat`: Given a command from the payload (at a minimum this is the DesiredCourse: heading, speed, possibly depth, pitch, roll, and z_rate (dive rate)), convert this into a form to send to the frontseat and send it. This is called in response to data from the `command_request` (or `desired_course`) group.
- `send_data_to_frontseat`: Send some data to the frontseat. Not all drivers will have any meaningful data to provide to the frontseat, but this is here for cases when data from sensors attached to the payload is needed by the frontseat. For example if the payload has a USBL array and the frontseat needs that data to update its navigation estimate, the USBL data could be provided through this function. This is called in response to data from the `data_to_frontseat` group.
- `send_raw_to_frontseat`: Directly send a raw message to the frontseat. This is triggered by data from the  `raw_send_request` group.
- `frontseat_state`: Must return the frontseat state, as inferred by the driver from messages received from the frontseat.
- `frontseat_providing_data`: Is the frontseat providing us data. This is vaguely defined to allow drivers to support a wide array of vehicles, but essentially this should be true if we're receiving the expected "core" data set, or false if we're not receiving anything (or not receiving the critical data messages).
- `loop()`: Called at 10 Hz.

Along with these virtual methods, the driver has access to a variety of signals (callbacks) it can or must trigger:

```cpp
    boost::signals2::signal<void(const protobuf::CommandResponse& data)> signal_command_response;
    boost::signals2::signal<void(const protobuf::InterfaceData& data)> signal_data_from_frontseat;
    boost::signals2::signal<void(const protobuf::Raw& data)> signal_raw_from_frontseat;
    boost::signals2::signal<void(const protobuf::Raw& data)> signal_raw_to_frontseat;
```

- `signal_command_response`: Called by the driver when the frontseat responds (acks) a command
- `signal_data_from_frontseat`: Called by the driver when the frontseat provides data.
- `signal_raw_from_frontseat`: Called by the driver for every raw message from the frontseat.
- `signal_raw_to_frontseat`: Called by the driver when it sends a raw message to the frontseat.

The `goby_basic_frontseat_simulator` is a standalone C++ application that implements a very simple dynamic simulator combined with an ASCII payload API for use with `goby_frontseat_interface`.

The API is as follows:
```
> means message from goby_frontseat_interface (backseat) to goby_basic_frontseat_simulator (frontseat)
< means message from goby_basic_frontseat_simulator (frontseat) to goby_frontseat_interface (backseat)

- goby_basic_frontseat_simulator runs a TCP server.
- all messages are "\r\n" terminated

1. simulator init message
(duration = 0 means no timeout)
(duration, freq, accel, hdg_rate, z_rate, warp are optional)
FREQ = control loop frequency in hertz
ACCEL = vehicle acceleration in m/s
HDG_RATE = vehicle turn rate in deg/s
Z_RATE = vehicle dive rate in m/s
WARP = run this factor faster than real time
> START,LAT:42.1234,LON:-72,DURATION:600,FREQ:10,ACCEL:0.5,HDG_RATE:45,Z_RATE:1,WARP:1

2. frontseat state messages
  sent after successful START command
< CTRL,STATE:PAYLOAD 
  sent after DURATION is up in START command
< CTRL,STATE:IDLE

3. frontseat nav command
  generated from primitive dynamics model
< NAV,LAT:42.1234,LON:-72.5435,DEPTH:200,HEADING:223,SPEED:1.4

4. backseat desired course command
> CMD,HEADING:260,SPEED:1.5,DEPTH:100

5. frontseat response to backseat CMD
  CMD is good
< CMD,RESULT:OK
  error in the CMD
< CMD,RESULT:ERROR
```

Given this, we can see how the implementation of the driver is done. We can examine the code (`goby3/src/middleware/frontseat/simulator/basic/basic_simulator_frontseat_driver.*`) while we look at this sequence diagram:

```mermaid
sequenceDiagram
    participant fs as goby_basic_frontseat_simulator
    participant driver as BasicSimulatorFrontSeatInterface
    participant base as InterfaceBase
    driver->>driver: tcp start (constructor)
    driver->>fs: START,...
    driver->>base: signal_raw_to_frontseat(START,...)
    fs->>driver: CTRL,STATE, ...
    driver->>base: signal_raw_from_frontseat(CTRL,STATE,...)
    driver->>driver: update frontseat state
    loop Data / Command Loop
        base->>driver: frontseat_state()
        driver->>base: return: frontseat state
        base->>driver: frontseat_providing_data()
        driver->>base: return: is frontseat providing data
        driver->>driver: loop(): check timeout on frontseat_providing_data
        fs-)driver: NAV,...
        driver->>base: signal_raw_from_frontseat(NAV,...)
        driver->>base: signal_data_from_frontseat: parsed NAV
        driver->>driver: set frontseat_providing_data true
        base-)driver: send_command_to_frontseat
        driver->>fs: CMD,...
        driver->>base: signal_raw_to_frontseat(CMD,...)
        fs->>driver: CMD,RESULT
        driver->>base: signal_raw_from_frontseat(CMD,RESULT,...)
        driver->>base: signal_command_response (if requested)
    end

    loop Optional raw commands
    base->>driver: send_raw_to_frontseat({raw})
    driver->>fs: {raw}
    fs->>driver: CMD,RESULT
    end
```

### Running the frontseat interface

The `goby_frontseat_interface` requires exactly one driver, defined a shared library path in the environmental variable `FRONTSEAT_DRIVER_LIBRARY`.

For convenience, we build wrapper scripts, such as `goby_frontseat_interface_basic_simulator`, which is simply defined (on my computer) as 

```bash
>  cat $(which goby_frontseat_interface_basic_simulator)
#!/bin/bash
LD_LIBRARY_PATH=/home/toby/goby3/build/lib:${LD_LIBRARY_PATH} FRONTSEAT_DRIVER_LIBRARY=libgoby_frontseat_basic_simulator.so.30 exec goby_frontseat_interface $@
```

Graphically, this is how the plugin model works:
```mermaid
flowchart TD
  basic_simulator_frontseat_driver.h & basic_simulator_frontseat_driver.cpp -->|compiler|libgoby_frontseat_basic_simulator.so
  libgoby_frontseat_basic_simulator.so -->|dlopen, runtime, from FRONTSEAT_DRIVER_LIBRARY| goby_frontseat_interface
```

## Goby / MOOS interface

pHelmIvP (in the open source MOOS-IvP project) is a useful multi-objective (e.g. heading, speed, depth) behavior-based decision engine (or "robotic captain" if you're so inclined). Since it's based on the MOOS middleware, we have to create a "bridge" to couple the MOOS middleware to the Goby3 interprocess layer. We do this by defining `goby::moos::Translator` threads, and running them either directly within any MultiThreadApplication, or with the `goby_moos_gateway` application.

### goby::moos::Translator

`goby::moos::Translator` is a shorthand (typedef) for `BasicTranslator<goby::middleware::SimpleThread>`. In short, it's a Goby Thread containing a `CMOOSCommClient`, which roughly the equivalent in MOOS to a Goby `InterProcessPortal`.

```mermaid
flowchart TD
   subgraph goby_moos_gateway or other MultiThreadApplication
   subgraph Translator
      CMOOSCommClient
      CMOOSCommClient <-->|your code| SimpleThread 
      SimpleThread
   end
   end
   CMOOSCommClient <--> MOOSDB
   pHelmIvP <--> MOOSDB
   SimpleThread <--> gobyd
   goby_app1 <--> gobyd
   goby_app2 <--> gobyd
```

You can have multiple Translators within a given application for different purposes if you want:

```mermaid
flowchart TD
   subgraph goby_moos_gateway or other MultiThreadApplication
   subgraph FrontSeatTranslator
      CMOOSCommClient1[CMOOSCommClient]
      CMOOSCommClient1 <-->|your code| SimpleThread1
      SimpleThread1[SimpleThread]
   end
   subgraph MOOSSensorsTranslator
      CMOOSCommClient2[CMOOSCommClient]
      CMOOSCommClient2 <-->|your code| SimpleThread2
      SimpleThread2[SimpleThread]
   end
   end
   CMOOSCommClient1 <--> MOOSDB
   CMOOSCommClient2 <--> MOOSDB
   pHelmIvP <--> MOOSDB
   iSensor <--> MOOSDB
   SimpleThread1 <--> gobyd
   SimpleThread2 <--> gobyd
```

Let's take a look at the interface for the BasicTranslator <https://goby.software/3.0/classgoby_1_1moos_1_1BasicTranslator.html>:

```cpp
SimpleThread<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>& goby()
std::string translator_name()
MOOSInterface& moos()
void loop()
 ```

Pretty straightforward: when we want to get call Goby stuff we call `goby()` and MOOS stuff is in `moos()`. The Goby side we should be fairly familiar at this point. On the MOOS side we have a few options:

```cpp
class MOOSInterface
{
  // when we receive this MOOS variable, call this function (acts much like the Goby subscription callbacks)
  void add_trigger(const std::string& moos_var,
                   const std::function<void(const CMOOSMsg&)>& func);
  // subscribe to the MOOS variable, and keep its latest value in a buffer.
  void add_buffer(const std::string& moos_var) { buffer_vars_.insert(moos_var); }
  // access the buffered values
  std::map<std::string, CMOOSMsg>& buffer() { return buffer_; }
  // access the CMOOSCommClient (Register, Notify, Post, etc.)
  CMOOSCommClient& comms() { return comms_; }
};
```

Now that we have that, let's look at it in action with the `FrontSeatTranslation` that interfaces the core `pHelmIvP` messages with Goby. This resides in `goby3/src/moos/middleware/frontseat/frontseat_gateway_plugin.h`.

```mermaid
flowchart TD
    subgraph MOOS/Goby
    subgraph goby_moos_gateway
    FrontSeatTranslation
    end
    end
    subgraph MOOS
    pHelmIvP[pHelmIvP]
    pHelmIvP -->|DESIRED_*, IVPHELM_STATE| FrontSeatTranslation 
    FrontSeatTranslation -->|NAV_*| pNodeReporter
    pNodeReporter -->|NODE_REPORT| pHelmIvP
    end
    subgraph Goby
    goby_frontseat_interface -->|node_status| FrontSeatTranslation
    FrontSeatTranslation -->|helm_state, desired_course| goby_frontseat_interface
    end
```

Much like `goby_frontseat_interface`, `goby_moos_gateway` uses an environmental variable (`GOBY_MOOS_GATEWAY_PLUGINS`) to load a list of plugin libraries (delimited by any of ";:,"), for example:

```bash
GOBY_MOOS_GATEWAY_PLUGINS=liblamss_goby3_gateway_plugin.so.0:libgoby_coroner_moos_gateway_plugin.so.30  goby_moos_gateway
```


Since `FrontSeatTranslation` is just a Goby Thread, we can simplify this for a common scenario by also including it in `goby_frontseat_interface`. This is done if we include the `[goby.moos.protobuf.moos_helm] {}` block of the `goby_frontseat_interface` configuration (see `goby_frontseat_interface_basic_simulator --example_config`).

In this case (which is what we're running in the Trail example), the structure looks like this:

```mermaid
flowchart TD
    
    subgraph Goby
    subgraph goby_frontseat_interface
    FrontSeatTranslation
    driver[frontseat::InterfaceBase implementation]
    end
    end
    subgraph MOOS
    pHelmIvP[pHelmIvP]
    pHelmIvP -->|DESIRED_*, IVPHELM_STATE| FrontSeatTranslation 
    FrontSeatTranslation -->|NAV_*| pNodeReporter
    pNodeReporter -->|NODE_REPORT| pHelmIvP
    end
    subgraph Goby
    driver -->|node_status| FrontSeatTranslation
    FrontSeatTranslation -->|helm_state| driver
    end
    driver <--> frontseat
```

## Command pHelmIvP through Goby

Yesterday, I had you add a command message from the topside to the USV. Now we'll walk through how to get that command over to pHelmIvP and actually change the vehicle's behavior.

First let's take a look at the command message that I designed (yours may look slightly different):

```protobuf
// src/lib/messages/command_dccl.proto
syntax = "proto2";

import "dccl/option_extensions.proto";

package goby3_course.dccl;

message USVCommand
{
    option (.dccl.msg) = {
        codec_version: 3
        id: 126
        max_bytes: 32
        unit_system: "si"
    };

    required double time = 1 [(.dccl.field) = {
        codec: "dccl.time2",
        units {derived_dimensions: "time"}
    }];

    enum AutonomyState
    {
        WAYPOINTS = 1;
        POLYGON = 2;
    }
    required AutonomyState desired_state = 2;

    optional int32 polygon_sides = 3 [(.dccl.field) = {min: 3 max: 10}];
    optional int32 polygon_radius = 4
        [(.dccl.field) = {min: 100 max: 2000 precision: -1}];
}
```

Now in `goby3_course_usv_manager` I've subscribed to this message coming from the topside:

```cpp
// src/bin/manager/usv/app.cpp
void goby3_course::apps::USVManager::subscribe_commands()
{
    auto on_command = [](const USVCommand& command_msg) {
        glog.is_verbose() && glog << group("commands")
                                  << "Received USVCommand: " << command_msg.ShortDebugString()
                                  << std::endl;
    };

    goby::middleware::protobuf::TransporterConfig subscriber_cfg;
    subscriber_cfg.mutable_intervehicle()->add_publisher_id(1);
    auto& buffer_cfg = *subscriber_cfg.mutable_intervehicle()->mutable_buffer();
    buffer_cfg.set_ack_required(true);
    buffer_cfg.set_max_queue(1);

    using goby3_course::groups::usv_command;
    intervehicle().subscribe<usv_command, USVCommand>(on_command, {subscriber_cfg});
}
```

Let's run this and see how it works. First let's enable VERBOSE glog output on the USV manager:

```
# usv.launch
# ...
goby3_course_usv_manager <(config/usv.pb.cfg.py goby3_course_usv_manager) -v -n
```

Then run the code:

```bash
goby_launch -P topside.launch
goby3_course_n_auvs=0 goby_launch -P usv.launch
screen -r usv.goby3_course_usv_manager
```

Then if we open Goby Liaison <http://localhost:50000/?_=/commander> we can send the command.

After we wait until the next comms cycle, we get an ACK and we see the command show up at `goby3_course_usv_manager`.

We could now proceed in a few different ways:

- subscribe directly in our plugin on intervehicle to `usv_command`. This is a reasonable approach, though I've been finding it helpful to keep most of the intervehicle subscriptions to a single application.
- run the `goby::moos::Translator` subclass as a thread directly within `USVManager` (after converting `USVManager` to a MultiThreadApplication). Also a reasonable approach, and this is similar to how `goby_frontseat_interface` is handling the MOOS translations. It depends on how much you want to spread out or consolidate the entry/exit points between Goby and MOOS.
- republish the command on `interprocess` from `goby3_course_usv_manager` so we can subscribe to it from our `goby_moos_gateway` plugin (based on `goby::moos::Translator`).

We'll choose the last technique:
```mermaid
flowchart TD
   subgraph topside
       goby_liaison
   end
   goby_liaison -->|usv_command|goby3_course_usv_manager

   subgraph usv
   goby3_course_usv_manager -->|usv_command| SimpleThread1
   subgraph goby_moos_gateway
   subgraph CommandTranslator
      CMOOSCommClient1[CMOOSCommClient]
      SimpleThread1 -->|convert| CMOOSCommClient1
      SimpleThread1[SimpleThread]
   end
   end
   CMOOSCommClient1 -->|DEPLOY_STATE, POLYGON_UPDATES| pHelmIvP
   end
```

So, republish the command on **interprocess** within the USV:

```cpp
// src/bin/manager/usv/app.cpp
void goby3_course::apps::USVManager::subscribe_commands()
{
    using goby3_course::groups::usv_command;
    auto on_command = [this](const USVCommand& command_msg) {
        // ...
        interprocess().publish<usv_command>(command_msg);
    };
}
```

Then, we can create our plugin library. We already have one (`src/lib/moos_gateway/goby3_course_gateway_plugin.*`) that is in use by the AUVs (to publish the `usv_nav` as a `NODE_REPORT` for the pHelmIvP trail behavior), so we can augment it with our additional data for the commands.

Let's add our new Translation thread:

```cpp
// src/lib/moos_gateway/goby3_course_gateway_plugin.h
namespace goby3_course
{
namespace moos
{
// ... 
class CommandTranslation : public goby::moos::Translator
{
  public:
    CommandTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg);
};
} // namespace moos
} // namespace goby3_course
```

```cpp
#include "goby3-course/messages/command_dccl.pb.h"
// src/lib/moos_gateway/goby3_course_gateway_plugin.cpp
extern "C"
{
    void goby3_moos_gateway_load(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
 //...
        handler->launch_thread<goby3_course::moos::CommandTranslation>();
    }

    void goby3_moos_gateway_unload(
        goby::zeromq::MultiThreadApplication<goby::apps::moos::protobuf::GobyMOOSGatewayConfig>*
            handler)
    {
 //...
        handler->join_thread<goby3_course::moos::CommandTranslation>();
    }
}

// ...

goby3_course::moos::CommandTranslation::CommandTranslation(const goby::apps::moos::protobuf::GobyMOOSGatewayConfig& cfg)
        : goby::moos::Translator(cfg)
{
    using goby3_course::dccl::USVCommand;
    using goby3_course::groups::usv_command;

    auto on_usv_command = [this](const USVCommand& command) {
        // send update first
        if (command.desired_state() == USVCommand::POLYGON)
        {
            std::stringstream update_ss;
            update_ss << "polygon=radial::x=0,y=0,radius=" << command.polygon_radius()
                      << ",pts=" << command.polygon_sides();
            moos().comms().Notify("POLYGON_UPDATES", update_ss.str());
        }
        moos().comms().Notify("DEPLOY_STATE",
                              USVCommand::AutonomyState_Name(command.desired_state()));
    };
    goby().interprocess().subscribe<usv_command>(on_usv_command);
}
```

Reference <https://oceanai.mit.edu/ivpman/pmwiki/pmwiki.php?n=Helm.BehaviorLoiter> for the Loiter behavior. Update the behavior file for the USV:

```
// launch/trail/config/templates/usv.bhv.in
initialize   DEPLOY_STATE = POLYGON
// ...
//----------------------------------------------
Behavior = BHV_Waypoint
{ 
// ...
  condition = DEPLOY_STATE = WAYPOINTS
}
Behavior = BHV_Loiter
{
   name         = polygon
   pwt          = 100
   condition    = DEPLOY_STATE=POLYGON
   updates      = POLYGON_UPDATES

   center_activate = true     
   speed = 1.5

   polygon = radial:: x=0,y=0,radius=200,pts=6
}
```

Add `goby_moos_gateway` to `usv.launch`:

```
# usv.launch

[env=LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$HOME/goby3-course/build/lib,env=GOBY_MOOS_GATEWAY_PLUGINS=libgoby3_course_moos_gateway_plugin.so] goby_moos_gateway <(config/usv.pb.cfg.py goby_moos_gateway) -vv
```

And finally, add generation in `usv.pb.cfg.py`:

```python
# launch/trail/config/usv.pb.cfg.py
if common.app == 'gobyd':    
# ...
elif common.app == 'goby_moos_gateway':
    print(config.template_substitute(templates_dir+'/moos_gateway.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     moos_port=common.vehicle.moos_port(vehicle_id)))
```

Now we can rerun the topside and the USV.

Open uMS to port 9001 to see what is published to the MOOSDB, and send a "WAYPOINTS" command from Liaison.

Let's bump up the WARP again:

```python
# launch/trail/config/common/sim.py
warp=10
```

And then run it all: `./all.launch`.


## Extras (Optional, based on time)

### Time in Goby

Switch back the HealthStatus message that we were looking at yesterday.

We can improve this message by adding a timestamp to it, and at the same time look at how time is handled in Goby.

Time in C++11 and newer is handled by the `std::chrono` library. Unfortunately, date handling isn't provided until C++20, and we're targeting C++14 in Goby3. Additionally, we need a serialized representation of a point in time or duration for use in Protobuf messages, etc. So, we these three related but slightly diverging concepts, we arrive at these needs and the Goby3 choice of solution:

  - point in time (e.g. right now) or duration (`duration`, or difference between two `time_point`s, e.g. 2 hours, 5 minutes) -> `std::chrono`.
  - date representation -> `boost::posix_time::ptime`
  - serializable representation:
    - For points in time: seconds (double) or microseconds (int64_t or uint64_t) since the UNIX epoch (1 January, 1970, midnight UTC) -> `boost::units::quantity<...time>`, typedef'd to `goby::time::MicroTime` (for `int64_t` microseconds) and `goby::time::SITime` (for `double` seconds)
    - For durations: seconds (double) or microseconds (int64_t)  -> `goby::time::MicroTime`, `goby::time::SITime`

All of these representations can be seamlessly converted between using the `goby::time::convert()` and `goby::time::convert_duration()` family of functions provided in `goby/time/convert.h`. You may notice that there's no difference in the serializable time representations for points of time and durations, which is the main reason that `convert()` and `convert_duration()` exist. In this case, the context or message field name will indicate type of time is being used (e.g. `message_timestamp` indicates a point in time, versus `deploy_duration` would be a duration).

The DCCL library has an integration with the `boost::units` library, which makes it a useful way to safely set and retrieve dimensioned fields of DCCL or "vanilla" Protobuf messages, using the additional `_with_units()` methods that DCCL adds to Protobuf. We used DCCL a bit yesterday when we did the **intervehicle** section, but for now, we'll look again at the units part.

Let's add a timestamp to our HealthStatus message:

```protobuf
// health_status.proto
import "dccl/option_extensions.proto";
//...
message HealthStatus
{
// ...
    option (dccl.msg).unit_system = "si";
    required HealthState state = 1;
    required uint64 timestamp = 2
        [(dccl.field).units = { prefix: "micro" base_dimensions: "T" }];
}
```

Without going into great detail (see libdccl.org for more detail), this indicates we are using the SI system for unit definitions, and tagging the `timestamp` field as having the dimensions of time (T), which is seconds in SI, and the `prefix`  parameter makes this microseconds. By convention we treat this as "microseconds since the UNIX epoch."

Now, back in our interprocess1 publisher code, we can add the current timestamp to this message. Writing this out fully, we get:

```cpp
#include <goby/time/system_clock.h>
#include <goby/time/convert.h>
// ...
void goby3_course::apps::Publisher::loop()
{
    goby3_course::protobuf::HealthStatus health_status_msg;
    // ...
    goby::time::SystemClock::time_point now = goby::time::SystemClock::now();
    auto now_microseconds_since_unix = goby::time::convert<goby::time::MicroTime>(now);
    health_status_msg.set_timestamp_with_units(now_microseconds_since_unix);
}
```

`goby::time::SystemClock` is a thin wrapper around `std::chrono::system_clock` that provides the ability to run the "real clock" at some factor of the real time for simulation purposes (referred to as "warping"). This is helpful and this I would suggest always using this for Goby applications. That said, `std::chrono` types will work fine with Goby as well, if you're willing to forgo the faster-than-realtime functionality.

This is a lot to type every time we want to set our timestamps, so `SystemClock` has a template overload for `now()` that can take any type that is convertible by the `convert()` function family. Using that, we end up with the equivalent, much cleaner:

```cpp
health_status_msg.set_timestamp_with_units(goby::time::SystemClock::now<goby::time::MicroTime>());
```

For the interprocess1 subscriber, we may wish to read that timestamp and extract the date. To do so, we can use the `boost::posix_time::ptime` class (which Goby will replace in the future with std::chrono in C++20):

```cpp
#include <boost/date_time/posix_time/ptime.hpp>

#include <goby/time/convert.h>
//...

auto on_health_status = [](const goby3_course::protobuf::HealthStatus& health_status_msg) {
        auto timestamp =
            goby::time::convert<boost::posix_time::ptime>(health_status_msg.timestamp_with_units());
        glog.is_verbose() && glog << "Timestamp as date: "
                                  << boost::posix_time::to_simple_string(timestamp) << std::endl;
};
```

Now when we rerun our code, we will see the current time as microseconds since UNIX included in the message.

```bash
gobyd
goby3_course_interprocess1_publisher -v
goby3_course_interprocess1_subscriber -v
```

As one final example, we can calculate the approximate message latency:

```cpp
auto microseconds_latency =
    std::chrono::microseconds(goby::time::SystemClock::now() -
                              goby::time::convert<goby::time::SystemClock::time_point>(
                                  health_status_msg.timestamp_with_units()));
glog.is_verbose() && glog << "Latency (microsec): " << microseconds_latency.count()
                          << std::endl;
```


### goby_clang_tool

The `goby_clang_tool` has two actions it can perform:

- *generate* an interface file for an application
- *visualize* a deployment using multiple applications' interface files.

#### Generate

During the *generate* action, the `goby_clang_tool` is a tool that uses the Clang compiler's libtooling library to process the C++ code and extract the `publish` and `subscribe` calls from the code. From here, it can produce an "interface" file for that application, which provides a complete list of all the publications and subscriptions for that application.

In this course, we build these interface files (which are defined in YAML) to `build/share/interfaces`. Let's take a look at the one for `goby3_course_auv_manager`:

```yaml
# build/share/interfaces/goby3_course_auv_manager_interface.yml
application: goby3_course_auv_manager
intervehicle:
  publishes:
    - group: goby3_course::auv_nav;2
      scheme: DCCL
      type: goby3_course::dccl::NavigationReport
      thread: goby3_course::apps::AUVManager
  subscribes:
    - group: goby3_course::usv_nav;1
      scheme: DCCL
      type: goby3_course::dccl::NavigationReport
      necessity: optional
      thread: goby3_course::apps::AUVManager
interprocess:
  publishes:
    - group: goby3_course::usv_nav;1
      scheme: PROTOBUF
      type: goby3_course::dccl::NavigationReport
      thread: goby3_course::apps::AUVManager
# ... omitting coroner & terminate groups
    - group: goby3_course::auv_nav;2
      scheme: DCCL
      type: goby3_course::dccl::NavigationReport
      thread: goby3_course::apps::AUVManager
      inner: true
    - group: goby3_course::auv_nav;2
      scheme: PROTOBUF
      type: goby3_course::dccl::NavigationReport
      thread: goby3_course::apps::AUVManager
      inner: true
  subscribes:
    - group: goby::middleware::frontseat::node_status
      scheme: PROTOBUF
      type: goby::middleware::frontseat::protobuf::NodeStatus
      necessity: optional
      thread: goby3_course::apps::AUVManager
# ... omitting coroner & terminate groups
interthread:
# not meaningful in a SingleThreadApplication
```

Here we see a useful summary of all of the publications and subscriptions for this application, based on the Clang compiler's understanding of our code. Thus, if we change anything, all we need to do is recompile to regenerate this file. 

This kind of tool can be used to avoid "documentation rot" (where documentation lags implementation and thus becomes less than helpful), as well as static analysis (compile time verifications).

Using CMake, we can use the `generate_interfaces(${APP})` function to call `goby_clang_tool` using the generate action to create the interfaces file on the provided target `${APP}`. This function (and others) are defined in the `cmake/GobyClangTool.cmake` file.

We can add this to the goby_moos_gateway plugin library:

```cmake
# src/lib/moos_gateway/CMakeLists.txt

if(export_goby_interfaces)
  generate_interfaces(${LIB})
endif()
```

#### Visualize

Once we have a collection of interface files for our applications, we can set up a "deployment", which is just a list of which applications we intend to run when we deploy this code. The deployment is defined in YAML (but by us, not the compiler, as it has no idea where we want to run this code), and for the Trail example resides in `launch/trail/trail_deployment.yml`:

```yaml
# launch/trail/trail_deployment.yml
deployment: trail_deployment
platforms:
  - name: topside
    interfaces:
      - goby3_course_topside_manager_interface.yml
      - @GOBY_INTERFACES_DIR@/goby_opencpn_interface_interface.yml
      - @GOBY_INTERFACES_DIR@/goby_geov_interface_interface.yml
  - name: usv
    interfaces:
      - goby3_course_usv_manager_interface.yml
      - @GOBY_INTERFACES_DIR@/goby_frontseat_interface_interface.yml
  - name: auv0
    interfaces:
      - goby3_course_auv_manager_interface.yml
      - @GOBY_INTERFACES_DIR@/goby_frontseat_interface_interface.yml
  - name: auvN
    interfaces:
      - goby3_course_auv_manager_interface.yml
      - @GOBY_INTERFACES_DIR@/goby_frontseat_interface_interface.yml
```

This file provides the paths to the appropriate interface files. Using CMake, we can call the following to run `goby_clang_tool` in visualize mode (as well as the Graphviz `dot` tool)

```cmake
generate_interfaces_figure(launch/trail/trail_deployment.yml ${YML_OUT_DIR} trail_interfaces.pdf "--no-disconnected")
```

CMake preprocesses the deployment file to replace `@GOBY_INTERFACES_DIR@` with the actual path to the interface files in Goby3. Then `goby_clang_tool` uses this deployment file, along with the referenced interface files, to produce a Graphviz "dot" file. These "dot" files are then processed by Graphviz to generate a PDF or other supported output file that visualizes the connections (i.e. matching publish/subscribe calls).

Add all the interfaces for the code we've added in the last few days:

```yaml
deployment: trail_deployment
platforms:
  - name: topside
    interfaces:
...
      - goby3_course_command_test_interface.yml
  - name: usv
    interfaces:
...
      - @GOBY_INTERFACES_DIR@/goby_coroner_interface.yml
      - goby3_course_usv_health_monitor_interface.yml        
```

And rebuild to see the new applications.

#### Current limitations

The tool currently has no way of knowing about the network topology, so any matching publish/subscribe pairs are connected on the visualization, even if the two nodes won't be on the same physical link (e.g. topside and AUVs in our Trail example). As such you will see a direct **intervehicle** link from `goby3_course_auv_manager` to `goby3_course_topside_manager` that doesn't actually exist based on how we've created our network.

This will be solved in the future by augmenting the deployment file to include this information.

