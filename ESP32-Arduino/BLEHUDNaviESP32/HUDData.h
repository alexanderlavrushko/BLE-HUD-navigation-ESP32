#ifndef HUDDATA_H_INCLUDED
#define HUDDATA_H_INCLUDED

#include <string>

//enum Direction
//{
//    DirectionNone = 0,
//    DirectionStart = 1,
//    DirectionEasyLeft = 2,
//    DirectionEasyRight = 3,
//    DirectionEnd = 4,
//    DirectionVia = 5,
//    DirectionKeepLeft = 6,
//    DirectionKeepRight = 7,
//    DirectionLeft = 8,
//    DirectionOutOfRoute = 9,
//    DirectionRight = 10,
//    DirectionSharpLeft = 11,
//    DirectionSharpRight = 12,
//    DirectionStraight = 13,
//    DirectionUTurnLeft = 14,
//    DirectionUTurnRight = 15,
//    DirectionFerry = 16,
//    DirectionStateBoundary = 17,
//    DirectionFollow = 18,
//    DirectionMotorway = 19,
//    DirectionTunnel = 20,
//    DirectionExitLeft = 21,
//    DirectionExitRight = 22,
//    DirectionRoundaboutRSE  = 23,
//    DirectionRoundaboutRE   = 24,
//    DirectionRoundaboutRNE  = 25,
//    DirectionRoundaboutRN   = 26,
//    DirectionRoundaboutRNW  = 27,
//    DirectionRoundaboutRW   = 28,
//    DirectionRoundaboutRSW  = 29,
//    DirectionRoundaboutRS   = 30,
//    DirectionRoundaboutLSE  = 31,
//    DirectionRoundaboutLE   = 32,
//    DirectionRoundaboutLNE  = 33,
//    DirectionRoundaboutLN   = 34,
//    DirectionRoundaboutLNW  = 35,
//    DirectionRoundaboutLW   = 36,
//    DirectionRoundaboutLSW  = 37,
//    DirectionRoundaboutLS   = 38,
//};

class HUDData
{
    uint8_t speedLimit;
//    Direction direction;
    std::string message;
};

#endif // HUDDATA_H_INCLUDED
