#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <SPI.h>
#include "OLED.h"
#include "ImagesOther.h"
#include "ImagesDirections.h"
#include "ImagesLanes.h"
#include "Font8x8.h"
#include "GlyphShifter.h"
#include "DataConstants.h"

#define SERVICE_UUID        "DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86"
#define CHAR_INDICATE_UUID  "DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86"
#define CHAR_WRITE_UUID     "DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86"

// ---------------------
// Variables for display
// ---------------------
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

const int spiClk = 32000000; // 32 MHz (40 MHz causes problems, about 1% of bytes are not received by display)

uint16_t g_canvas[128 * 128] = {};
DrawingMode g_drawingMode = DrawingModeNormal;

// -----------------
// Variables for BLE
// -----------------
BLEServer* g_pServer = NULL;
BLECharacteristic* g_pCharIndicate = NULL;
bool g_deviceConnected = false;
uint32_t g_lastActivityTime = 0;
bool g_isNaviDataUpdated = false;
std::string g_naviData;

class MyServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer) override
    {
        g_deviceConnected = true;
        g_lastActivityTime = millis();
    }

    void onDisconnect(BLEServer* pServer) override
    {
        g_deviceConnected = false;
        BLEDevice::startAdvertising();
    }
};

class MyCharWriteCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        g_lastActivityTime = millis();
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0)
        {
            g_naviData = value;
            g_isNaviDataUpdated = true;
            Serial.print("New value. ");
        }
    }
};

void setup()
{
    Serial.begin(115200);
    Serial.println("BLENaviPeripheral2 setup() started");

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

    memset(g_canvas, 0, sizeof(g_canvas));
    Draw4bitImageProgmem(0, 32, 128, 64, IMG_logoTbt128x64);
    Draw4bitImageProgmem(0, 0, 32, 32, IMG_logoBluetooth32x32);
    RedrawFromCanvas();

    OLED_WriteReg(0xAF); //Turn on the OLED display

    Serial.println("Display init done");

    // init BLE
    Serial.println("BLE init started");

    BLEDevice::init("ESP32 HUD");
    g_pServer = BLEDevice::createServer();
    g_pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = g_pServer->createService(SERVICE_UUID);

    // characteristic for indicate
    {
        uint32_t charProperties = BLECharacteristic::PROPERTY_INDICATE;
        g_pCharIndicate = pService->createCharacteristic(CHAR_INDICATE_UUID, charProperties);
        g_pCharIndicate->addDescriptor(new BLE2902());
        g_pCharIndicate->setValue("");
    }

    // characteristic for write
    {
        uint32_t charProperties = BLECharacteristic::PROPERTY_WRITE;
        BLECharacteristic *pCharWrite = pService->createCharacteristic(CHAR_WRITE_UUID, charProperties);
        pCharWrite->setCallbacks(new MyCharWriteCallbacks());
    }

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE init done");

    Serial.println("setup() finished");
}

void loop()
{
    if (g_deviceConnected)
    {
        if (g_isNaviDataUpdated)
        {
            g_isNaviDataUpdated = false;
    
            std::string currentData = g_naviData;
            if (currentData.size() > 0)
            {
                memset(g_canvas, 0, sizeof(g_canvas));
                if (currentData[0] == 1)
                {
                    Serial.print("Reading basic data: length = ");
                    Serial.println(currentData.length());
                    
                    const int speedOffset = 1;
                    const int instructionOffset = 2;
                    const int textOffset = 3;

                    if (currentData.length() > textOffset)
                    {
                        int scale = 4;
                        const char* text = currentData.c_str() + textOffset;
                        const int textLen = strlen(text);
                        if (textLen > 8)
                        {
                            scale = 2;
                        }
                        else if (textLen > 6)
                        {
                            scale = 3;
                        }
                        DrawMessage(currentData.c_str() + textOffset, 0, 64, scale, true, Color4To16bit(0x0F)/*0xFFFF*/);
                    }
                    
                    if (currentData.length() > instructionOffset)
                        DrawDirection(currentData.c_str()[instructionOffset]);

                    if (currentData.length() > speedOffset)
                        DrawSpeed(currentData.c_str()[speedOffset]);

                    RedrawFromCanvas();
                }
                else
                {
                    Serial.println("invalid first byte");
                }
            }
        }
        else
        {
            uint32_t time = millis();
            if (time - g_lastActivityTime > 4000)
            {
                g_lastActivityTime = time;
                g_pCharIndicate->indicate();
            }
        }
    }
    else if (millis() > 3000)
    {
        const int yOffset = 128 - 16 - 1;
        const int rowSize = 128 * 2;
        memset((uint8_t*)g_canvas + yOffset * rowSize, 0, 16 * rowSize);
        DrawMessage("Disconnected", 0, yOffset, 2, true, 0xFFFF);
        RedrawFromCanvas();
    }
    delay(10);
}

void DrawMessage(const char* msg, int xStart, int yStart, int scale, bool overwrite, uint16_t color)
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

void DrawDirection(uint8_t direction)
{
    const uint8_t* imageProgmem = ImageFromDirection(direction);
    if (imageProgmem)
    {
        Draw4bitImageProgmem(0, 0, 64, 64, imageProgmem);
    }
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

const uint8_t* ImageFromDirection(uint8_t direction)
{
    switch (direction)
    {
        case DirectionNone: return nullptr;
        case DirectionStart: return IMG_directionWaypoint;
        case DirectionEasyLeft: return IMG_directionEasyLeft;
        case DirectionEasyRight: return IMG_directionEasyRight;
        case DirectionEnd: return IMG_directionWaypoint;
        case DirectionVia: return IMG_directionWaypoint;
        case DirectionKeepLeft: return IMG_directionKeepLeft;
        case DirectionKeepRight: return IMG_directionKeepRight;
        case DirectionLeft: return IMG_directionLeft;
        case DirectionOutOfRoute: return IMG_directionOutOfRoute;
        case DirectionRight: return IMG_directionRight;
        case DirectionSharpLeft: return IMG_directionSharpLeft;
        case DirectionSharpRight: return IMG_directionSharpRight;
        case DirectionStraight: return IMG_directionStraight;
        case DirectionUTurnLeft: return IMG_directionUTurnLeft;
        case DirectionUTurnRight: return IMG_directionUTurnRight;
        case DirectionFerry: return IMG_directionFerry;
        case DirectionStateBoundary: return IMG_directionStateBoundary;
        case DirectionFollow: return IMG_directionFollow;
        case DirectionMotorway: return IMG_directionMotorway;
        case DirectionTunnel: return IMG_directionTunnel;
        case DirectionExitLeft: return IMG_directionExitLeft;
        case DirectionExitRight: return IMG_directionExitRight;
        case DirectionRoundaboutRSE: return IMG_directionRoundaboutRSE;
        case DirectionRoundaboutRE: return IMG_directionRoundaboutRE;
        case DirectionRoundaboutRNE: return IMG_directionRoundaboutRNE;
        case DirectionRoundaboutRN: return IMG_directionRoundaboutRN;
        case DirectionRoundaboutRNW: return IMG_directionRoundaboutRNW;
        case DirectionRoundaboutRW: return IMG_directionRoundaboutRW;
        case DirectionRoundaboutRSW: return IMG_directionRoundaboutRSW;
        case DirectionRoundaboutRS: return IMG_directionRoundaboutRS;
        case DirectionRoundaboutLSE: return IMG_directionRoundaboutLSE;
        case DirectionRoundaboutLE: return IMG_directionRoundaboutLE;
        case DirectionRoundaboutLNE: return IMG_directionRoundaboutLNE;
        case DirectionRoundaboutLN: return IMG_directionRoundaboutLN;
        case DirectionRoundaboutLNW: return IMG_directionRoundaboutLNW;
        case DirectionRoundaboutLW: return IMG_directionRoundaboutLW;
        case DirectionRoundaboutLSW: return IMG_directionRoundaboutLSW;
        case DirectionRoundaboutLS: return IMG_directionRoundaboutLS;
    }
    return IMG_directionError;
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