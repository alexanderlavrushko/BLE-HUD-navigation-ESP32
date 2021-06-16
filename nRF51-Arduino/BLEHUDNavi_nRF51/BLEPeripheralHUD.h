#ifndef BLEPERIPHERALHUD_H_INCLUDED
#define BLEPERIPHERALHUD_H_INCLUDED

#include <SPI.h>
#include <BLEPeripheral.h>
#include "HUDData.h"

constexpr int MAX_DATA_TRANSFER_SIZE = 20;

typedef void (*HUDDataHandler)(const HUDData*);

class BLEPeripheralHUD
{
public:
    BLEPeripheralHUD()
    : m_peripheral()
    , m_service("DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86")
    , m_charIndicate("DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86", BLEIndicate, MAX_DATA_TRANSFER_SIZE)
    , m_charNaviData("DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86", BLEWrite, MAX_DATA_TRANSFER_SIZE)
    {
    }

    void begin()
    {
        m_peripheral.setDeviceName("nRF51 HUD");
        m_peripheral.setLocalName("nRF51 HUD");
        m_peripheral.setAdvertisedServiceUuid(m_service.uuid());
    
        m_peripheral.addAttribute(m_service);
        m_peripheral.addAttribute(m_charIndicate);
        m_peripheral.addAttribute(m_charNaviData);
    
        m_peripheral.begin();
    }

    bool isConnected()
    {
        BLECentral central = m_peripheral.central();
        return central.connected();
    }

    void checkData(HUDDataHandler hudHandler)
    {
        BLECentral central = m_peripheral.central();
        if (central && central.connected())
        {
            if (m_charNaviData.written())
            {
                // size = max size, length = actual data size
                const uint8_t length = m_charNaviData.valueLength();
                const uint8_t* data = m_charNaviData.value();
                if (length > 0 && data)
                {
                    if (data[0] == 1)
                    {
                        const int speedOffset = 1;
                        const int instructionOffset = 2;
                        const int textOffset = 3;

                        char message[MAX_DATA_TRANSFER_SIZE] = {};

                        HUDData hud;
                        hud.speedLimit = 0;
                        hud.direction = DirectionNone;
                        hud.message = message;

                        if (length > speedOffset)
                        {
                            hud.speedLimit = data[speedOffset];
                        }

                        if (length > instructionOffset)
                        {
                            hud.direction = static_cast<Direction>(data[instructionOffset]);
                        }

                        if (length > textOffset)
                        {
                            const int textLen = length - textOffset;
                            const char* textPtr = reinterpret_cast<const char*>(data) + textOffset;
                            memcpy(message, textPtr, textLen);
                            message[textLen] = 0;

                            hud.messageSize = textLen;
                        }
                        hudHandler(&hud);
                    }
                }
            }
        }
    }

public:
    BLEPeripheral m_peripheral;
    BLEService m_service;
    BLECharacteristic m_charIndicate;
    BLECharacteristic m_charNaviData;
};

#endif // BLEPERIPHERALHUD_H_INCLUDED
