#ifndef GFXEXTENSION_H_INCLUDED
#define GFXEXTENSION_H_INCLUDED

#include <Adafruit_GFX.h>
#include "ColorConversion.h"

void drawGrayscaleImageProgmem(Adafruit_GFX* gfx, int x, int y, int width, int height, const uint8_t* pBmp, int bitsPerPixel)
{
    colorConversionFunction convertTo16bit = (bitsPerPixel == 2) ? Color2To16bit : Color4To16bit;
    const int sizePixels = width * height;
    const int pixelsInByte = 8 / bitsPerPixel;
    
    uint8_t firstPixelMask = 0x01;
    for (int i = 1; i < bitsPerPixel; ++i)
    {
        firstPixelMask |= (1 << i);
    }

    for (int i = 0; i < sizePixels; i += pixelsInByte)
    {
        uint8_t data = pgm_read_byte(pBmp++);

        uint8_t currentPixelMask = firstPixelMask;
        for (int pixelIndex = 0; pixelIndex < pixelsInByte; ++pixelIndex, currentPixelMask <<= bitsPerPixel)
        {
            uint16_t pixelRaw = (data & currentPixelMask) >> (bitsPerPixel * pixelIndex);
            int16_t yOffset = (i + pixelIndex) / width;
            if (yOffset < height)
            {
                int16_t xOffset = (i + pixelIndex) % width;
                gfx->drawPixel(x + xOffset, y + yOffset, convertTo16bit(pixelRaw));
            }
        }
    }
}

void drawTextCentered(Adafruit_GFX* gfx, const char* text, const GFXfont* font, int textSize, const Point2D& ptCenter, uint16_t color)
{
    gfx->setFont(font);
    gfx->setTextSize(textSize);

    Rect2D rcMeasured = {};
    gfx->getTextBounds(text, 0, 0, &rcMeasured.x, &rcMeasured.y, (uint16_t*)&rcMeasured.width, (uint16_t*)&rcMeasured.height);
    
    int16_t cursorX = ptCenter.x - rcMeasured.centerX();
    int16_t cursorY = ptCenter.y - rcMeasured.centerY();
    gfx->setCursor(cursorX, cursorY);
    gfx->setTextColor(color);
    gfx->print(text);
}

#endif // GFXEXTENSION_H_INCLUDED
