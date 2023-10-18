# ESP ATX WoWL

This repository contains the code to program an ESP8255 based controller to read the voltage from pin D1 (use as the power monitor status) and control with current a BJT connected to pin D5 through its base terminal.

A more detailed explanation of the circuit implemented can be found on [my blog post](https://blog.procsiab.net/posts/power-manage-your-motherboard-over-wifi/) on this project.

## Dependencies

Before installing any library, ensure that your IDE has support for the ESP8266 boards: for Arduino IDE, insert the correct JSON file URL inside the additional boards text box, then install the `esp8266` board SDK from the boards manager ([this](https://github.com/esp8266/Arduino#installing-with-boards-manager) is the official guide).

In addition, since the flash tool for ESP platforms is based on Python, a working Python 3 install is required.

To correctly compile this program through Arduino IDE, you should install the following libraries:

- ESPAsyncTCP
- ESPAsyncWebServer
- ArduinoJson
- ESPConnect
- ElegantOTA
- ESPDash
- NTPClient

## Build and upload

The process is the same as for every Arduino-supported microcontroller, as shown in [this guide](https://randomnerdtutorials.com/installing-esp8266-nodemcu-arduino-ide-2-0/) from the inspiring blog RandomNerdTutorials.

## Usage

When powered for the first time, the ESP will spawn an access point with the name `esp-atx-wowl`: you can connect to it without a password, to configure the connection to *your* Wi-Fi network.

After completing the configuration, the ESP will reboot and start serving a web interface on port 80 with a status indicator and two buttons.

To update a new sketch on the air, follow the documentation for the component used to implement this function, [ElegantOTA](https://randomnerdtutorials.com/esp8266-nodemcu-ota-over-the-air-arduino/).

# Warning

The code inside this repo is for demonstration purposes and does not include any authentication mechanisms to protect the information and access to the device when deployed: follow the guidelines for the respective components to implement adequate authentication.

- [Protect OTA firmware updates](https://docs.elegantota.pro/authentication/#example-usage)
- [Protect the dashboard](https://docs.espdash.pro/features/authentication/)
- [Protect the setup captive portal](https://github.com/ayushsharma82/ESPConnect#espconnectautoconnectconst-char-ssid-const-char-password-unsigned-long-timeout)
