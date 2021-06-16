#ifndef HUDLAYOUT_H_INCLUDED
#define HUDLAYOUT_H_INCLUDED

struct Point2D
{
    int16_t x;
    int16_t y;
};

struct Rect2D
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;

    int16_t centerX() const
    {
        return x + width / 2;
    }

    int16_t centerY() const
    {
        return y + height / 2;
    }

    Point2D center() const
    {
        return { centerX(), centerY()};
    }

    int16_t right() const
    {
        return x + width;
    }

    int16_t bottom() const
    {
        return y + height;
    }
};

struct HUDLayout
{
    Rect2D rcSpeed;
    Rect2D rcInstruction;
    Rect2D rcMessage;
};

#endif // HUDLAYOUT_H_INCLUDED
