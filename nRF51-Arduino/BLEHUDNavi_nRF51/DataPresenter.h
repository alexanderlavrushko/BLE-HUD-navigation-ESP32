#ifndef DATAPRESENTER_H_INCLUDED
#define DATAPRESENTER_H_INCLUDED

#include "HUDData.h"
#include "HUDLayout.h"
#include "GFXExtension.h"
#include "ImagesOther.h"
#include "ImagesDirections.h"
#include "FontCompact6x8.h"
#include "FontSpeedLimit.h"
#include "FontSpeedLimitWide.h"

class DataPresenter
{
public:
    explicit DataPresenter(Adafruit_GFX* gfx, const HUDLayout* layout)
    : m_gfx(gfx)
    , m_layout(layout)
    {
    }

    DataPresenter() = delete;

public:
    virtual void presentData(const HUDData* hud)
    {
        m_gfx->fillScreen(0x0000);
        drawSpeed(hud->speedLimit);
        drawDirection(hud->direction);
        drawHUDMessage(hud->message);
    }

protected:
    void drawSpeed(uint8_t speed)
    {
        if (speed == 0)
        {
            return;
        }
    
        const Rect2D& rc = m_layout->rcSpeed;
        if (1)
        {
            m_gfx->drawRGBBitmap(rc.x, rc.y, reinterpret_cast<const uint16_t*>(IMG_Speed64x64_16b), rc.width, rc.height);
        }
        else
        {
            int16_t radius = min(rc.width / 2, rc.height / 2) - 1;
            Point2D center = { rc.x + radius, rc.y + radius };
            m_gfx->fillCircle(center.x, center.y, radius, 0xFFFF);
            m_gfx->fillCircle(center.x, center.y, radius - 1, 0xF800);
            m_gfx->fillCircle(center.x, center.y, radius - 6, 0xFFFF);
        }
    
        m_gfx->setTextSize(1);
        const GFXfont* font = speed < 100 ? (&FontSpeedLimitWide) : (&FontSpeedLimit);
        m_gfx->setFont(font);
    
        char str[4] = {};
        sprintf(str, "%u", (unsigned int)speed);
        Rect2D rcText = {};
        m_gfx->getTextBounds(str, 0, 0, &rcText.x, &rcText.y, (uint16_t*)&rcText.width, (uint16_t*)&rcText.height);
    
        int16_t cursorX = rc.centerX() - rcText.centerX();
        int16_t cursorY = rc.centerY() - rcText.centerY();
    
        m_gfx->setCursor(cursorX, cursorY);
        m_gfx->setTextColor(0x0000);
        m_gfx->print(str);
    }
    
    void drawDirection(uint8_t direction)
    {
        const uint8_t* imageProgmem = imageFromDirection(direction);
        if (imageProgmem)
        {
            const Rect2D& rc = m_layout->rcInstruction;
            draw4bitImageProgmem(m_gfx, rc.x, rc.y, rc.width, rc.height, imageProgmem);
        }
    }
    
    void drawHUDMessage(const char* message)
    {
        m_gfx->setFont(&FontCompact6x8);
    
        const int initialScale = 3;
        const int minScale = 3;
        const Rect2D& rcWanted = m_layout->rcMessage;
        Rect2D rcMeasured = {};
    
        // adjust text scale and measure text bounds
        int scale = initialScale;
        for (; scale >= minScale; --scale)
        {
            m_gfx->setTextSize(scale);
            m_gfx->getTextBounds(message, 0, 0,
                                 &rcMeasured.x, &rcMeasured.y,
                                 (uint16_t*)&rcMeasured.width, (uint16_t*)&rcMeasured.height);
            if (rcMeasured.width <= rcWanted.width && rcMeasured.height <= rcWanted.height)
            {
                break;
            }
        }
    
        // fill text background, if necessary
        if (rcMeasured.height > rcWanted.height)
        {
            m_gfx->fillRect(rcWanted.x, rcWanted.y, rcMeasured.width, rcMeasured.height, 0x0000);
        }
    
        // for custom fonts the cursor is bottom left corner of the first letter
        // (not top left corner as you might expect)
        int16_t cursorX = rcWanted.x;
        int16_t cursorY = rcWanted.y - rcMeasured.y;
    
        m_gfx->setCursor(cursorX, cursorY);
        m_gfx->setTextColor(0xFFFF);
        m_gfx->print(message);
    }
    
    const uint8_t* imageFromDirection(uint8_t direction)
    {
        switch (direction)
        {
            case DirectionNone: return nullptr;
            case DirectionStart: return IMG_directionWaypoint;
            case DirectionEasyLeft: return IMG_directionEasyLeft;
            case DirectionEasyRight: return IMG_directionEasyRight;
            case DirectionEnd: return IMG_directionWaypoint;
            case DirectionVia: return IMG_directionWaypoint;
            case DirectionKeepLeft: return IMG_directionKeepLeft;
            case DirectionKeepRight: return IMG_directionKeepRight;
            case DirectionLeft: return IMG_directionLeft;
            case DirectionOutOfRoute: return IMG_directionOutOfRoute;
            case DirectionRight: return IMG_directionRight;
            case DirectionSharpLeft: return IMG_directionSharpLeft;
            case DirectionSharpRight: return IMG_directionSharpRight;
            case DirectionStraight: return IMG_directionStraight;
            case DirectionUTurnLeft: return IMG_directionUTurnLeft;
            case DirectionUTurnRight: return IMG_directionUTurnRight;
            case DirectionFerry: return IMG_directionFerry;
            case DirectionStateBoundary: return IMG_directionStateBoundary;
            case DirectionFollow: return IMG_directionFollow;
            case DirectionMotorway: return IMG_directionMotorway;
            case DirectionTunnel: return IMG_directionTunnel;
            case DirectionExitLeft: return IMG_directionExitLeft;
            case DirectionExitRight: return IMG_directionExitRight;
            case DirectionRoundaboutRSE: return IMG_directionRoundaboutRSE;
            case DirectionRoundaboutRE: return IMG_directionRoundaboutRE;
            case DirectionRoundaboutRNE: return IMG_directionRoundaboutRNE;
            case DirectionRoundaboutRN: return IMG_directionRoundaboutRN;
            case DirectionRoundaboutRNW: return IMG_directionRoundaboutRNW;
            case DirectionRoundaboutRW: return IMG_directionRoundaboutRW;
            case DirectionRoundaboutRSW: return IMG_directionRoundaboutRSW;
            case DirectionRoundaboutRS: return IMG_directionRoundaboutRS;
            case DirectionRoundaboutLSE: return IMG_directionRoundaboutLSE;
            case DirectionRoundaboutLE: return IMG_directionRoundaboutLE;
            case DirectionRoundaboutLNE: return IMG_directionRoundaboutLNE;
            case DirectionRoundaboutLN: return IMG_directionRoundaboutLN;
            case DirectionRoundaboutLNW: return IMG_directionRoundaboutLNW;
            case DirectionRoundaboutLW: return IMG_directionRoundaboutLW;
            case DirectionRoundaboutLSW: return IMG_directionRoundaboutLSW;
            case DirectionRoundaboutLS: return IMG_directionRoundaboutLS;
        }
        return IMG_directionError;
    }

protected:
    Adafruit_GFX* m_gfx;
    const HUDLayout* m_layout;
};

#endif // DATAPRESENTER_H_INCLUDED
