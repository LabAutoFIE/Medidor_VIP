#include <Arduino.h> // Librería base para Arduino
#include <Wire.h>    // Librería para comunicación I2C
#include <INA226.h>  // Librería del sensor INA226 (Rob Tillaart)

// Creo instancia sensor INA226 c/ dirección p/ defecto (0x40)
INA226 ina(0x40);          // Dirección I2C por defecto del INA226 es 0x40
const int pinPulsador = 2; // PIN D2 digital donde está conectado el pulsador

void setup()
{
    Wire.begin();                       // Inicializa el bus I2C
    Serial.begin(9600);                 // Inicializa puerto serie p/ monitoreo
    pinMode(pinPulsador, INPUT_PULLUP); // Usa resistencia interna, pulsador activo en LOW
    // VERSIÓN 1.1: Salida en formato CSV p/ fácil registro y análisis
    // Serial.println("Timestamp[s],Tension[V],Corriente[mA],Potencia[mW]"); // Encabezado CSV (quitar comentario luego probar INA226)

    // Escaneo de dispositivos I2C conectados (útil p/ verificar dirección)
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
    // Configurar tiempos de conversión p/ mayor precisión:
    ina.setBusVoltageConversionTime(INA226_4200_us);   // Canal de tensión del BUS
    ina.setShuntVoltageConversionTime(INA226_4200_us); // Canal de tensión del Shunt
    ina.setAverage(INA226_16_SAMPLES);                 // Promedia 16 muestras
}

bool medicionActiva = false;

void loop()
{

    // Lectura de tensión del bus en Volt
    float tension = ina.getBusVoltage(); // [V]
    // Verifica si se presionó el pulsador p/ activar medición y tensión >= 1,1 [V]
    if (digitalRead(pinPulsador) == LOW && tension >= 1.1)
    {
        medicionActiva = true; // Activa medición
        delay(50);             // Retardo p/ evitar rebotes pulsador
    }
    // Si la medición está activa y la tensión sigue en rango
    if (medicionActiva && tension >= 1.1)
    {
        // Lectura de corriente en miliAmper:
        float corriente = ina.getCurrent_mA(); // [mA]
        // Lectura de potencia en miliWatt:
        float potencia = ina.getPower_mW(); // [mW]

        // VERSIÓN 1.0: Salida formato legible puerto serie (comentar si se usa formato CSV)
        Serial.println("=== Medición INA226 ===");
        // Muestra los valores por puerto serie
        Serial.print("Tensión [V]: "); // Mostrar tensión en Volt
        Serial.println(tension);       // medicionActiva

        Serial.print("Corriente [mA]: "); // Mostrar corriente en miliAmper
        Serial.println(corriente);        // medicionActiva

        Serial.print("Potencia [mW]: "); // Mostrar potencia en miliWatt
        Serial.println(potencia);        // medicionActiva

        // VERSIÓN 1.1: Salida en formato CSV p/ fácil registro y análisis (comentar si se usa formato legible)
        // Obtener timestamp aproximado (solo si con módulo RTC de tiempo real como el DS3231, caso contrario usar millis)
        // unsigned long tiempo = millis() / 1000; // Tiempo en segundos desde inicio

        // Imprimir en formato CSV:
        // Serial.print(tiempo);
        // Serial.print(",");
        // Serial.print(tension, 3); // 3 decimales
        // Serial.print(",");
        // Serial.print(corriente, 3); // 3 decimales
        // Serial.print(",");
        // Serial.println(potencia, 2); // 2 decimales
    }
    // Condición: si la tensión es menor a 1,1 [V], no mide
    else if (medicionActiva && tension < 1.1)
    {
        Serial.println("⚠️ Tensión inferior a 1,1 [V]. Medición detenida."); // Alerta
        medicionActiva = false;                                             // Desactiva medición
    }

    delay(10000); // Espera 10 segundos antes de repetir
}