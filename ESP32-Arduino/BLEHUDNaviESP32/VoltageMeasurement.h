#ifndef VOLTAGEMEASUREMENT_H_INCLUDED
#define VOLTAGEMEASUREMENT_H_INCLUDED

#include "esp_adc_cal.h"

class VoltageMeasurement
{
public:
    VoltageMeasurement(int pinVoltage, int pinVoltageEnable)
    : m_pinVoltage(pinVoltage)
    , m_pinVoltageEnable(pinVoltageEnable)
    {
    }

    virtual ~VoltageMeasurement() = default;

    void begin()
    {
        pinMode(m_pinVoltageEnable, OUTPUT);
        someSetupMagic();
    }

    void end()
    {
        // without this the module won't wake up with button if powered from battery,
        // especially if entered deep sleep when powered from USB
        pinMode(m_pinVoltageEnable, INPUT_PULLUP);
    }

    float measureVolts()
    {
        float voltage = 0;
        
        // ADC_EN is the ADC detection enable port
        // If the USB port is used for power supply, it is turned on by default
        // If it is powered by battery, it needs to be set to high level
        digitalWrite(m_pinVoltageEnable, HIGH);
        delay(10); // need to wait, otherwise wrong result on battery (0.2V instead 4.1V)
        {
            uint16_t v = analogRead(m_pinVoltage);
            voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        }
        digitalWrite(m_pinVoltageEnable, LOW);

        return voltage;
    }
    
private:
    void someSetupMagic()
    {
        // no idea if it's necessary, it's copied from ESP32 TTGO FactoryTest example

        esp_adc_cal_characteristics_t adc_chars = {};
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars); //Check type of calibration value used to characterize ADC
        if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
        {
            Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
            vref = adc_chars.vref;
        }
        else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
        {
            Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
        }
        else
        {
            Serial.println("Default Vref: 1100mV");
        }
    }

private:
    int m_pinVoltage;
    int m_pinVoltageEnable;
    int vref = 1100;
};

#endif // VOLTAGEMEASUREMENT_H_INCLUDED
