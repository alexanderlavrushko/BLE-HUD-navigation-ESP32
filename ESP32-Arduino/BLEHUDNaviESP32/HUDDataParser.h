#ifndef HUDDATAPARSER_H_INCLUDED
#define HUDDATAPARSER_H_INCLUDED

#include "HUDData.h"

class HUDDataParser
{
public:
    static HUDData parse(const uint8_t* buffer, uint16_t bufferSize)
    {
        const int SPEED_LIMIT_OFFSET = 0;
        const int INSTRUCTION_OFFSET = 1;
        const int TEXT_OFFSET = 2;

        HUDData hud;
        if (bufferSize > SPEED_LIMIT_OFFSET)
        {
            hud.speedLimit = buffer[SPEED_LIMIT_OFFSET];
        }
        if (bufferSize > INSTRUCTION_OFFSET)
        {
            hud.direction = static_cast<Direction>(buffer[INSTRUCTION_OFFSET]);
        }
        if (bufferSize > TEXT_OFFSET)
        {
            const char* textBegin = reinterpret_cast<const char*>(buffer) + TEXT_OFFSET;
            const char* textEnd = reinterpret_cast<const char*>(buffer) + bufferSize;
            hud.text = std::string(textBegin, textEnd);
        }
        return hud;
    }
};

#endif // HUDDATAPARSER_H_INCLUDED
