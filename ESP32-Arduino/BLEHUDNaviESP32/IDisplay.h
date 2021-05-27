#ifndef IDISPLAY_H_INCLUDED
#define IDISPLAY_H_INCLUDED

#include <stdint.h>

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    virtual void Init() = 0;
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual void SendImage(const int xStart,
                           const int yStart,
                           const int width,
                           const int height,
                           const uint16_t* data) = 0;
    virtual void EnterSleepMode() = 0;
};

#endif // IDISPLAY_H_INCLUDED
