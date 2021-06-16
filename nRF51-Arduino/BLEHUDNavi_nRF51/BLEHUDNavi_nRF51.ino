/*
 * BLE HUD Navigation
 * 
 * Bluetooth Low Energy Head-up display
 * Shows navigation instructions received from a phone (e.g. "After 200 meters turn right")
 * 
 * Processor: Nordic nRF51822
 * Hardware: "extracted" from smart watch iGET FIT F2, display 80x160, driver: ST7735S
 * Arduino Core for Nordic nRF5: https://github.com/sandeepmistry/arduino-nRF5
 * Settings:
 * Board: Generic nRF51
 * Chip: 32 KB RAM (xxac)
 *       note that chip with less RAM generates a build error,
 *       because it cannot allocate an array of 80x160 pixels (1 byte per pixel)
 * SoftDevice: S110
 * Low Frequency Clock: Crystal Oscillator
 */

#include "BLEPeripheralHUD.h"
#include "ColorConversion.h"
#include "GFXAdapter8bit.h"
#include "DataPresenter.h"
#include "Display_ST7735S_80x160.h"
#include "FontCondensed16px.h"

constexpr int CANVAS_WIDTH = 80;
constexpr int CANVAS_HEIGHT = 160;
uint8_t g_canvas[CANVAS_WIDTH * CANVAS_HEIGHT] = {};

GFXAdapter8bit g_gfx(g_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
Display_ST7735S_80x160 g_display;
HUDLayout g_layout = {};
BLEPeripheralHUD g_blePeripheral;
bool g_lastConnectedState = false;
uint32_t g_lastDisconnectTime = 0;

void setup()
{
    g_layout.rcInstruction = { 8, 0, 64, 64 };
    g_layout.rcMessage = { 0, g_layout.rcInstruction.bottom(), CANVAS_WIDTH, 32 };
    g_layout.rcSpeed = { 8, g_layout.rcMessage.bottom(), 64, 64 }; 

    drawLogo();

    g_display.begin();
    redrawFromCanvas();

    // init BLE
    g_blePeripheral.begin();
}

void loop()
{
    if (g_blePeripheral.isConnected())
    {
        if (!g_lastConnectedState)
        {
            g_gfx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 0x0000);
            redrawFromCanvas();
            g_lastConnectedState = true;
        }

        g_blePeripheral.checkData([](const HUDData* hud)
        {
            DataPresenter presenter(&g_gfx, &g_layout);
            presenter.presentData(hud);
            redrawFromCanvas();
        });
    }
    else if (g_lastConnectedState)
    {
        g_lastDisconnectTime = millis();
        g_lastConnectedState = false;
    }
    else if (millis() > g_lastDisconnectTime + 3000)
    {
        g_gfx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 0x0000);

        Point2D center = { CANVAS_WIDTH / 2, CANVAS_HEIGHT - 32 };
        drawTextCentered(&g_gfx, "Disconnected", &FontCondensed16px, /*textSize=*/1, center, 0xFFFF);

        drawLogo();
        redrawFromCanvas();
    }
}

void drawLogo()
{
    drawGrayscaleImageProgmem(&g_gfx, 0, 60, 80, 40, IMG_logoTbt80x40_4b, /*bits per pixel*/4);
}

void redrawFromCanvas()
{
    g_display.sendImage8(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, g_canvas);
}
