#ifndef BLEPERIPHERALHUD_H_INCLUDED
#define BLEPERIPHERALHUD_H_INCLUDED

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "HUDDataParser.h"
#include <memory>
#include <functional>

typedef std::function<void(bool isConnected)> ConnectionChangedCallback;
typedef std::function<void(const HUDData& hudData)> HUDDataChangedCallback;

class BLEPeripheralHUD: public BLEServerCallbacks, public BLECharacteristicCallbacks
{
public:
    BLEPeripheralHUD()
    : m_serviceUUID     ("DD3F0AD1-6239-4E1F-81F1-91F6C9F01D86")
    , m_charIndicateUUID("DD3F0AD2-6239-4E1F-81F1-91F6C9F01D86")
    , m_charWriteUUID   ("DD3F0AD3-6239-4E1F-81F1-91F6C9F01D86")
    {
    }

    void begin()
    {
        m_pServer.reset(BLEDevice::createServer());
        m_pServer->setCallbacks(this);
        m_pService.reset(m_pServer->createService(m_serviceUUID.c_str()));

        // characteristic for write
        {
            uint32_t propertyFlags = BLECharacteristic::PROPERTY_WRITE;
            m_pCharWrite.reset(m_pService->createCharacteristic(m_charWriteUUID.c_str(), propertyFlags));
            m_pCharWrite->setCallbacks(this);
            m_pCharWrite->setValue("");
        }

        // characteristic for indicate
        {
            uint32_t propertyFlags = BLECharacteristic::PROPERTY_INDICATE;
            m_pCharIndicate.reset(m_pService->createCharacteristic(m_charIndicateUUID.c_str(), propertyFlags));
            m_pCharIndicate->addDescriptor(new BLE2902());
            m_pCharIndicate->setValue("");
        }

        m_pService->start();
    }

    const char* getServiceUUID()
    {
        return m_serviceUUID.c_str();
    }

    void setConnectionChangedCallback(ConnectionChangedCallback&& callback)
    {
        m_connectionChangedCallback = std::move(callback);
    }

    void setHUDDataChangedCallback(HUDDataChangedCallback&& callback)
    {
        m_hudDataChangedCallback = std::move(callback);
    }

    void loop()
    {
        bool isConnected = m_isCentralConnected;
        if (isConnected != m_lastIsCentralConnected)
        {
            m_lastIsCentralConnected = isConnected;
            if (m_connectionChangedCallback)
            {
                m_connectionChangedCallback(isConnected);
            }
        }

        bool isNewData = m_isNewDataInWriteBuffer;
        if (isNewData)
        {
            m_isNewDataInWriteBuffer = false;
            std::string writeBuffer = m_writeBuffer;
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(writeBuffer.c_str());
            const uint16_t bufferSize = writeBuffer.size();
            const int HEADER_SIZE = 1;
            if (bufferSize >= HEADER_SIZE)
            {
                const int CONTENT_TYPE_HUDDATA = 1;
                if (buffer[0] == CONTENT_TYPE_HUDDATA)
                {
                    HUDData hudData = HUDDataParser::parse(buffer + HEADER_SIZE, bufferSize - HEADER_SIZE);
                    if (m_hudDataChangedCallback)
                    {
                        m_hudDataChangedCallback(hudData);
                    }
                }
            }
        }

        if (m_isCentralConnected)
        {
            uint32_t time = millis();
            if (time - m_lastActivityTime > 4000)
            {
                m_lastActivityTime = time;
                m_pCharIndicate->indicate();
            }
        }
    }

protected: // BLEServerCallbacks implementation
    void onConnect(BLEServer* pServer) override
    {
        Serial.println("onConnect");
        m_isCentralConnected = true;
        m_lastActivityTime = millis();
    }

    void onDisconnect(BLEServer* pServer) override
    {
        Serial.println("onDisconnect");
        m_isCentralConnected = false;
    }

protected: // BLECharacteristicCallbacks implementation
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        m_lastActivityTime = millis();
        std::string value = pCharacteristic->getValue();

        m_writeBuffer = value;
        m_isNewDataInWriteBuffer = true;

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

private:
    std::unique_ptr<BLEServer> m_pServer;
    std::unique_ptr<BLEService> m_pService;
    std::unique_ptr<BLECharacteristic> m_pCharWrite;
    std::unique_ptr<BLECharacteristic> m_pCharIndicate;
    std::string m_serviceUUID;
    std::string m_charIndicateUUID;
    std::string m_charWriteUUID;

    bool m_isCentralConnected = false;
    bool m_lastIsCentralConnected = false;
    uint32_t m_lastActivityTime = 0;
    bool m_isNewDataInWriteBuffer = false;
    std::string m_writeBuffer;

    ConnectionChangedCallback m_connectionChangedCallback;
    HUDDataChangedCallback m_hudDataChangedCallback;
};

#endif // BLEPERIPHERALHUD_H_INCLUDED
