#ifndef GFXEXTENSION_H_INCLUDED
#define GFXEXTENSION_H_INCLUDED

#include <Adafruit_GFX.h>
#include "ColorConversion.h"

void draw4bitImageProgmem(Adafruit_GFX* gfx, int x, int y, int width, int height, const uint8_t* pBmp)
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

        gfx->drawPixel(xLeft, yLeft, Color4To16bit(leftPixel));
        gfx->drawPixel(xRight, yRight, Color4To16bit(rightPixel));
    }
}

#endif // GFXEXTENSION_H_INCLUDED
