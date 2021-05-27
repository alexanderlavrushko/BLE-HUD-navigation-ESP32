#ifndef TFT_TTGO_H_INCLUDED
#define TFT_TTGO_H_INCLUDED

#include "IDisplay.h"

// Copy folder TFT_eSPI to Arduino/libraries from here:
// https://github.com/Xinyuan-LilyGO/TTGO-T-Display
#include <TFT_eSPI.h>

class TFT_TTGO: public IDisplay
{
public:
    TFT_TTGO();

    // prohibited
    TFT_TTGO(const TFT_TTGO&) = delete;
    TFT_TTGO& operator=(const TFT_TTGO&) = delete;
    TFT_TTGO(TFT_TTGO&&) = delete;
    TFT_TTGO& operator=(TFT_TTGO&&) = delete;

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
    TFT_eSPI m_tft;
    int m_width;
    int m_height;
};

#endif // TFT_TTGO_H_INCLUDED
