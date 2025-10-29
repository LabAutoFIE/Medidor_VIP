#include <Arduino.h> // Librería base para Arduino
#include <Wire.h>    // Librería para comunicación I2C
#include <INA226.h>  // Librería del sensor INA226 (Rob Tillaart)

// Creo instancia sensor INA226 c/ dirección p/ defecto (0x40)
INA226 ina(0x40); // Dirección I2C por defecto del INA226 es 0x40

void setup()
{
    Wire.begin();       // Inicializa el bus I2C
    Serial.begin(9600); // Inicializa puerto serie p/ monitoreo
    // Escaneo de dispositivos I2C conectados (útil para verificar dirección)
    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Dispositivo I2C encontrado en 0x");
            Serial.println(address, HEX);
        }
    }
    // Inicializo sensor INA226
    if (!ina.begin())
    {
        Serial.println("Error: INA226 no conectado.");
        while (true)
            ; // Detiene programa si no se detecta el sensor
    }
    // Calibración sensor: corriente máxima esperada = 0.8 A, shunt = 0.1 Ω
    // Esto asegura que la tensión en el shunt no exceda los 81.9 mV
    int err = ina.setMaxCurrentShunt(0.8, 0.1); // 0,8 A máx, 0.1 Ω shunt (0.1 Ω × 0.8 A = 80 mV) ✅
    if (err != INA226_ERR_NONE)
    {
        Serial.print("Error de calibración INA226: ");
        Serial.println(err);
        while (true)
            ; // Detiene el programa si la calibración falla
    }
}

void loop()
{
    // Lectura de tensión del bus en voltios
    float tension = ina.getBusVoltage();

    // Lectura de corriente en miliamperios
    float corriente = ina.getCurrent_mA();

    // Lectura de potencia en miliwatts
    float potencia = ina.getPower_mW();

    // Muestra los valores por puerto serie
    Serial.print("Tensión [V]: ");
    Serial.println(tension);

    Serial.print("Corriente [mA]: ");
    Serial.println(corriente);

    Serial.print("Potencia [mW]: ");
    Serial.println(potencia);

    delay(1000); // Espera 1 segundo antes de repetir
}
