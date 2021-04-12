# BLE-HUD-navigation-ESP32

https://www.facebook.com/story.php?story_fbid=10156820199717852&id=92684932851

![Prototype](/images/IMG_BLE_HUD.png)

## How it works
The application on the phone sends instructions to ESP32 using Bluetooth Low Energy:
* The phone acts as BLE Central (also called Master, Client)
* ESP32 acts as BLE Peripheral (also called Slave, Server)

### Compatible applications
Currently the only compatible application is available on iOS: [GPS Navigation by Tripomatic](https://apps.apple.com/app/gps-navigation-maps/id1206711655).

To enable BLE HUD in the app (it's an unofficial prototype feature):
* Menu / Settings / Info / About / tap 3 times on any item (new line About appears at the top) / About / BLE HUD / Start
* Bluetooth permission required

Notes:
* While the feature is enabled, the application automatically scans and connects to ESP32 module when possible
* Instructions are sent only while the app in foreground and the screen is on (maybe even for a few seconds after going to background)
  * *Sending instructions in background is technically possible, I hope it will be available soon*
* After temporarily going to background, the app restores BLE connection as soon as it becomes active again
* The feature becomes off after restarting the application

### Application lifecycle
1. Connection:
   1. Central (the application on the phone) scans for a Peripheral with wanted service UUID (see Table of UUIDs) - it's going to be our ESP32
   1. The application connects to the first found device
1. Sending instruction:
   1. Central writes new data to the characteristic (see Table of UUIDs) as soon as the data changes (current speed limit or an instruction)
   1. Example: 0x01320A3335306D, meaning: basic data (0x01), current speed limit is 50km/h (0x32), turn right (0x0A) in 350m (0x3335306D, not null-terminated string)
   1. If the app doesn't send an update for a few seconds, ESP32 sends an empty indication (see Table of UUIDs), letting the app know that it wants a data update

### Table of UUIDs
Name | UUID
----- | ---------------
Service | DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86
Characteristic for indicate | DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86
Characteristic for data write | DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86

### Display
Display: Waveshare WS14747 128x128 OLED RGB ([link](https://www.waveshare.com/1.5inch-rgb-oled-module.htm))

Protocol: SSD1351

*Note: there is a great graphics library for this display: [Adafruit-SSD1351-library](https://github.com/adafruit/Adafruit-SSD1351-library), but it's not used in this project.*

Connected this way:
ESP32 | Display WS14747
----- | ---------------
G23 | DIN
G18 | CLK
G5 | CS
G17 | DC
G16 | RST

![Display connection](/images/IMG_Display_connection.jpg)
