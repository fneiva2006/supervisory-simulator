# Industrial supervisory system simulator

This college assessment project consists of a simulated industrial supervisory system. The idea is to show how it is possible to integrate many industrial devices and systems of different hierarchy together.

This project contains different processes that are responsible for simulating the execution of specific tasks of an industrial pipeline. These processes should communicate with each other in real time using different types of technologies and protocol. In the case of this assessment, it is shown by the use of semaphores, mutexes, shared memory, statcks, windows pipes and other Inter Process Communication (IPC) tools.

The entry point for the program is the executable *p5_user.exe*. It will open the main supervisory system and start up the simulation of the other distributed industrial devices.