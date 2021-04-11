# BLE-HUD-navigation-ESP32

https://www.facebook.com/story.php?story_fbid=10156820199717852&id=92684932851

![Prototype](/images/IMG_BLE_HUD.png)

## How it works
ESP32 reads navigation instructions from a phone using Bluetooth Low Energy:
* ESP32 acts as BLE Central (also called Master, Client)
* Phone acts as BLE Peripheral (also called Slave, Server)

*Note: roles will be changed in future versions, ESP32 will become Peripheral, phone will become Central.*

### Compatible applications
Currently the only compatible application is Sygic Navigation iOS.

This feature is a prototype, to enable it: Menu / Settings / Info / About / tap 3 times on any item, hidden items appear / BLE HUD.

After enabling, it will be available while the app is in foreground and the screen is on. Note that ESP32 scans only once after power on. 

### Application lifecycle
1. Connection:
   1. ESP32 scans for a Peripheral with BLE service UUID = 91BAD492-B950-4226-AA2B-4EDE9FA42F59
   1. If a device is found, ESP32 connects to it
   1. ESP32 reads application state from the characteristic with UUID = 0D563A58-196A-48CE-ACE2-DFEC78ACC814
   1. If the state is '3' (0x33), ESP32 writes "connect" to the characteristic with UUID = A27590DD-92A1-4362-88D4-490FE00B01F5
   1. Connection done
1. Reading instruction:
   1. ESP32 reads data from the characteristic with UUID = 0D563A58-196A-48CE-ACE2-DFEC78ACC814
   1. Example: 0x690732083335306D00, meaning: instruction (0x69), flags (0x07), current speed limit is 50km/h (0x32), turn right (0x08) in 350m (0x3335306D00)
   1. Flags byte:
     * bit 0 (value & 0x01) - data contains instruction (e.g. turn right)
     * bit 1 (value & 0x02) - data contains speed limit on the current street
     * bit 2 (value & 0x04) - data contains the distance to the instruction
     * bit 4 (value & 0x10) - lane assistant data is available, but need to read another characteristic with UUID = 852905F0-A74A-4CD7-88FE-9FC581049684

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
