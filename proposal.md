# ¯\_(ツ)_/¯

Name of the project: A Very Social Network

Team members and their responsibilities: 
Allison: User Interface and hardware, library implementation 
Kylee: Serial Protocol for wiring and radio waves (creating a way to send messages over radio waves)
Justin: Implement serial protocol to send messages between pi’s without them interfering with each other

Goal of the project: 
We are going to establish a chat messenger interface amongst two (and eventually up to ‘n’) Raspberry Pi’s that transfers information through wires (and potentially radio waves). There will be one wire to which all of the 'chatting' pi's will be connected with one pi's connected computer providing the 3.3 V of power to all of the pi's. We want to use the keyboard and console modules we have developed to display an apple-messenger-esque chat. Each pi will be associated with one keyboard and one monitor. 

Milestone for week 10:
Establish communication between two pis through wire connections (via GPIO pins) 
Create a serial protocol for transferring information
Check that multiple pi’s are not sending information at the same time
Finish setting up a basic version of the user interface to display the messages (much like a messaging system)

Resources needed:
Radio Sensors for the raspberry pi
https://tutorials-raspberrypi.com/raspberry-pi-sensors-overview-50-important-components/
Specifically the section Raspberry Pi Sensors – Wireless / Infrared (IR) / Bluetooth
We are going to test Pat’s radio sensors to see if they will work for our purposes. If they do, we are going to order a set of 10 433 MHz transmitters and receivers from Amazon for two day delivery.
One or two additional pi’s (We currently have four amongst the three of us)
