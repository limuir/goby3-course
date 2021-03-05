# Day 3: Autonomy

Before you start, update your branch to 
`post-lecture3` which includes all the code changes I made during the lecture today:

```bash
cd goby3-course
git fetch
git checkout post-lecture3
```

## Assignment 1: 
**Goal:**  Develop a simple "helm" application and use it to control the vehicle through `goby_frontseat_interface`.

**Task:**

There's a lot of work in autonomy these days and we would like to lower the barrier of entry to get a new autonomy algorithm or system onto real vehicles. Using the `goby_frontseat_interface`, we have a straightforward way to connect new autonomy systems to deployed vehicles.

In this task, you will replace the pHelmIvP-based autonomy system on the USV with a (simple) one of your own. We want to command the USV to:

- At a fixed speed of 1.5 m/s:
  - Follow a fixed course due east for a configurable duration of time.
  - Follow a fixed course due south for a configurable duration of time.
  - Follow a fixed course northwest and stop once the vehicle is no longer getting closer to its starting point.

The result will be (approximately, depending on the durations and currents) a triangle path.

A few suggestions for getting started:

- Create a new SingleThreadApplication, either using the `pattern` files or from scratch. Let's call it `goby3_course_helm`.
- Refer to the `goby_frontseat_interfaces` interface (`goby3/build/share/goby/interfaces/goby_frontseat_interface_interface.yml`):
  - Publish "HELM_DRIVE" to the `helm_state` group of `goby_frontseat_interface`, say at 1 Hz.
  - Publish to the `desired_course` group of `goby_frontseat_interface` for your setpoints, say at 1 Hz.
  - Read (subscribe to) the group `node_status` for the vehicle's current position.
- Disable the IvPHelm code. 
  - Comment out the launch of pHelmIvP in `usv.launch`.
- Add the `goby3_course_helm` to the `usv.launch` file, add a template for its configuration, and enable the configuration generation in `usv.pb.cfy.py`.

You can test the `goby3_course_helm` by running the topside and USV (or the whole mission with `all.launch`) and visualize it on Google Earth or OpenCPN.

## Assignment 2: 

**Goal:** Once your new autonomy engine (Assignment 1) reaches the end of its mission, create a RECOVER command to send to the AUVs. Upon receipt, the AUVs will enter a RECOVER state and come to the surface.

**Task:**

Once our USV completes the third (northwest) leg of its mission and stops, we want to send a command to the AUVs to RECOVER (drive to the surface and aggregate at a single point). 

Your task is to:

- Create a new DCCL message for the AUV command (You can use dccl id 127).
- Publish the AUV command on the USV.
- Modify the IvPHelmTranslation plugin (`src/lib/moos_gateway`) for `goby_moos_gateway` to subscribe for the AUV command and publish an appropriate MOOS variable for recovery.
- Update the AUV behavior file (`launch/trail/config/templates/auv.bhv.in`) to change behavior and recover when the command is received. You can use the StationKeep behavior (<https://oceanai.mit.edu/ivpman/pmwiki/pmwiki.php?n=Helm.BehaviorStationKeep>) along with a ConstantDepth behavior of 0 m (you can either send an update to the existing `deploy_depth` behavior or create a new one that activates only on recovery).  

Make sure you test this with the complete `./all.launch` mission.

## Wrap up

Excellent work - now we've shown that we can develop and deploy our own autonomy behaviors and use the `goby_frontseat_interface` to talk to the actual vehicle. At this point you'd be ready to put it in the water with any vehicle for which you or someone else has written a driver for (currently Iver3, GD Bluefin, and Waveglider SV2 vehicles). 

In addition, we've developed a second command for multivehicle autonomy, where one vehicle reaching a mission goal (the USV reaching its end of mission) can trigger a change in autonomous behavior for a fleet of other vehicles (the AUVs going into recovery).