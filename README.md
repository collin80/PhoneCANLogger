Android Application that connects to ESP32RET devices.
Will log all CAN traffic to your phone. Useful
to be able to use something like a Macchina A2
or EVTV ESP32DUE to log traffic while driving a
car without having to lug a laptop around.

You must configure your CAN buses first with
either SavvyCAN or the serial console. This app
merely uses the existing settings. You must also
have your ESP32 configured to be on the same network
as your phone/tablet.

This program was not exactly engineered to the highest
quality standards. Rather, it was roughly coded until
it more or less worked. Also, setting up Qt to compile
Android apps is a real pain in the ass. If enough
people find this repo and ask maybe I'll code it
up better and put it on the Play store.
