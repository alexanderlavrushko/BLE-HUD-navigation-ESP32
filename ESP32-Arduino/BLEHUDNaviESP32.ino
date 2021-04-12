#include "BLEDevice.h"
#include <SPI.h>
#include "OLED.h"
#include "ImagesOther.h"
#include "ImagesDirections.h"
#include "ImagesLanes.h"
#include "Font8x8.h"
#include "GlyphShifter.h"
#include "SygicConstants.h"


const byte MIN_CHAR_WIDTH = 3;
const byte SPACE_BETWEEN_CHARS = 1;
enum DrawingMode
{
    DrawingModeNormal = 0,
    DrawingModeMirror,
    DrawingMode180,
    DrawingMode180Mirror,
    DrawingModeCount
};


#define PIN_BUTTON 19


const int spiClk = 32000000; // 32 MHz (40 MHz causes problems, about 1% of bytes are not received by display)

uint16_t g_canvas[128 * 128] = {};
DrawingMode g_drawingMode = DrawingModeNormal;
bool g_isButtonPressed = false;

// The remote service we wish to connect to.
BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// The characteristic of the remote service we are interested in.
BLEUUID    charPingUUID("0d563a58-196a-48ce-ace2-dfec78acc814");
BLEUUID    charUUID("A27590DD-92A1-4362-88D4-490FE00B01F5");
BLEUUID    charLanesUUID("852905F0-A74A-4CD7-88FE-9FC581049684");

const int BACKGROUND_DEVICES_SIZE = 16;
int nBackgroundDevicesCount = 0;
BLEAdvertisedDevice* ppBackgroundDevices[BACKGROUND_DEVICES_SIZE] = {};

BLEAddress *pServerAddress = nullptr;
BLEAdvertisedDevice* myDevice = nullptr;
bool doConnect = false;
bool connected = false;

BLERemoteCharacteristic* pRemoteCharacteristicPing = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristicLanes = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;

std::string g_laneData;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("MyClientCallback onDisconnect");
  }
};

bool connectToServer(BLEAdvertisedDevice* deviceToConnect) {
    Serial.print("Forming a connection to ");
    Serial.println(deviceToConnect->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(deviceToConnect);
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicPing = pRemoteService->getCharacteristic(charPingUUID);
    if (pRemoteCharacteristicPing == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charPingUUID.toString().c_str());
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    std::string value = pRemoteCharacteristicPing->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());

    try
    {
        pRemoteCharacteristicLanes = pRemoteService->getCharacteristic(charLanesUUID);
        if (pRemoteCharacteristicLanes == nullptr)
        {
            Serial.println(" - characteristic lanes not found (nullptr)");
        }
        else
        {
            Serial.println(" - Found our characteristic Lanes");
        }
    }
    catch (...)
    {
        Serial.println(" - characteristic lanes not found (exception)");
    }
    
    {
        // Obtain a reference to the characteristic in the service of the remote BLE server.
        pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
        if (pRemoteCharacteristic == nullptr) {
          Serial.print("Failed to find our characteristic UUID: ");
          Serial.println(charUUID.toString().c_str());
          return false;
        }
        Serial.println(" - Found our characteristic");
    
//        // Read the value of the characteristic.
//        std::string value = pRemoteCharacteristic->readValue();
//        Serial.print("The characteristic value was: ");
//        Serial.println(value.c_str());
//    
//        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    Serial.print("AD data: ");
    const uint8_t* payload = advertisedDevice.getPayload();
    for (int i = 0; i < 62; ++i)
    {
        if (payload[i] < 0x10)
        {
            Serial.print(" 0");
        }
        else
        {
            Serial.print(" ");
        }

        Serial.print(payload[i], HEX);
    }
    Serial.println("");

    // We have found a device, let us now see if it contains the service we are looking for.
    String addressStr(advertisedDevice.getAddress().toString().c_str());
    if (advertisedDevice.haveServiceUUID())
    {
        if (advertisedDevice.getServiceUUID().equals(serviceUUID)) {
          // 
          Serial.print("Found our device!  address: "); 
          advertisedDevice.getScan()->stop();

          myDevice = new BLEAdvertisedDevice(advertisedDevice);
          pServerAddress = new BLEAddress(advertisedDevice.getAddress());
          doConnect = true;
        }
        else
        {
            Serial.println("ServiceUUID() doesnt match"); 
        }
    }
    else
    {
//        Serial.println("haveServiceUUID() == false");
//        
//        Serial.print("Mdata: ");
//        Serial.print(advertisedDevice.getManufacturerData().length());
//        Serial.print(" ");
//        const uint8_t* mdata = (const uint8_t*)advertisedDevice.getManufacturerData().data();
//        for (int i = 0; i < advertisedDevice.getManufacturerData().length(); ++i)
//        {
//            if (mdata[i] < 0x10)
//                Serial.print(" 0");
//            else
//                Serial.print(" ");
//    
//            Serial.print(mdata[i], HEX);
//        }
//        Serial.println("");

        std::string manufacturerData = advertisedDevice.getManufacturerData();
        if (manufacturerData.length() == 19) //hack: Apple device with BLE services in background
        {
            //manufacturer ID: Apple = 0x004C
            const uint8_t* mdata = (const uint8_t*)manufacturerData.data();
            if ((mdata[0] == 0x4C && mdata[1] == 0x00) ||
                (mdata[0] == 0x00 && mdata[1] == 0x4C))
            {
                Serial.println("mdata OK");

                if (nBackgroundDevicesCount < BACKGROUND_DEVICES_SIZE)
                {
                    ppBackgroundDevices[nBackgroundDevicesCount] = new BLEAdvertisedDevice(advertisedDevice);
                    ++nBackgroundDevicesCount;
                }
                doConnect = true;
            }
        }
        else
        {
            Serial.println("manufacturerData: not Apple device with background BLE services");
        }
    }
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup()
{
//    pinMode(PIN_BUTTON, INPUT_PULLUP);

//init SPI
    pinMode(OLED_PIN_DC, OUTPUT);
    pinMode(OLED_PIN_RST, OUTPUT);

//    OLED_PIN_RST_0;
//    delay(500);
    OLED_PIN_RST_1;
    delay(200);

    vspi = new SPIClass(VSPI);
    vspi->begin();
    vspi->setHwCs(true);

    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));

    OLED_InitReg(false);

    delay(200);

    FillColor(0x0, 0, 0, 128, 128);
    OLED_WriteReg(0xAF); //Turn on the OLED display

    SetDrawArea(0, 44, 128, 48);
    SendProgmemConverting4to16bit(gImage_Sygic128x48, 128 * 48 / 2);

//init BLE
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");
    
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 30 seconds.
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(3);

    pinMode(PIN_BUTTON, INPUT_PULLUP);

    Serial.println("setup() finished");
} // End of setup.


// This is the Arduino main loop function.
void loop()
{
    Serial.println("loop() start");
    bool wasPressed = g_isButtonPressed;
    g_isButtonPressed = (digitalRead(PIN_BUTTON) == LOW);
    if (g_isButtonPressed && !wasPressed)
    {
        g_drawingMode = (DrawingMode)((int)g_drawingMode + 1);
        if (g_drawingMode == DrawingModeCount)
            g_drawingMode = DrawingModeNormal;
//        OLED_WriteReg(0xa0);
//        OLED_WriteReg(g_isMirror ? 0x50 : 0x53);
    }

    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
    // connected we set the connected flag to be true.
    if (doConnect)
    {
        if (myDevice)
        {
            if (connectToServer(myDevice))
            {
                Serial.println("We are now connected to the BLE Server.");
                connected = true;
    
                FillColor(0x00, 0, 0, 128, 128);
            }
            else
            {
                Serial.println("We have failed to connect to the server; there is nothin more we will do.");
            }
        }
        else
        {
            for (int i = 0; i < nBackgroundDevicesCount; ++i)
            {
                try
                {
                    if (connectToServer(ppBackgroundDevices[i]))
                    {
                        Serial.println("We are now connected to the BLE Server (from background list).");
                        connected = true;
            
                        FillColor(0x00, 0, 0, 128, 128);
                        myDevice = ppBackgroundDevices[i];
                    }
                    else
                    {
                        Serial.println("Error: cannot connect to BLE server in background");
                    }
                }
                catch (...)
                {
                    Serial.println("Error: exception during connecting to BLE server in background");
                }
            }
        }
        doConnect = false;
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected)
    {
        if (pRemoteCharacteristicPing)
        {
            try
            {
                std::string pingData = pRemoteCharacteristicPing->readValue();
                String newValue(pingData.c_str());
                Serial.println("Read characteristic PING value: \"" + newValue + "\"");
                
                if (pingData.size() > 0)
                {
                    memset(g_canvas, 0, sizeof(g_canvas));

                    if (pingData[0] == '1')
                    {
                        DrawMessage("App\nnot\nready", 0, 0, 4, true, 0xFFFF);
                        RedrawFromCanvas();
                    }
                    else if (pingData[0] == '2')
                    {
                        DrawMessage("No\nroute", 0, 32, 4, true, 0xFFFF);
                        
                        const int flagsOffset = 1;
                        const int speedOffset = 2;
                        if (pingData.length() > speedOffset)
                            DrawSpeed(pingData.c_str()[speedOffset]);
                        
                        RedrawFromCanvas();
                    }
                    else if (pingData[0] == '3')
                    {
                        String newValue = "connect";
                        Serial.println("writing \"" + newValue + "\"");
                        pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
                    }
                    else if (pingData[0] == 'i')
                    {
                        const int flagsOffset = 1;
                        const int speedOffset = 2;
                        const int instructionOffset = 3;
                        const int distanceOffset = 4;

                        if (pingData.length() > distanceOffset)
                            DrawMessage(pingData.c_str() + distanceOffset, 0, 64, 4, true, Color4To16bit(0x0F)/*0xFFFF*/);
                        
                        if (pingData.length() > instructionOffset)
                            DrawDirection(pingData.c_str()[instructionOffset]);

                        if (pingData.length() > speedOffset)
                            DrawSpeed(pingData.c_str()[speedOffset]);

                        if (pingData.length() > flagsOffset)
                        {
                            const uint8_t ReadLanesRequired = 1 << 4;
                            uint8_t flags = pingData.c_str()[flagsOffset];
                            if (flags & ReadLanesRequired)
                            {
                                if (pRemoteCharacteristicLanes)
                                {
                                    g_laneData = pRemoteCharacteristicLanes->readValue();
                                }
                                else
                                {
                                    Serial.println("Characteristic Lanes not init");
                                }
                            }
                            if (g_laneData.length() > 0)
                            {
                                DrawLanes(g_laneData, 96);
                            }
                        }
                        RedrawFromCanvas();
                    }
                }
            }
            catch (...)
            {
                Serial.println("exception during ping");
            }
        }
        else
        {
            Serial.println("pRemoteCharacteristic == NULL");
        }
    }

    delay(1000); // Delay a second between loops.
} // End of loop

void DrawMessage(const char *msg, int xStart, int yStart, int scale, bool overwrite, uint16_t color)
{
    const int lineHeight = 8 * scale + 4;

    const int xEnd = 120;
    const int yEnd = 120;

    int x = xStart;
    int y = yStart;
    
    Font8x8::GlyphShifter shifter;
    for (int charIndex = 0; charIndex < strlen(msg); ++charIndex)
    {
        if (msg[charIndex] == '\n')
        {
            x = xStart;
            y += lineHeight;
            if (y > yEnd)
                return;
            continue;
        }
        shifter.PutGlyph(Font8x8::FormatChar(msg[charIndex]), MIN_CHAR_WIDTH, SPACE_BETWEEN_CHARS);
        while (shifter.HasGlyph())
        {
            uint8_t column = shifter.ShiftLeft();
            DrawColumn8(x, y, column, scale, overwrite, color);
            x += scale;
            if (x > xEnd)
            {
                x = xStart;
                y += lineHeight;
                if (y > yEnd)
                    return;
            }
        }
    }
}

//void DrawRemoteDirection(uint8_t direction)
//{
//    SetDrawArea(0, 0, 64, 64);
//    const uint8_t* imageProgmem = ImageFromDirection(direction);
//    SendProgmemConverting4to16bit(imageProgmem, 64 * 64 / 2);
//}

void DrawDirection(uint8_t direction)
{
    const uint8_t* imageProgmem = ImageFromDirection(direction);
    Draw4bitImageProgmem(0, 0, 64, 64, imageProgmem);
}

void DrawSpeed(uint8_t speed)
{
    if (speed == 0)
        return;

    char str[4] = {};
    sprintf(str, "%u", (unsigned int)speed);
    DrawImageProgmem(64, 0, 64, 64, reinterpret_cast<const uint16_t*>(IMG_Speed64rgb));

    int x = 10;
    int y = 18;
    int scale = 4;
    
    if (speed <= 9)
        x = 20;
    else if (speed <= 19)
        x = 14;
    else if (speed >= 100)
    {
        y = 22;
        scale = 3;
        if (speed >= 110 && speed <= 119)
            x = 14;
    }
    DrawMessage(str, 64 + x, y, scale, false, 0x0000);
}

void DrawLanes(const std::string& laneData, int yOffset)
{
    typedef uint16_t LaneInfo;
    const int MAX_LANES = 6;
    const int IMG_WIDTH = 20;
    const int IMG_HEIGHT = 32;
    const int xDeltaUTurn = 3;
    const size_t lanesCount = laneData.length() / sizeof(LaneInfo);
    const LaneInfo* pData = reinterpret_cast<const LaneInfo*> (laneData.c_str());

    int xOffset = xDeltaUTurn;

    enum LaneEnum
    {
    FlagSymbolUTurnLeft                 = 1 << 0,
    FlagSymbolLeft                      = 1 << 1,
    FlagSymbolHalfLeft                  = 1 << 2,
    FlagSymbolStraight                  = 1 << 3,
    FlagSymbolHalfRight                 = 1 << 4,
    FlagSymbolRight                     = 1 << 5,
    FlagSymbolUTurnRight                = 1 << 6,

    FlagLineIntrLineWithLongLines       = 1 << 7,
    FlagLineDoubleSolidLine             = 1 << 8,
    FlagLineSingleSolidLine             = 1 << 9,
    FlagLineCombinedSingleSolidAndIntr  = 1 << 10,
    FlagLineCombinedIntrAndSingleSolid  = 1 << 11,
    FlagLineIntrLineWithShortLines      = 1 << 12,

    FlagLaneTake                        = 1 << 15,
    };

    int startLane = 0;
    for (int i = 0; i < lanesCount; ++i)
    {
        LaneInfo lane = pData[i];
        if (lane & FlagLaneTake)
        {
            if (i - startLane > (MAX_LANES - 1))
                startLane = i - (MAX_LANES - 1);
            break;
        }
    }
    
    for (int i = startLane; i < lanesCount; ++i)
    {
        LaneInfo lane = pData[i];

        const uint8_t colorFlag = 0x01;
        uint8_t highlight = 0x00;
        if (lane & FlagLaneTake)
            highlight = 0x0F;
            
        if (lane & FlagSymbolHalfLeft)
        {
            if (lane & FlagSymbolStraight)
                lane |= FlagSymbolLeft; //visually 2 images cannot mix "slightly left and strait", so draw "straight & left"
            else
                Draw4bitImageProgmemReplacing(xOffset, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolHalfLeft, colorFlag, highlight);
        }
        if (lane & FlagSymbolLeft)
            Draw4bitImageProgmemReplacing(xOffset, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolLeft, colorFlag, highlight);
        if (lane & FlagSymbolUTurnLeft)
            Draw4bitImageProgmemReplacing(xOffset - xDeltaUTurn, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolUTurnLeft, colorFlag, highlight);
            
        if (lane & FlagSymbolHalfRight)
        {
            if (lane & FlagSymbolStraight)
                lane |= FlagSymbolRight; //visually 2 images cannot mix "slightly right and strait", so draw "straight & right"
            else
                Draw4bitImageProgmemReplacing(xOffset, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolHalfRight, colorFlag, highlight);
        }
        if (lane & FlagSymbolRight)
            Draw4bitImageProgmemReplacing(xOffset, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolRight, colorFlag, highlight);
        if (lane & FlagSymbolUTurnRight)
            Draw4bitImageProgmemReplacing(xOffset + xDeltaUTurn, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolUTurnRight, colorFlag, highlight);
        
        if (lane & FlagSymbolStraight)
            Draw4bitImageProgmemReplacing(xOffset, yOffset, IMG_WIDTH, IMG_HEIGHT, IMG_SymbolStraight, colorFlag, highlight);
            
        xOffset += IMG_WIDTH;
    }
}

const uint8_t* ImageFromDirection(uint8_t direction)
{
    switch (direction)
    {
        case DirectionStart:      return IMG_directionStart;
        case DirectionEasyLeft:   return IMG_directionEasyLeft;
        case DirectionEasyRight:  return IMG_directionEasyRight;
        case DirectionEnd:        return IMG_directionEnd;
        case DirectionVia:        return IMG_directionEnd;
        case DirectionKeepLeft:   return IMG_directionKeepLeft;
        case DirectionKeepRight:  return IMG_directionKeepRight;
        case DirectionLeft:       return IMG_directionLeft;
        case DirectionRight:      return IMG_directionRight;
        case DirectionSharpLeft:  return IMG_directionSharpLeft;
        case DirectionSharpRight: return IMG_directionSharpRight;
        case DirectionStraight:   return IMG_directionStraight;
        case DirectionUTurnLeft:  return IMG_directionUTurnLeft;
        case DirectionUTurnRight: return IMG_directionUTurnRight;
        //right side roundabouts
        case DirectionRoundaboutSE:  return IMG_directionRoundRSE;
        case DirectionRoundaboutE:   return IMG_directionRoundRE;
        case DirectionRoundaboutNE:  return IMG_directionRoundRNE;
        case DirectionRoundaboutN:   return IMG_directionRoundRN;
        case DirectionRoundaboutNW:  return IMG_directionRoundRNW;
        case DirectionRoundaboutW:   return IMG_directionRoundRW;
        case DirectionRoundaboutSW:  return IMG_directionRoundRSW;
        case DirectionRoundaboutS:   return IMG_directionRoundRS;
        //left side roundabouts
        case DirectionRoundaboutSE + 8: return IMG_directionRoundLSE;
        case DirectionRoundaboutE  + 8: return IMG_directionRoundLE;
        case DirectionRoundaboutNE + 8: return IMG_directionRoundLNE;
        case DirectionRoundaboutN  + 8: return IMG_directionRoundLN;
        case DirectionRoundaboutNW + 8: return IMG_directionRoundLNW;
        case DirectionRoundaboutW  + 8: return IMG_directionRoundLW;
        case DirectionRoundaboutSW + 8: return IMG_directionRoundLSW;
        case DirectionRoundaboutS  + 8: return IMG_directionRoundLS;
        case DirectionFerry:         return IMG_directionFerry;
        case DirectionStateBoundary: return IMG_directionStateBoundary;
        case DirectionExitRight:     return IMG_directionExitRight;
        case DirectionFollow:        return IMG_directionFollow;
        case DirectionExitLeft:      return IMG_directionExitLeft;
        case DirectionMotorway:      return IMG_directionMotorway;
    }
    return IMG_directionOutOfRoute;
}

void RedrawFromCanvas()
{
    SetDrawArea(0, 0, 128, 128);

    auto pCanvas = reinterpret_cast<const uint8_t*>(g_canvas);
    const int SCREEN_WIDTH = 128;
    const int SCREEN_HEIGHT = 128;


    OLED_WriteReg(0x5c); //write RAM mode
    if (g_drawingMode == DrawingModeMirror)
    {
        for (int y = SCREEN_HEIGHT - 1; y >= 0; --y)
        {
            for (int x = 0; x < SCREEN_WIDTH; ++x)
            {
                const int i = (y * SCREEN_WIDTH + x) * sizeof(g_canvas[0]);
                OLED_WriteData(pCanvas[i + 1]);
                OLED_WriteData(pCanvas[i]);
            }
        }
    }
    else if (g_drawingMode == DrawingMode180Mirror)
    {
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            for (int x = SCREEN_WIDTH - 1; x >= 0; --x)
            {
                const int i = (y * SCREEN_WIDTH + x) * sizeof(g_canvas[0]);
                OLED_WriteData(pCanvas[i + 1]);
                OLED_WriteData(pCanvas[i]);
            }
        }
    }
    else if (g_drawingMode == DrawingMode180)
    {
        for (int i = sizeof(g_canvas) - 1; i > 0; i -= sizeof(g_canvas[0]))
        {
            OLED_WriteData(pCanvas[i]);
            OLED_WriteData(pCanvas[i - 1]);
        }
    }
    else
    {
        for (int i = 1; i < sizeof(g_canvas); i += sizeof(g_canvas[0]))
        {
            OLED_WriteData(pCanvas[i]);
            OLED_WriteData(pCanvas[i - 1]);
        }
    }
//    
//    Send4bitFromEachByte(g_canvas, 128 * 128);
}


void Send4bitFromEachByte(const uint8_t* pBmp, uint16_t sizeBytes)
{
    OLED_WriteReg(0x5c); //write RAM mode
    for (uint16_t i = 0; i < sizeBytes; ++i)
    {
        uint16_t data = Color4To16bit(0b11110000 ^ pBmp[i]);
        
        OLED_WriteData(static_cast<uint8_t>(data >> 8));
        OLED_WriteData(static_cast<uint8_t>(data));
    }
}

void DrawImageProgmem(int xStart, int yStart, int width, int height, const uint16_t* pBmp)
{
    const int sizePixels = width * height;
    for (int y = yStart; y < yStart + height; ++y)
    {
        for (int x = xStart; x < xStart + width; ++x)
        {
            SetPixelCanvas(x, y, pgm_read_word(pBmp++));
        }
    }
}

void Draw4bitImageProgmem(int x, int y, int width, int height, const uint8_t* pBmp)
{
    const int BYTES_IN_ROW = 128;
    const int sizePixels = width * height;
    for (int i = 1; i < sizePixels; i += 2)
    {
        uint8_t data = pgm_read_byte(pBmp++);
        uint8_t leftPixel = (data & 0x0F);
        uint8_t rightPixel = (data & 0xF0) >> 4;

        int yLeft = y + (i - 1) / width;
        int xLeft = x + (i - 1) % width;
                
        int yRight = y + i / width;
        int xRight = x + i % width;

        SetPixelCanvas(xLeft, yLeft, Color4To16bit(leftPixel));
        SetPixelCanvas(xRight, yRight, Color4To16bit(rightPixel));
    }
}

void Draw4bitImageProgmemReplacing(int x, int y, int width, int height, const uint8_t* pBmp, uint8_t toReplace, uint8_t replaceWith)
{
    const int BYTES_IN_ROW = 128;
    const int sizePixels = width * height;
    for (int i = 1; i < sizePixels; i += 2)
    {
        uint8_t data = pgm_read_byte(pBmp++);
        uint8_t leftPixel = (data & 0x0F);
        uint8_t rightPixel = (data & 0xF0) >> 4;

        if (leftPixel == toReplace)
            leftPixel = replaceWith;
        if (rightPixel == toReplace)
            rightPixel = replaceWith;

        int yLeft = y + (i - 1) / width;
        int xLeft = x + (i - 1) % width;
                
        int yRight = y + i / width;
        int xRight = x + i % width;

        SetPixelCanvasIfNot0(xLeft, yLeft, Color4To16bit(leftPixel));
        SetPixelCanvasIfNot0(xRight, yRight, Color4To16bit(rightPixel));
    }
}

void SetPixelCanvas(int x, int y, uint16_t value)
{
    const int BYTES_IN_ROW = 128;
    int indexDestination = y * BYTES_IN_ROW + x;
    if (indexDestination < sizeof(g_canvas) / sizeof(g_canvas[0]))
        g_canvas[indexDestination] = value;
}

void SetPixelCanvasIfNot0(int x, int y, uint16_t value)
{
    if (value)
        SetPixelCanvas(x, y, value);
}

void DrawColumn8(uint8_t x, uint8_t y, uint8_t columnData, int scale, bool overwrite, uint16_t color)
{
    const uint16_t BYTES_IN_ROW = 128;

    uint8_t mask = 1;
    for (uint8_t row = 0; row < Font8x8::HEIGHT * scale; row += scale)
    {
        for (int fillIndex = 0; fillIndex < scale * scale; ++fillIndex)
        {
            uint8_t yDestination = y + row + fillIndex / scale;
            uint16_t drawIndex = yDestination * BYTES_IN_ROW + x + fillIndex % scale;
            if (drawIndex >= sizeof(g_canvas) / sizeof(g_canvas[0]))
                return;
    
            if (columnData & mask)
                g_canvas[drawIndex] = color;
            else if (overwrite)
                g_canvas[drawIndex] = 0x0000;
        }        

        mask <<= 1;
    }
}
