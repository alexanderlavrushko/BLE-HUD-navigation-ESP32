#include "Display_ST7735S_80x160.h"
#include "ColorConversion.h"

#define DisplayClass Display_ST7735S_80x160

namespace
{
    constexpr int SCREEN_WIDTH = 80;
    constexpr int SCREEN_HEIGHT = 160;

    // config for smart watch iGET FIT F2, display 80x160
    constexpr int PIN_DISPLAY_SPI_DATA = 13; // pin in Port 0, physical pin 19
    constexpr int PIN_DISPLAY_SPI_CLK = 4; // pin in Port 0, physical pin 8
    constexpr int PIN_DISPLAY_RESET = 23; // pin in Port 0, physical pin 42
    constexpr int PIN_DISPLAY_SLAVE_SELECT = 24; // pin in Port 0, physical pin 43
    constexpr int PIN_DISPLAY_REGISTER_DATA = 22; // pin in Port 0, physical pin 41
    constexpr int PIN_DISPLAY_BACKLIGHT = 30; // pin in Port 0, physical pin 3
    constexpr int PIN_DISPLAY_SPI_MISO = 6; // pin in Port 0, physical pin 10

    constexpr uint8_t ValueReset_NormalOperation = HIGH;
    constexpr uint8_t ValueReset_Reset = LOW;

    constexpr uint8_t ValueSlaveSelect_Active = LOW;
    constexpr uint8_t ValueSlaveSelect_Inactive = HIGH;

    constexpr uint8_t ValueRegisterData_Register = LOW;
    constexpr uint8_t ValueRegisterData_Data = HIGH;
}

DisplayClass::DisplayClass()
: m_spi(SPI)
, m_state(EState::NotInitialized)
, m_powerState(EPowerState::Sleep)
, m_availabilityTimestampMs(0)
{
}

void DisplayClass::begin()
{
    // configure pins to output
    pinMode(PIN_DISPLAY_RESET, OUTPUT);
    pinMode(PIN_DISPLAY_SLAVE_SELECT, OUTPUT);
    pinMode(PIN_DISPLAY_REGISTER_DATA, OUTPUT);    
    pinMode(PIN_DISPLAY_BACKLIGHT, OUTPUT);

    // set initial pins state
    digitalWrite(PIN_DISPLAY_BACKLIGHT, LOW);
    digitalWrite(PIN_DISPLAY_SLAVE_SELECT, ValueSlaveSelect_Active);
    digitalWrite(PIN_DISPLAY_REGISTER_DATA, ValueRegisterData_Data);

    SPI.setPins(PIN_DISPLAY_SPI_MISO, PIN_DISPLAY_SPI_CLK, PIN_DISPLAY_SPI_DATA);
    SPI.begin(); // begin is required to apply the pins
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

    m_state = EState::Initialized;
    
    // reset display, to be sure we have a consistent state
    hardwareReset();

    setupGraphics();
    fillColor(0x0000, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    setBacklight(true);
}

int DisplayClass::getWidth()
{
    return SCREEN_WIDTH;
}

int DisplayClass::getHeight()
{
    return SCREEN_HEIGHT;
}

void DisplayClass::sendImage(int16_t xStart, int16_t yStart,
                             int16_t width, int16_t height,
                             const uint16_t* data)
{
    setDrawArea(xStart, yStart, width, height);
    sendCommand(0x2C); // memory write
    
    const uint32_t pixelCount = width * height;
    for (uint32_t i = 0; i < pixelCount; ++i)
    {
        uint16_t value = data[i];
        m_spi.transfer(value >> 8);
        m_spi.transfer(value);
    }
}

void DisplayClass::sendImage8(int16_t xStart, int16_t yStart,
                              int16_t width, int16_t height,
                              const uint8_t* data)
{
    setDrawArea(xStart, yStart, width, height);
    sendCommand(0x2C); // memory write

    const uint8_t* ptrEnd = data + width * height;
    for (const uint8_t* ptr = data; ptr != ptrEnd; ++ptr)
    {
        const uint16_t color16 = Color8To16bit(*ptr);
        m_spi.transfer(color16 >> 8);
        m_spi.transfer(color16);
    }
}

void DisplayClass::setSleepMode(bool hasToSleep)
{
    if (hasToSleep)
    {
        fillColor(0x0000, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // avoid short blink during next wake up, fill the screen now
        setBacklight(false);
        setPowerState(EPowerState::Sleep);
    }
    else
    {
        setPowerState(EPowerState::On);
        setBacklight(true);
    }
}

void DisplayClass::hardwareReset()
{
    digitalWrite(PIN_DISPLAY_RESET, ValueReset_Reset);
    delayMicroseconds(11); // ST7735S: 10 microseconds to recognize reset
    digitalWrite(PIN_DISPLAY_RESET, ValueReset_NormalOperation);
    m_availabilityTimestampMs = millis() + 121; // ST7735S: 120 milliseconds before exit sleep (Sleep Out command)
    m_powerState = EPowerState::Sleep;
//    delay(6); // ST7735S: 5 milliseconds before sending commands
//    delay(121); // ST7735S: 120 milliseconds before exit sleep (Sleep Out command)
}

void DisplayClass::setupGraphics()
{
    setPowerState(EPowerState::On);

    sendCommand(0x21); // invert colors, no idea why but it works as normal colors

    sendCommand(0xB1); // frame rate control in mode normal, full colors
    sendData(0x05);
    sendData(0x3A);
    sendData(0x3A);

    sendCommand(0xB2); // frame rate control in mode idle, 8 colors
    sendData(0x05);
    sendData(0x3A);
    sendData(0x3A);
    
    sendCommand(0xB3); // frame rate control in mode partial, full colors
    sendData(0x05);
    sendData(0x3A);
    sendData(0x3A);
    sendData(0x05);
    sendData(0x3A);
    sendData(0x3A);
    
    sendCommand(0xB4); // display inversion control
    sendData(0x03);

    sendCommand(0xC0); // power control 1
    sendData(0x62);
    sendData(0x02);
    sendData(0x04);

    sendCommand(0xC1); // power control 2
    sendData(0xC0);

    sendCommand(0xC2); // power control 3 in mode normal, full colors
    sendData(0x0D);
    sendData(0x00);

    sendCommand(0xC3); // power control 4 in mode idle, 8 colors
    sendData(0x8D);
    sendData(0x6A);

    sendCommand(0xC4); // power control 5 in mode partial, full colors
    sendData(0x8D);
    sendData(0xEE);

    sendCommand(0xC5); // VCOM control 1
    sendData(0x0E);

    sendCommand(0xE0); // gamma '+' polarity correction
    sendData(0x10);
    sendData(0x0E);
    sendData(0x02);
    sendData(0x03);
    sendData(0x0E);
    sendData(0x07);
    sendData(0x02);
    sendData(0x07);
    sendData(0x0A);
    sendData(0x12);
    sendData(0x27);
    sendData(0x37);
    sendData(0x00);
    sendData(0x0D);
    sendData(0x0E);
    sendData(0x10);

    sendCommand(0xE1); // gamma '-' polarity correction
    sendData(0x10);
    sendData(0x0E);
    sendData(0x03);
    sendData(0x03);
    sendData(0x0F);
    sendData(0x06);
    sendData(0x02);
    sendData(0x08);
    sendData(0x0A);
    sendData(0x13);
    sendData(0x26);
    sendData(0x36);
    sendData(0x00);
    sendData(0x0D);
    sendData(0x0E);
    sendData(0x10);

    sendCommand(0x36); // memory data access control, MX, MY, RGB
    sendData(0xC8);

    sendCommand(0x3A); // interface pixel format
    sendData(0x05); // 5-6-5 RGB, 16-bit per pixel

    sendCommand(0x29); // display on
}

DisplayClass::EState DisplayClass::getState()
{
    return m_state;
}

DisplayClass::EPowerState DisplayClass::getPowerState()
{
    return m_powerState;
}

void DisplayClass::setPowerState(EPowerState newState)
{
    if (m_state != EState::Initialized)
        begin();

    if (newState == m_powerState)
        return;

    waitUntilAvailable();

    if (newState == EPowerState::On)
    {
        sendCommand(0x11); // Sleep Out
        delay(121);
    }
    else if (newState == EPowerState::Sleep)
    {
        sendCommand(0x10); // Sleep In
        delay(121);
    }
    m_powerState = newState;
}

void DisplayClass::setBacklight(bool isOn)
{
    digitalWrite(PIN_DISPLAY_BACKLIGHT, isOn ? HIGH : LOW);
}

void DisplayClass::waitUntilAvailable()
{
    uint32_t timeNowMs = millis();
    if (timeNowMs < m_availabilityTimestampMs)
    {
        uint32_t deltaMs = m_availabilityTimestampMs - timeNowMs;
        delay(deltaMs);
    }
}

void DisplayClass::sendCommand(uint8_t cmd)
{
    digitalWrite(PIN_DISPLAY_REGISTER_DATA, ValueRegisterData_Register);
    m_spi.transfer(cmd);
    digitalWrite(PIN_DISPLAY_REGISTER_DATA, ValueRegisterData_Data);
}

void DisplayClass::sendData(uint8_t data)
{
    m_spi.transfer(data);
}

void DisplayClass::sendData16(uint16_t data)
{
    m_spi.transfer(data >> 8);
    m_spi.transfer(data & 0xFF);
}

// drawing methods
void DisplayClass::setDrawArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    x += 26;
    y += 1;
    sendCommand(0x2A); // column address set
    sendData16(x);
    sendData16(x + width - 1);
    sendCommand(0x2B); // row address set
    sendData16(y);
    sendData16(y + height - 1);
}

void DisplayClass::fillColor(uint16_t color, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    setDrawArea(x, y, width, height);
    sendCommand(0x2C); // memory write
    
    const uint32_t highByte = color >> 8;
    const uint32_t lowByte = color & 0xFF;
    const uint32_t pixelCount = width * height;
    
    for (uint32_t i = 0; i < pixelCount; ++i)
    {
        m_spi.transfer(highByte);
        m_spi.transfer(lowByte);
    }
}

#undef DisplayClass
