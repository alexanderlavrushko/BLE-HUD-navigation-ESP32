#include "OLED_SSD1351_nolib.h"
#include "Arduino.h"

namespace
{
    const int SCREEN_WIDTH = 128;
    const int SCREEN_HEIGHT = 128;
    const int OLED_PIN_DC = 17;
    const int OLED_PIN_RST = 16;
    const int SPI_FREQ = 32000000; // 32 MHz (40 MHz causes problems, about 1% of bytes are not received by display)
}

OLED_SSD1351_nolib::OLED_SSD1351_nolib()
: m_isInitialized(false)
, m_spi(NULL)
, m_width(SCREEN_WIDTH)
, m_height(SCREEN_HEIGHT)
, m_pinDC(OLED_PIN_DC)
, m_pinRST(OLED_PIN_RST)
{
}

int OLED_SSD1351_nolib::GetWidth()
{
    return m_width;
}

int OLED_SSD1351_nolib::GetHeight()
{
    return m_height;
}

void OLED_SSD1351_nolib::Init()
{
    if (m_isInitialized)
        return;

    m_isInitialized = true;

    pinMode(m_pinDC, OUTPUT);
    pinMode(m_pinRST, OUTPUT);

    m_spi = new SPIClass(VSPI);
    m_spi->begin();
    m_spi->setHwCs(true);
    m_spi->beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));

    digitalWrite(m_pinRST, HIGH);
    delay(200);

    InitReg();
    FillColor(0x0000, 0, 0, m_width, m_height);
    WriteReg(0xAF); //Turn on the OLED display
}

void OLED_SSD1351_nolib::SendImage(const int xStart,
                                             const int yStart,
                                             const int width,
                                             const int height,
                                             const uint16_t* data)
{
    SetDrawArea(xStart, yStart, width, height);

    WriteReg(0x5C); //write RAM mode
    for (int i = 0; i < width * height; ++i)
    {
        WriteColor(data[i]);
    }
}

void OLED_SSD1351_nolib::EnterSleepMode()
{
    WriteReg(0xAE); // turn off the OLED display
}

void OLED_SSD1351_nolib::WriteReg(uint8_t reg)
{
    digitalWrite(m_pinDC, LOW);
    m_spi->write(reg);
    digitalWrite(m_pinDC, HIGH); //return to default mode (data mode)
}

void OLED_SSD1351_nolib::WriteData(uint8_t data)
{
    m_spi->write(data);
}

void OLED_SSD1351_nolib::WriteColor(uint16_t color)
{
    WriteData(static_cast<uint8_t>(color >> 8));
    WriteData(static_cast<uint8_t>(color));
}

void OLED_SSD1351_nolib::InitReg()
{
    WriteReg(0xfd);    //unlock commands
    WriteData(0x12);
    WriteReg(0xfd);    //unlock commands
    WriteData(0xb1);

    WriteReg(0xae);    //--turn off oled panel
    WriteReg(0xa4);    //normal display mode

    WriteReg(0x15);     //set column address
    WriteData(0x00);    //start column   0
    WriteData(0x7f);    //end column   127

    WriteReg(0x75);     //set row address
    WriteData(0x00);    //start row   0
    WriteData(0x7f);    //end row   127

    //command A0
    //SSD1351
    //bit0 - 0=horizontal address increment; 1=vertical address increment
    //bit1 - 0=left to right; 1=right to left
    //bit2 - 0=color sequence A-B-C; 1=color sequence C-B-A
    //bit4 - 0=from up to down; 1=from down to up
    //bit5 - 0=disable split odd/even; 1=enable split odd/even
    //bit7:6 - 00/01=65k color mode; 10=262k; 11=262k 16-bit format 2
    WriteReg(0xa0);     //segment remap
    WriteData(0x74);    //74 in sample

    WriteReg(0xa1);     //start line
    WriteData(0x00);

    WriteReg(0xa2);     //display offset
    WriteData(0x00);

    WriteReg(0xab);     //function selection
    WriteData(0x01);    //[7:6] - 00=SPI, [0] - 1=enable internal Vdd regulator

    WriteReg(0xb1);     //set phase leghth
    WriteData(0xf1);    //f1; 0x32 in sample

    WriteReg(0xb2);     //enhancement display performance (4-byte), no docs
    WriteData(0x00);    //
    WriteData(0x00);    //
    WriteData(0x00);    //

    WriteReg(0xb3);     //set dclk ?? 0xf1 in sample
    WriteData(0x70);    //80Hz:0xc1 90Hz:0xe1   100Hz:0x00   110Hz:0x30 120Hz:0x50   130Hz:0x70     01

    WriteReg(0xb4);     //set VSL
    WriteData(0x00);    //there is no info in datasheet - experimentally 0x00 is the brightest
    WriteData(0x00);    //
    WriteData(0x00);    //

    WriteReg(0xb6);     //set second pre-charge
    WriteData(0x0f);    //old 0x0f

    WriteReg(0xbb);     //pre-charge voltage
    WriteData(0x17);    //0x17 default, max

    WriteReg(0xbe);     //Vcomh,
    WriteData(0x0f);    //old 0x0f, 0x05 in sample

    WriteReg(0xc1);     //contrast for colors
    WriteData(0xff);    //color A
    WriteData(0xff);    //color B
    WriteData(0xff);    //color C

    WriteReg(0xca);     //set multiplex ratio
    WriteData(0x7f);
    
    WriteData(0xc7);    //master contrast
    WriteData(0x0f);    //max

    WriteReg(0xa6);
//    WriteReg(0xaf);   //display on
}

void OLED_SSD1351_nolib::SetDrawArea(int xStart, int yStart, int width, int height)
{
    WriteReg(0x15);
    WriteData(xStart);
    WriteData(xStart + width - 1);

    WriteReg(0x75);
    WriteData(yStart);
    WriteData(yStart + height - 1);
}

void OLED_SSD1351_nolib::FillColor(uint16_t color, int xStart, int yStart, int width, int height)
{
    SetDrawArea(xStart, yStart, width, height);

    WriteReg(0x5c); //write RAM mode
    for (int i = 0; i < width * height; ++i)
    {
        WriteColor(color);
    }
}
