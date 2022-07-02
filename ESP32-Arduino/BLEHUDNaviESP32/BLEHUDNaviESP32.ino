#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Button2.h> // version 2.0.3, available in Arduino libraries, or https://github.com/LennartHennigs/Button2
#include "IDisplay.h"
#include "ImagesOther.h"
#include "ImagesDirections.h"
#include "ImagesLanes.h"
#include "Font8x8GlyphShifter.h"
#include "DataConstants.h"
#include "VoltageMeasurement.h"

// -----------------
// Display selection
// Uncomment wanted display with corresponding header
// -----------------

// OLED 128x128 RGB, Waveshare 14747, driver SSD1351
// Doesn't require external libraries
// Pins: DIN=23, CLK=18, CS=5, DC=17, RST=16, uses SPIClass(VSPI)
#include "OLED_SSD1351_nolib.h"
OLED_SSD1351_nolib selectedDisplay;
constexpr bool ENABLE_VOLTAGE_MEASUREMENT = false;

// TTGO T-Display TFT 135x240
// Requires library TFT_eSPI from here: https://github.com/Xinyuan-LilyGO/TTGO-T-Display
// (copy TFT_eSPI to Arduino/libraries)
//#include "TFT_TTGO.h"
//TFT_TTGO selectedDisplay;
//constexpr bool ENABLE_VOLTAGE_MEASUREMENT = true;

// ---------------------
// Variables for display
// ---------------------
#define DISPLAY_MIRRORED 0 // 0 or 1
#define DISPLAY_ROTATION 0 // 0, 90, 180 or 270

IDisplay& g_display = selectedDisplay;
const int CANVAS_WIDTH = g_display.GetWidth();
const int CANVAS_HEIGHT = g_display.GetHeight();
const int CANVAS_SIZE_BYTES = CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(uint16_t);
uint16_t* g_canvas = NULL;

// ---------------------
// Constants
// ---------------------
#define SERVICE_UUID        "DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86"
#define CHAR_INDICATE_UUID  "DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86"
#define CHAR_WRITE_UUID     "DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86"

#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_MAGENTA  0xF81F

// -----------------
// Variables for BLE
// -----------------
BLEServer* g_pServer = NULL;
BLECharacteristic* g_pCharIndicate = NULL;
bool g_deviceConnected = false;
uint32_t g_lastActivityTime = 0;
bool g_isNaviDataUpdated = false;
std::string g_naviData;

// --------
// Buttons
// --------
#define TTGO_LEFT_BUTTON 0
#define GPIO_NUM_TTGO_LEFT_BUTTON GPIO_NUM_0

#define TTGO_RIGHT_BUTTON 35
//#define GPIO_NUM_TTGO_RIGHT_BUTTON GPIO_NUM_35

#define BUTTON_DEEP_SLEEP TTGO_LEFT_BUTTON
#define GPIO_NUM_WAKEUP GPIO_NUM_TTGO_LEFT_BUTTON

Button2 g_btnDeepSleep(BUTTON_DEEP_SLEEP);
bool g_sleepRequested = false;

// --------
// Voltage measurement
// --------
#define VOLTAGE_ADC_ENABLE          14
#define VOLTAGE_ADC_PIN             34
static VoltageMeasurement g_voltage(VOLTAGE_ADC_PIN, VOLTAGE_ADC_ENABLE);
static bool g_showVoltage = false;
Button2 g_btn1(TTGO_RIGHT_BUTTON);

// ---------------------
// Bluetooth event callbacks
// ---------------------
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
            Serial.print("New value, length = ");
            Serial.print(value.length());
            Serial.print(": ");
            for (int i = 0; i < value.length(); ++i)
            {
                char tmp[4] = "";
                sprintf(tmp, "%02X ", value[i]);
                Serial.print(tmp);
            }
            Serial.println();
        }
    }
};

void setup()
{
    Serial.begin(115200);
    Serial.println("BLENaviPeripheral2 setup() started");

    g_display.Init();
    g_canvas = new uint16_t[CANVAS_WIDTH * CANVAS_HEIGHT];

    memset(g_canvas, 0, CANVAS_SIZE_BYTES);
    Draw4bitImageProgmem(0, 32, 128, 64, IMG_logoTbt128x64_4b);
    RedrawFromCanvas();

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

    // setup deep sleep button
    g_btnDeepSleep.setLongClickTime(500);
    g_btnDeepSleep.setLongClickDetectedHandler([](Button2& b) {
        g_sleepRequested = true;
    });
    g_btnDeepSleep.setReleasedHandler([](Button2& b) {
        if (!g_sleepRequested)
        {
            return;    
        }
        
        g_display.EnterSleepMode();

        // without this the module won't wake up with button if powered from battery,
        // especially if entered deep sleep when powered from USB
        g_voltage.end();
        
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_WAKEUP, 0);
        delay(200);
        esp_deep_sleep_start();
    });

    // setup voltage measurement
    if (ENABLE_VOLTAGE_MEASUREMENT)
    {
        g_voltage.begin();
        g_btn1.setPressedHandler([](Button2& b) {
            g_showVoltage = true;
        });
        g_btn1.setReleasedHandler([](Button2& b) {
            g_showVoltage = false;
        });
    }

    Serial.println("setup() finished");
}

void loop()
{
    g_btnDeepSleep.loop();
    g_btn1.loop();
    if (g_sleepRequested)
    {
        DrawBottomMessage("SLEEP", COLOR_MAGENTA);
    }
    else if (g_showVoltage)
    {
        static uint64_t voltageTimeStamp = 0;
        if (millis() - voltageTimeStamp > 1000)
        {
            voltageTimeStamp = millis();
            String voltageStr = String(g_voltage.measureVolts()) + " V";
            DrawBottomMessage(voltageStr.c_str(), COLOR_WHITE);
        }
    }
    else if (g_deviceConnected)
    {
        if (g_isNaviDataUpdated)
        {
            g_isNaviDataUpdated = false;
    
            std::string currentData = g_naviData;
            if (currentData.size() > 0)
            {
                memset(g_canvas, 0, CANVAS_SIZE_BYTES);
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
        DrawBottomMessage("Disconnected", COLOR_WHITE);
    }
    delay(10);
}

void DrawBottomMessage(const char* msg, uint16_t color)
{
    const int16_t textHeight = 16;
    const int16_t yOffset = CANVAS_HEIGHT - textHeight;
    FillRect(0, yOffset, CANVAS_WIDTH, textHeight, COLOR_BLACK);
    DrawMessage(msg, 0, yOffset, 2, true, color);
    RedrawFromCanvas();
}

void DrawMessage(const char* msg, int xStart, int yStart, int scale, bool overwrite, uint16_t color)
{
    const int lineHeight = 8 * scale + 4;

    const int xEnd = CANVAS_WIDTH - scale;
    const int yEnd = CANVAS_HEIGHT - scale;

    int x = xStart;
    int y = yStart;
    
    Font8x8::GlyphShifter shifter;
    for (int charIndex = 0; charIndex < strlen(msg); ++charIndex)
    {
        if (msg[charIndex] == '\n')
        {
            x = xStart;
            y += lineHeight;
            if (y >= yEnd)
                return;
            continue;
        }
        shifter.PutChar(msg[charIndex]);
        while (shifter.HasGlyph())
        {
            uint8_t column = shifter.ShiftLeft();
            DrawColumn8(x, y, column, scale, overwrite, color);
            x += scale;
            if (x >= xEnd)
            {
                x = xStart;
                y += lineHeight;
                if (y >= yEnd)
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
    DrawImageProgmem(64, 0, 64, 64, reinterpret_cast<const uint16_t*>(IMG_speedLimit64x64_16b));

    int x = 10;
    int y = 18;
    int scale = 4;
    
    if (speed <= 9)
        x = 20;
    else if (speed <= 19)
        x = 14;
    else if (speed >= 40 && speed <= 49)
        x = 7;
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
    g_display.SendImage(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, g_canvas);
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

uint16_t Color4To16bit(uint16_t color4bit)
{
    color4bit &= 0x0F;
    uint16_t color16bit = 0;

    const uint16_t maxColor4bit = 0x0F;
    const uint16_t maxColor5bit = 0x1F;
    const uint16_t maxColor6bit = 0x3F;
    
    const uint16_t red   = color4bit * maxColor5bit / maxColor4bit;
    const uint16_t green = color4bit * maxColor6bit / maxColor4bit;
    const uint16_t blue  = color4bit * maxColor5bit / maxColor4bit;

    //color 16 bit: rrrrrggg gggbbbbb
    color16bit |= red << 11;
    color16bit |= green << 5;
    color16bit |= blue;
    
//    if (color4bit & 0b10000000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 12; //red, 5 bit
//    if (color4bit & 0b01000000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 7;  //green, 6 bit
//    if (color4bit & 0b00100000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 1;  //blue, 5 bit

    return color16bit;
}

void Draw4bitImageProgmem(int x, int y, int width, int height, const uint8_t* pBmp)
{
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

void SetPixelCanvas(int16_t x, int16_t y, uint16_t value)
{
#if DISPLAY_MIRRORED
    x = CANVAS_WIDTH - x;
#endif

#if DISPLAY_ROTATION == 90
    const int16_t xOriginal = x;
    x = CANVAS_WIDTH - y;
    y = xOriginal;
#elif DISPLAY_ROTATION == 180
    x = CANVAS_WIDTH - x;
    y = CANVAS_HEIGHT - y;
#elif DISPLAY_ROTATION == 270
    const int16_t xOriginal = x;
    x = y;
    y = CANVAS_HEIGHT - xOriginal;
#endif
    
    if (x < 0 || y < 0 || x >= CANVAS_WIDTH || y >= CANVAS_HEIGHT)
    {
        return;
    }
    g_canvas[y * CANVAS_WIDTH + x] = value;
}

void SetPixelCanvasIfNot0(int16_t x, int16_t y, uint16_t value)
{
    if (value)
        SetPixelCanvas(x, y, value);
}

void DrawColumn8(int16_t x, int16_t y, uint8_t columnData, int scale, bool overwrite, uint16_t color)
{
    uint8_t mask = 1;
    for (uint8_t row = 0; row < Font8x8::HEIGHT * scale; row += scale)
    {
        for (int fillIndex = 0; fillIndex < scale * scale; ++fillIndex)
        {
            int16_t xDestination = x + fillIndex % scale;
            int16_t yDestination = y + row + fillIndex / scale;
    
            if (columnData & mask)
                SetPixelCanvas(xDestination, yDestination, color);
            else if (overwrite)
                SetPixelCanvas(xDestination, yDestination, 0x0000);
        }        

        mask <<= 1;
    }
}

void FillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (x >= CANVAS_WIDTH ||
        y >= CANVAS_HEIGHT ||
        width <= 0 ||
        height <= 0)
    {
        return;
    }

    int16_t xStart = max(x, int16_t(0));
    int16_t yStart = max(y, int16_t(0));
    int16_t xEnd = min(x + width, CANVAS_WIDTH);
    int16_t yEnd = min(y + height, CANVAS_HEIGHT);

    for (int16_t y = yStart; y < yEnd; ++y)
    {
        for (int16_t x = xStart; x < xEnd; ++x)
        {
            SetPixelCanvas(x, y, color);
        }
    }
}
