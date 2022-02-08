#ifndef BLEPERIPHERALHUD_H_INCLUDED
#define BLEPERIPHERALHUD_H_INCLUDED

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "HUDData.h"
#include <memory>

constexpr int MAX_DATA_TRANSFER_SIZE = 20;

typedef void (*HUDDataHandler)(const HUDData*);

class BLEPeripheralHUD: public BLEServerCallbacks, public BLECharacteristicCallbacks
{
public:
    BLEPeripheralHUD()
    {
    }

    void begin()
    {
        // TODO: remove these logs
        Serial.print("Temp log: BLEPeripheral begin:");
        Serial.print("m_centralConnected=");
        Serial.print((int)m_centralConnected);
        Serial.print(", m_lastActivityTime=");
        Serial.println(m_lastActivityTime);

        const char* SERVICE_UUID       = "DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86";
        const char* CHAR_INDICATE_UUID = "DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86";
        const char* CHAR_WRITE_UUID    = "DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86";

        m_pServer.reset(BLEDevice::createServer());
        m_pServer->setCallbacks(this);
        m_pService.reset(m_pServer->createService(SERVICE_UUID));

        // characteristic for write
        {
            uint32_t propertyFlags = BLECharacteristic::PROPERTY_WRITE;
            m_pCharWrite.reset(m_pService->createCharacteristic(CHAR_WRITE_UUID, propertyFlags));
            m_pCharWrite->setCallbacks(this);
            m_pCharWrite->setValue("");
        }

        // characteristic for indicate
        {
            uint32_t propertyFlags = BLECharacteristic::PROPERTY_INDICATE;
            m_pCharIndicate.reset(m_pService->createCharacteristic(CHAR_INDICATE_UUID, propertyFlags));
            m_pCharIndicate->addDescriptor(new BLE2902());
            m_pCharIndicate->setValue("");
        }

        m_pService->start();
    }

protected: // BLEServerCallbacks implementation
    void onConnect(BLEServer* pServer) override
    {
        Serial.println("onConnect");
        m_centralConnected = true;
    }

    void onDisconnect(BLEServer* pServer) override
    {
        Serial.println("onDisconnect");
        m_centralConnected = false;
    }

protected: // BLECharacteristicCallbacks implementation
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        m_lastActivityTime = millis();
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0)
        {
            m_naviData = value;
//            g_isNaviDataUpdated = true;
            Serial.print("New value, length = ");
            Serial.print(value.length());
            Serial.print(": ");
            for (int i = 0; i < value.length(); ++i)
            {
                char tmp[4] = "";
                sprintf(tmp, "%02X ", value[i]);
                Serial.print(tmp);
            }
            Serial.println();
        }
    }

public:
    bool isConnected()
    {
        return m_centralConnected;
    }

    void loop()
    {
//        BLECentral central = m_peripheral.central();
//        if (central && central.connected())
//        {
//            if (m_charNaviData.written())
//            {
//                // size = max size, length = actual data size
//                const uint8_t length = m_charNaviData.valueLength();
//                const uint8_t* data = m_charNaviData.value();
//                if (length > 0 && data)
//                {
//                    if (data[0] == 1)
//                    {
//                        const int speedOffset = 1;
//                        const int instructionOffset = 2;
//                        const int textOffset = 3;
//
//                        char message[MAX_DATA_TRANSFER_SIZE] = {};
//
//                        HUDData hud;
//                        hud.speedLimit = 0;
//                        hud.direction = DirectionNone;
//                        hud.message = message;
//
//                        if (length > speedOffset)
//                        {
//                            hud.speedLimit = data[speedOffset];
//                        }
//
//                        if (length > instructionOffset)
//                        {
//                            hud.direction = static_cast<Direction>(data[instructionOffset]);
//                        }
//
//                        if (length > textOffset)
//                        {
//                            const int textLen = length - textOffset;
//                            const char* textPtr = reinterpret_cast<const char*>(data) + textOffset;
//                            memcpy(message, textPtr, textLen);
//                            message[textLen] = 0;
//
//                            hud.messageSize = textLen;
//                        }
//                        hudHandler(&hud);
//                    }
//                }
//            }
//        }
    }

public:
    std::unique_ptr<BLEServer> m_pServer;
    std::unique_ptr<BLEService> m_pService;
    std::unique_ptr<BLECharacteristic> m_pCharWrite;
    std::unique_ptr<BLECharacteristic> m_pCharIndicate;
    bool m_centralConnected = false;
    uint32_t m_lastActivityTime = 0;
    std::string m_naviData;
};

#endif // BLEPERIPHERALHUD_H_INCLUDED
