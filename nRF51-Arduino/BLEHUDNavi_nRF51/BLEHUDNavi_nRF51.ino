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

constexpr int CANVAS_WIDTH = 80;
constexpr int CANVAS_HEIGHT = 160;
uint8_t g_canvas[CANVAS_WIDTH * CANVAS_HEIGHT] = {};

GFXAdapter8bit g_gfx(g_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
Display_ST7735S_80x160 g_display;
HUDLayout g_layout = {};
BLEPeripheralHUD g_blePeripheral;

void setup()
{
    g_layout.rcInstruction = { 8, 0, 64, 64 };
    g_layout.rcMessage = { 0, g_layout.rcInstruction.bottom(), CANVAS_WIDTH, 32 };
    g_layout.rcSpeed = { 8, g_layout.rcMessage.bottom(), 64, 64 }; 

    draw4bitImageProgmem(&g_gfx, 0, 60, 80, 40, IMG_logoTbt80x40_4b);

    g_display.begin();
    redrawFromCanvas();

    // init BLE
    g_blePeripheral.begin();
}

void loop()
{
    g_blePeripheral.checkData([](const HUDData* hud)
    {
        DataPresenter presenter(&g_gfx, &g_layout);
        presenter.presentData(hud);
        redrawFromCanvas();
    });
}

void redrawFromCanvas()
{
    g_display.sendImage8(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, g_canvas);
}
