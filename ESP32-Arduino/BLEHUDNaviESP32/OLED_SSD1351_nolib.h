#ifndef OLED_SSD1351_NOLIB_H_INCLUDED
#define OLED_SSD1351_NOLIB_H_INCLUDED

#include "IDisplay.h"
#include <SPI.h>

class OLED_SSD1351_nolib: public IDisplay
{
public:
    OLED_SSD1351_nolib();

    // prohibited
    OLED_SSD1351_nolib(const OLED_SSD1351_nolib&) = delete;
    OLED_SSD1351_nolib& operator=(const OLED_SSD1351_nolib&) = delete;
    OLED_SSD1351_nolib(OLED_SSD1351_nolib&&) = delete;
    OLED_SSD1351_nolib& operator=(OLED_SSD1351_nolib&&) = delete;

public:
    // override
    void Init() override;
    int GetWidth() override;
    int GetHeight() override;
    void SendImage(const int xStart,
                   const int yStart,
                   const int width,
                   const int height,
                   const uint16_t* data) override;
    void EnterSleepMode() override;

private:
    void WriteReg(uint8_t reg);
    void WriteData(uint8_t data);
    void WriteColor(uint16_t color);
    void InitReg();
    void SetDrawArea(int xStart, int yStart, int width, int height);
    void FillColor(uint16_t color, int xStart, int yStart, int width, int height);
    
private:
    bool m_isInitialized;
    SPIClass* m_spi;
    int m_width;
    int m_height;
    int m_pinDC;
    int m_pinRST;
};

#endif // OLED_SSD1351_NOLIB_H_INCLUDED
