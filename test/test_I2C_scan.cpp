#include <Arduino.h> // Librería base p/ Arduino
#include <Wire.h>    // Librería p/ comunicación I2C

// --- ESTRUCTURA DISPOSITIVOS CONOCIDOS ---
struct I2CIdentifier
{
    byte address;
    const char *description;
};

// --- TABLA IDENTIFICACIÓN DISPOSITIVOS ---
const I2CIdentifier knownDevices[] = {
    {0x20, "MCP23017 Expansor de E/S GPIO"},
    {0x27, "LCD (PCF8574 16x2 - Azul)"},
    {0x28, "SC18IS602/603 Bridge SPI ↔ I2C"},
    {0x38, "AHT10 / AHT20 Sensor humedad y temperatura"},
    {0x3E, "SX1509 Expansor GPIO con I2C"},
    {0x3F, "LCD (PCF8574A 16x2 - Verde)"},
    {0x40, "INA226 Sensor Tensión, Corriente y Potencia"},
    {0x48, "TMP102 / LM75 Sensor temperatura"},
    {0x5C, "DHT12 Sensor humedad y temperatura"},
    {0x68, "RTC DS1307 / DS3231 / MPU6050 Reloj en Tiempo Real"},
    {0x69, "MPU6050 Acelerómetro y Giróscopo"},
    {0x70, "TCA9548A / PCA954x Multiplexor I2C)"},
    {0x76, "BME280 / BMP280 Sensor ambiental: temperatura, huemdad y presión barométrica)"},
    {0x77, "BMP180 / BMP280 / BME280 Sensor ambiental (dirección alternativa)"}};
const byte knownCount = sizeof(knownDevices) / sizeof(knownDevices[0]);

void setup()
{
    Wire.begin();       // Inicializa el bus I2C
    Serial.begin(9600); // Inicializa puerto serie p/ monitoreo 9600 baudios
    delay(1000);        // Espera p/ asegurar que el monitor serie esté listo

    // --- Escaneo de dispositivos I2C conectados (útil p/ verificar dirección y depuración) ---
    Serial.println(F("--- Escaneo I2C ---"));
    Serial.println(); // línea en blanco
    Serial.println(F("Dispositivos encontrados en direcciones (Adress) 0x:"));
    Serial.println(); // línea en blanco

    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print(address, HEX); // Muestra dirección HEX
            Serial.print(F(" → "));     // flecha

            bool found = false;
            for (byte i = 0; i < knownCount; i++)
            {
                if (knownDevices[i].address == address)
                {
                    Serial.println(knownDevices[i].description);
                    found = true;
                    break;
                }
            }
            if (!found)
                Serial.println(F("Dispositivo desconocido"));
        }
    }
    Serial.println(); // línea en blanco
    Serial.println(F("--- Fin Escaneo I2C ---"));
}

void loop()
{
    // Vacío
}
