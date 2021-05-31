#ifndef DISPLAYST7735S80X160_H_INCLUDED
#define DISPLAYST7735S80X160_H_INCLUDED

#include <SPI.h>

class Display_ST7735S_80x160
{
public:
    Display_ST7735S_80x160();

    Display_ST7735S_80x160(const Display_ST7735S_80x160&) = delete;
    Display_ST7735S_80x160(Display_ST7735S_80x160&&) = delete;
    Display_ST7735S_80x160& operator=(const Display_ST7735S_80x160&) = delete;
    Display_ST7735S_80x160& operator=(Display_ST7735S_80x160&&) = delete;

public:
    void begin();
    virtual int getWidth();
    virtual int getHeight();
    virtual void sendImage(int16_t xStart, int16_t yStart,
                           int16_t width, int16_t height,
                           const uint16_t* data);
    virtual void sendImage8(int16_t xStart, int16_t yStart,
                            int16_t width, int16_t height,
                            const uint8_t* data);
    virtual void setSleepMode(bool hasToSleep);

protected:
public:
    enum class EState
    {
        NotInitialized = 0,
        Initialized = 1,
    };
    enum class EPowerState
    {
        Sleep = 0,
        On = 1,
    };

protected:
    void setupGraphics();
    void hardwareReset();
    EState getState();
    EPowerState getPowerState();
    void setPowerState(EPowerState newState);
    void setBacklight(bool isOn);
    void waitUntilAvailable();
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendData16(uint16_t data);

protected:
    void setDrawArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
    void fillColor(uint16_t color, uint8_t x, uint8_t y, uint8_t width, uint8_t height);

private:
    SPIClass& m_spi;
    EState m_state;
    EPowerState m_powerState;
    uint32_t m_availabilityTimestampMs;
};

#endif // DISPLAYST7735S80X160_H_INCLUDED
