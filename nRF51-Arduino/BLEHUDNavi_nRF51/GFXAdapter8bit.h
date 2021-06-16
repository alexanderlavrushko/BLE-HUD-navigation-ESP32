#ifndef GFXADAPTER8BIT_H_INCLUDED
#define GFXADAPTER8BIT_H_INCLUDED

#include <Adafruit_GFX.h>
#include "ColorConversion.h"

class GFXAdapter8bit : public Adafruit_GFX
{
public:
    GFXAdapter8bit(uint8_t* pData, int16_t width, int16_t height)
    : Adafruit_GFX(width, height)
    , m_pData(pData)
    , m_width(width)
    , m_height(height)
    {
    }

    GFXAdapter8bit() = delete;

// override
public:
    void drawPixel(int16_t x, int16_t y, uint16_t color16) override
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        {
            return;
        }
        m_pData[y * m_width + x] = Color16To8bit(color16);
    }

// 8-bit specific functionality
public:
    void drawPixel8bit(int16_t x, int16_t y, uint8_t color8)
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        {
            return;
        }
        m_pData[y * m_width + x] = color8;
    }

    void drawRGBBitmap8bitProgmem(int16_t xStart, int16_t yStart, const uint8_t* bitmap, int16_t w, int16_t h)
    {
        const uint8_t* ptr = bitmap;
        const int16_t xEnd = xStart + w;
        const int16_t yEnd = yStart + h;
        for (int16_t y = yStart; y < yEnd; ++y)
        {
            for (int16_t x = xStart; x < xEnd; ++x)
            {
                drawPixel8bit(x, y, pgm_read_byte(ptr++));
            }
        }
    }

private:
    uint8_t* m_pData;
    int16_t m_width;
    int16_t m_height;
};

#endif // GFXADAPTER8BIT_H_INCLUDED
