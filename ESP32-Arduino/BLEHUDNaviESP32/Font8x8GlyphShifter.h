#ifndef GLYPHSHIFTER_H_INCLUDED
#define GLYPHSHIFTER_H_INCLUDED

#include "Font8x8.h"

namespace Font8x8
{

const byte MIN_CHAR_WIDTH = 3;
const byte SPACE_BETWEEN_CHARS = 1;

class GlyphShifter
{
private:
    enum ESide
    {
        ERightSide = 0,
        ELeftSide = 1
    };

public:
    GlyphShifter() : m_spaceBeforeGlyph(0)
    {
        for (byte i = 0; i < HEIGHT; ++i)
            m_data[i] = 0;
    }
    
    bool HasGlyph()
    {
        if (m_spaceBeforeGlyph > 0)
            return true;
        return !IsDataEmpty();
    }

    void PutChar(const char c)
    {
        PutGlyph(FormatChar(c), MIN_CHAR_WIDTH, SPACE_BETWEEN_CHARS);
    }

    void PutGlyph(const byte* data, byte minWidth, byte spaceBeforeGlyph)
    {
        m_spaceBeforeGlyph = spaceBeforeGlyph;
        byte leftMargin = ComputeMargin(data, ELeftSide, minWidth);
        for (byte i = 0; i < HEIGHT; ++i)
            m_data[i] = data[i] << leftMargin;

        if (IsDataEmpty())
            m_spaceBeforeGlyph += minWidth;
    }

    void ShiftToBufferFromRight(byte* buffer)
    {
        if (m_spaceBeforeGlyph > 0)
        {
            for (byte i = 0; i < HEIGHT; ++i)
                buffer[i] = buffer[i] << 1;
            --m_spaceBeforeGlyph;
        }
        else
        {
            for (byte i = 0; i < HEIGHT; ++i)
            {
                buffer[i] = (buffer[i] << 1) | (m_data[i] >> (WIDTH - 1));
                m_data[i] = m_data[i] << 1;
            }
        }
    }

    byte ShiftLeft()
    {
        if (m_spaceBeforeGlyph > 0)
        {
            --m_spaceBeforeGlyph;
            return 0;
        }
        else
        {
            byte columnData = 0;
            for (byte i = 0; i < HEIGHT; ++i)
            {
                if (m_data[i] & (1 << 7))
                    columnData |= (1 << i);
                else
                    columnData &= ~(1 << i);
                    
                m_data[i] = m_data[i] << 1;
            }
            return columnData;
        }
    }

    void Clear()
    {
        memset(m_data, 0, HEIGHT);
    }

private:
    bool IsDataEmpty()
    {
        for (byte i = 0; i < HEIGHT; ++i)
        {
            if (m_data[i] != 0)
                return false;
        }
        return true;
    }

    static byte ComputeMargin(const byte* data, ESide side, byte minWidth)
    {
        if (minWidth > WIDTH)
            minWidth = WIDTH;

        byte border = 0;
        for (byte shift = 1; shift <= WIDTH - minWidth; ++shift)
        {
            bool isMaskOk = true;
            for (byte i = 0; i < HEIGHT; ++i)
            {
                const byte mask = (side == ELeftSide ? 0xFF >> shift : 0xFF << shift);
                if ((data[i] & mask) != data[i])
                {
                    isMaskOk = false;
                    break;
                }
            }
            if (!isMaskOk)
                break;
            border = shift;
        }
        return border;
    }

private:
    byte m_data[HEIGHT];
    byte m_spaceBeforeGlyph;
};

}

#endif // GLYPHSHIFTER_H_INCLUDED
