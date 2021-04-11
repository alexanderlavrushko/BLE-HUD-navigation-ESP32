#ifndef OLED_H_INCLUDED
#define OLED_H_INCLUDED

#define OLED_PIN_DC 17
#define OLED_PIN_DC_0       digitalWrite(OLED_PIN_DC, LOW)
#define OLED_PIN_DC_1       digitalWrite(OLED_PIN_DC, HIGH)

#define OLED_PIN_RST 16
#define OLED_PIN_RST_0      digitalWrite(OLED_PIN_RST, LOW)
#define OLED_PIN_RST_1      digitalWrite(OLED_PIN_RST, HIGH)

SPIClass* vspi = NULL;

void OLED_WriteReg(uint8_t reg)
{
    OLED_PIN_DC_0;
    vspi->write(reg);
    OLED_PIN_DC_1; //return to default mode (data mode)
}

void OLED_WriteData(uint8_t data)
{
    vspi->write(data);
}

uint16_t Color4To16bit(uint16_t color4bit)
{
    color4bit &= 0x0F;
    uint16_t color16bit = 0;

    const uint16_t maxColor4bit = 0x0F;
    const uint16_t maxColor5bit = 0x1F;
    const uint16_t maxColor6bit = 0x3F;
    
    const uint16_t red   = color4bit * maxColor5bit / maxColor4bit;
    const uint16_t green = color4bit * maxColor6bit / maxColor4bit;
    const uint16_t blue  = color4bit * maxColor5bit / maxColor4bit;

    //color 16 bit: rrrrrggg gggbbbbb
    color16bit |= red << 11;
    color16bit |= green << 5;
    color16bit |= blue;
    
//    if (color4bit & 0b10000000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 12; //red, 5 bit
//    if (color4bit & 0b01000000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 7;  //green, 6 bit
//    if (color4bit & 0b00100000 || (color4bit & 0xF0) == 0) color16bit |= (0x0F & color4bit) << 1;  //blue, 5 bit

    return color16bit;
}

void OLED_InitReg(bool isHorizontalMirror)
{
    OLED_WriteReg(0xfd);    //unlock commands
    OLED_WriteData(0x12);
    OLED_WriteReg(0xfd);    //unlock commands
    OLED_WriteData(0xb1);

    OLED_WriteReg(0xae);//--turn off oled panel
    OLED_WriteReg(0xa4);    //normal display mode
    
    OLED_WriteReg(0x15);    //set column address
    OLED_WriteData(0x00);    //start column   0
    OLED_WriteData(0x7f);    //end column   127
    
    OLED_WriteReg(0x75);    //set row address
    OLED_WriteData(0x00);    //start row   0
    OLED_WriteData(0x7f);    //end row   127
    
    //command A0
    //SSD1351
    //bit0 - 0=horizontal address increment; 1=vertical address increment
    //bit1 - 0=left to right; 1=right to left
    //bit2 - 0=color sequence A-B-C; 1=color sequence C-B-A
    //bit4 - 0=from up to down; 1=from down to up
    //bit5 - 0=disable split odd/even; 1=enable split odd/even
    //bit7:6 - 00/01=65k color mode; 10=262k; 11=262k 16-bit format 2
    OLED_WriteReg(0xa0);    //gment remap
    OLED_WriteData(0x74);    //74 in sample
    
    OLED_WriteReg(0xa1);    //start line
    OLED_WriteData(0x00);
    
    OLED_WriteReg(0xa2);    //display offset
    OLED_WriteData(0x00);

    OLED_WriteReg(0xab);    //function selection
    OLED_WriteData(0x01);    //[7:6] - 00=SPI, [0] - 1=enable internal Vdd regulator

    OLED_WriteReg(0xb1);    //set phase leghth
    OLED_WriteData(0xf1);    //f1; 0x32 in sample

    OLED_WriteReg(0xb2);    //enhancement display performance (4-byte), no docs
    OLED_WriteData(0x00);    //
    OLED_WriteData(0x00);    //
    OLED_WriteData(0x00);    //
    
    OLED_WriteReg(0xb3);    //set dclk ?? 0xf1 in sample
    OLED_WriteData(0x70);    //80Hz:0xc1 90Hz:0xe1   100Hz:0x00   110Hz:0x30 120Hz:0x50   130Hz:0x70     01
    
    OLED_WriteReg(0xb4);    //set VSL
    OLED_WriteData(0x00);    //there is no info in datasheet - experimentally 0x00 is the brightest
    OLED_WriteData(0x00);    //
    OLED_WriteData(0x00);    //

    OLED_WriteReg(0xb6);    //set second pre-charge
    OLED_WriteData(0x0f);    //old 0x0f

    OLED_WriteReg(0xbb);    //pre-charge voltage
    OLED_WriteData(0x17);    //0x17 default, max

    OLED_WriteReg(0xbe);    //Vcomh, 
    OLED_WriteData(0x0f);    //old 0x0f, 0x05 in sample

    OLED_WriteReg(0xc1);    //contrast for colors
    OLED_WriteData(0xff);    //color A
    OLED_WriteData(0xff);    //color B
    OLED_WriteData(0xff);    //color C

    OLED_WriteReg(0xca);    //set multiplex ratio
    OLED_WriteData(0x7f);
    
    OLED_WriteData(0xc7);    //master contrast    
    OLED_WriteData(0x0f);    //max

    OLED_WriteReg(0xa6);
//    OLED_WriteReg(0xaf);   //display on
}

void SetDrawArea(uint8_t xStart, uint8_t yStart, uint8_t width, uint8_t height)
{
    OLED_WriteReg(0x15);
    OLED_WriteData(xStart);
    OLED_WriteData(xStart + width - 1);
    
    OLED_WriteReg(0x75);
    OLED_WriteData(yStart);
    OLED_WriteData(yStart + height - 1);
}

void FillColor(uint16_t color, uint8_t xStart, uint8_t yStart, uint8_t width, uint8_t height)
{
    SetDrawArea(xStart, yStart, width, height);
    OLED_WriteReg(0x5c); //write RAM mode
    for (uint16_t i = 0; i < width * height; ++i)
    {
        OLED_WriteData(static_cast<uint8_t>(color >> 8));
        OLED_WriteData(static_cast<uint8_t>(color));
    }
}

void SendProgmemConverting4to16bit(const uint8_t* pBmp, uint16_t sizeBytesInput)
{
    OLED_WriteReg(0x5c); //write RAM mode
    for (uint16_t i = 0; i < sizeBytesInput; ++i)
    {
        uint16_t data = pgm_read_byte(pBmp);
        
        uint16_t firstPixel = Color4To16bit((data & 0x0F));
        uint16_t secondPixel = Color4To16bit((data & 0xF0) >> 4);
        
        OLED_WriteData(static_cast<uint8_t>(firstPixel >> 8));
        OLED_WriteData(static_cast<uint8_t>(firstPixel));
        OLED_WriteData(static_cast<uint8_t>(secondPixel >> 8));
        OLED_WriteData(static_cast<uint8_t>(secondPixel));
        
        ++pBmp;
    }
}

void SendAsIsProgmem(const uint8_t* pBmp, uint16_t sizeBytes)
{
    for (uint16_t i = 0; i < sizeBytes; ++i)
    {
        OLED_WriteData(pgm_read_byte(pBmp));
        ++pBmp;
    }
}

#endif //OLED_H_INCLUDED
