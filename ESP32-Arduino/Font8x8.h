#ifndef FONT8X8_H_INCLUDED
#define FONT8X8_H_INCLUDED

#include "Font8x8Common.h"
#include "Font8x8Chars.h"

namespace Font8x8
{

const byte* GetCharProgmem(const byte* ptr)
{
    memcpy_P(tmp, ptr, HEIGHT);

    byte charMask = 0;
    for (int i = 0; i < HEIGHT; ++i)
        charMask |= tmp[i];

    byte leftMargin = 0;
    for (; charMask != 0 && (charMask & (1 << 7)) == 0; charMask <<= 1)
         ++leftMargin;

    for (int i = 0; i < HEIGHT; ++i)
        tmp[i] <<= leftMargin;
        
    return tmp;
}

const byte* FormatChar(char c)
{
    int index = static_cast<int>(static_cast<unsigned char>(c));
    if (index > 127)
    {
        index = static_cast<int>('?');
    }
    const byte* pGlyph = (const byte*)pgm_read_dword(&(ascii[index]));
    return GetCharProgmem(pGlyph);
}

int CharWidth(const byte* buffer)
{
    byte charMask = 0;
    for (int i = 0; i < HEIGHT; ++i)
        charMask |= buffer[i];

    while (charMask != 0 && (charMask & (1 << 7)) == 0)
        charMask <<= 1;

    int width = 0;
    for (; charMask != 0; charMask <<= 1)
        ++width;

    return width;
}

}

#endif //FONT8X8_H_INCLUDED
