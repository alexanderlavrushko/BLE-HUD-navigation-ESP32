#ifndef COLORCONVERSION_H_INCLUDED
#define COLORCONVERSION_H_INCLUDED

typedef uint16_t (*colorConversionFunction)(uint16_t);

static uint16_t Color2To16bit(uint16_t color2bit)
{
    // |-------------|-------------------|
    // | color 2 bit |   color 16 bit    |
    // |-------------|-------------------|
    // | grayscale   | RGB (5:6:5)       |
    // | 000000nn => | rrrrrggg gggbbbbb |
    // | 000000ab => | ababaABA BABababa |
    // |-------------|-------------------|

    color2bit &= 0x03;
    const uint16_t color5bit = (color2bit << 3) | (color2bit << 1) | (color2bit >> 1); // 000000ab => 000ababa
    const uint16_t color6bit = (color2bit << 4) | (color2bit << 2) | (color2bit);      // 000000ab => 00ababab
    const uint16_t r16 = color5bit << 11;
    const uint16_t g16 = color6bit << 5;
    const uint16_t b16 = color5bit;
    return (r16 | g16 | b16);
}

static uint16_t Color4To16bit(uint16_t color4bit)
{
    // |-------------|-------------------|
    // | color 4 bit |   color 16 bit    |
    // |-------------|-------------------|
    // | grayscale   | RGB (5:6:5)       |
    // | 0000nnnn => | rrrrrggg gggbbbbb |
    // | 0000abcd => | abcdaABC DABabcda |
    // |-------------|-------------------|

    color4bit &= 0x0F;
    const uint16_t color5bit = (color4bit << 1) | (color4bit >> 3); // 0000abcd => 000abcda
    const uint16_t color6bit = (color4bit << 2) | (color4bit >> 2); // 0000abcd => 00abcdab
    const uint16_t r16 = color5bit << 11;
    const uint16_t g16 = color6bit << 5;
    const uint16_t b16 = color5bit;
    return (r16 | g16 | b16);
}

static uint16_t Color8To16bit(uint16_t color8)
{
    // |-------------|-------------------|
    // | color 8 bit |   color 16 bit    |
    // |-------------|-------------------|
    // | RGB (3:3:2) | RGB (5:6:5)       |
    // | rrrgggbb => | rrrrrggg gggbbbbb |
    // | abcDEFgh => | abcabDEF DEFghghg |
    // |-------------|-------------------|

    uint16_t color16 = 0;
    {
        const uint16_t r8 = color8 & 0xE0; // rrrgggbb => rrr00000
        color16 |= ((r8 << 8) | (r8 << 5)) & 0xF800; // rxy00000 => rxyrx000 00000000
    }
    {
        const uint16_t g8 = color8 & 0x1C; // rrrgggbb => 000ggg00
        color16 |= (g8 << 6) | (g8 << 3); // 000gxy00 => 00000gxy gxy00000
    }
    {
        const uint16_t b8 = color8 & 0x03; // rrrgggbb => 000000bb
        color16 |= (b8 << 3) | (b8 << 1) | (b8 >> 1); // 000000bx => 00000000 000bxbxb
    }
    return color16;
}

static uint16_t Color16To8bit(uint16_t color16)
{
    // |----------------------|-------------|
    // |   color 16 bit       | color 8 bit |
    // |----------------------|-------------|
    // | RGB (5:6:5)          | RGB (3:3:2) |
    // | rrrrrggg gggbbbbb => | rrrgggbb    |
    // | RRRrrGGG gggBBbbb => | RRRGGGBB    |
    // |----------------------|-------------|

    const uint8_t r8 = (color16 & 0xE000) >> 8; // RRRrr000 00000000 => RRR00000
    const uint8_t g8 = (color16 & 0x0700) >> 6; // 00000GGG ggg00000 => 000GGG00
    const uint8_t b8 = (color16 & 0x0018) >> 3; // 00000000 000BBbbb => 000000BB
    return (r8 | g8 | b8);
}

#endif // COLORCONVERSION_H_INCLUDED
