# Bluetooth Low Energy head-up display

https://www.facebook.com/story.php?story_fbid=10156820199717852&id=92684932851

![Prototype](/images/IMG_BLE_HUD.png)

## How it works
The application on the phone sends instructions to ESP32 using Bluetooth Low Energy:
* The phone acts as BLE Central (also called Master, Client)
* ESP32 acts as BLE Peripheral (also called Slave, Server)

### Supported modules
* ESP32 with external OLED display 128x128 (enabled by-default), [how to connect](https://github.com/alexanderlavrushko/BLE-HUD-navigation-ESP32#display-oled-128x128)
* ESP32 TTGO T-Display with embedded TFT 135x240, [how to enable](https://github.com/alexanderlavrushko/BLE-HUD-navigation-ESP32#ttgo-t-display)

### Application iOS
Currently the only compatible application is available on iOS: [GPS Navigation by Tripomatic](https://apps.apple.com/app/gps-navigation-maps/id1206711655).

To enable BLE HUD in the app (it's an unofficial prototype feature):
* Menu / Settings / Info / About / tap 3 times on any item (new line About appears at the top) / About / BLE HUD / Start
* Bluetooth permission required

Notes:
* When enabled, the application automatically scans and connects to ESP32 module (app must be in foreground)
* If connected to ESP32 and the route is set, the app can also send instructions from background
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

### TTGO T-Display
To enable TTGO T-Display:
1. Download library [TFT_eSPI adjusted by TTGO](https://github.com/Xinyuan-LilyGO/TTGO-T-Display), copy TFT_eSPI folder to Arduino/libraries
1. Uncomment display in code [here](https://github.com/alexanderlavrushko/BLE-HUD-navigation-ESP32/blob/7ac0d89f6ebf7e61cd1604369700daa455fdf0a5/ESP32-Arduino/BLEHUDNaviESP32/BLEHUDNaviESP32.ino#L26), and comment out the previous display

### Display OLED 128x128
Display: Waveshare 14747 128x128 OLED RGB ([link](https://www.waveshare.com/1.5inch-rgb-oled-module.htm))

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
