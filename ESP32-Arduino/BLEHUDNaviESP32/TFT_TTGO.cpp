#include "TFT_TTGO.h"

namespace
{
    const int SCREEN_WIDTH = 135;
    const int SCREEN_HEIGHT = 240;
}

TFT_TTGO::TFT_TTGO()
: m_width(SCREEN_WIDTH)
, m_height(SCREEN_HEIGHT)
, m_tft(SCREEN_WIDTH, SCREEN_HEIGHT)
{
}

void TFT_TTGO::Init()
{
    m_tft.init();
    m_tft.setRotation(0);
    m_tft.fillScreen(TFT_BLACK);
}

int TFT_TTGO::GetWidth()
{
    return m_width;
}

int TFT_TTGO::GetHeight()
{
    return m_height;
}

void TFT_TTGO::SendImage(const int xStart,
                         const int yStart,
                         const int width,
                         const int height,
                         const uint16_t* data)
{
    m_tft.setAddrWindow(xStart, yStart, width, height);
    m_tft.pushColors(const_cast<uint16_t*>(data), width * height, /*swap = */true);
}

void TFT_TTGO::EnterSleepMode()
{
    m_tft.fillScreen(TFT_BLACK); // avoid short blink during next wake up, fill the screen now
    digitalWrite(TFT_BL, LOW); // turn backlight off
    
    m_tft.writecommand(TFT_DISPOFF);
    m_tft.writecommand(TFT_SLPIN);
}
