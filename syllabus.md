# Syllabus

## Day 1 (Monday, March 8): Overview

- Lecture: 9 am-11 am EST. Overview of the Goby3 project as a whole, explore some existing and potential applications, and lay down the groundwork for the rest of the week’s technical sessions.
    - Slides: [goby3-course/lectures/day1-overview/2021-goby3-course-day1-overview.pdf][day1 slides]
    - Lecture Notes: [goby3-course/lectures/day1-overview/day1-overview_notes.pdf][day1 notes]
- Homework assignment. Download, setup, and update the course Virtual Machine. Run the multi-vehicle missions that we will explore the rest of the week.
    -  Assignment: [goby3-course/homework/day1-overview/day1-overview_assignment.pdf][day1 assignment]
- Office hours (on Zoom): 1-3 PM and 7-9 PM EST. Check Slack for Zoom links

[day1 slides]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day1-overview/2021-goby3-course-day1-overview.pdf
[day1 notes]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day1-overview/day1-overview_notes.pdf
[day1 assignment]: https://raw.githubusercontent.com/GobySoft/goby3-course/master/homework/day1-overview/day1-overview_assignment.pdf

## Day 2 (Tuesday, March 9): Technical I: Communications

- Lecture: 9 am-11 am EST. Hands-on with nested publish/subscribe in Goby3, from interthread to intervehicle layers. A look into DCCL and Protobuf marshalling schemes. Understanding the Goby intervehicle comms implementation.
    - Slides: None
    - Lecture Notes: [goby3-course/lectures/day2-comms/day2-comms_notes.pdf][day2 notes]
- Homework assignment. Create a command message from the topside to the USV. Add a health monitoring process to the USV that is reported topside.
    - Assignment: [goby3-course/homework/day2-comms/day2-comms_assignment.pdf][day2 assignment]
- Office hours (on Zoom): 1-3 PM and 7-9 PM EST. Check Slack for Zoom links

[day2 notes]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day2-comms/day2-comms_notes.pdf
[day2 assignment]: https://raw.githubusercontent.com/GobySoft/goby3-course/master/homework/day2-comms/day2-comms_assignment.pdf

## Day 3 (Wednesday, March 10): Technical II: Autonomy

- Lecture: 9 am-11 am EST. Integration of Goby3 as a higher level autonomy architecture via the extensible frontseat interface. Interfacing with MOOS-IvP (IvP Helm) autonomy.
    - Slides: [goby3-course/lectures/day3-autonomy/2021-goby3-course-day3-autonomy.pdf][day3 slides]
    - Lecture Notes: [goby3-course/lectures/day3-autonomy/day3-autonomy_notes.pdf][day3 notes]
- Homework assignment. Develop a simple “helm” application and use it to control the vehicle through goby_frontseat_interface. At the end of the mission, command the AUVs to recover.
    - Assignment: [goby3-course/homework/day3-autonomy/day3-autonomy_assignment.pdf][day3 assignment]
- Office hours (on Zoom): 1-3 PM and 7-9 PM EST. Check Slack for Zoom links

[day3 slides]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day3-autonomy/2021-goby3-course-day3-autonomy.pdf
[day3 notes]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day3-autonomy/day3-autonomy_notes.pdf
[day3 assignment]: https://raw.githubusercontent.com/GobySoft/goby3-course/master/homework/day3-autonomy/day3-autonomy_assignment.pdf

## Day 4 (Thursday, March 11): Technical III: Sensors

- Lecture: 9 am-11 am EST. I/O framework for interfacing with sensors (serial, TCP, UDP, CAN). Some Goby3 approaches to managing sensor drivers and data parsing.
    - Slides: [goby3-course/lectures/day4-sensors/2021-goby3-course-day4-sensors.pdf][day4 slides]
    - Lecture Notes: [goby3-course/lectures/day4-sensors/day4-sensors_notes.pdf][day4 notes]
- Homework assignment. Finalize a CTD simulator we started in class, and use it to publish data from the simulated AUVs to the USV.
    - Assignment: [goby3-course/homework/day4-sensors/day4-sensors_assignment.pdf][day4 assignment]
- Office hours (on Zoom): 1-3 PM and 7-9 PM EST. Check Slack for Zoom links


[day4 slides]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day4-sensors/2021-goby3-course-day4-sensors.pdf
[day4 notes]: https://github.com/GobySoft/goby3-course/blob/master/lectures/day4-sensors/day4-sensors_notes.pdf
[day4 assignment]: https://raw.githubusercontent.com/GobySoft/goby3-course/master/homework/day4-sensors/day4-sensors_assignment.pdf