#include <Arduino.h>
#include <Wire.h>
#include <RtcDS1307.h> // ó RtcDS3231.h -> DS3231
// --- precisión de ±2 minutos/mes - (~90 ppm)  ~24 minutos/ año---
// - Resolución de lectura: 1 segundo
RtcDS1307<TwoWire> rtc(Wire);

unsigned long previousMillis = 0;
const unsigned long interval = 10000; // 10 segundos
int actualizaciones = 0;
const int maxActualizaciones = 7; // cantidad de veces actualiza tiempo

void setup()
{
    Wire.begin();
    Serial.begin(9600);
    rtc.Begin();

    if (!rtc.IsDateTimeValid())
    {
        Serial.println("⚠️ RTC no válido o sin pila. Inicializando...");

        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        rtc.SetDateTime(compiled);

        // 🔧 Asegurarse de que el reloj esté corriendo
        rtc.SetIsRunning(true);

        Serial.println("✅ RTC inicializado con fecha de compilación.");
    }
    else
    {
        Serial.println("✅ RTC válido. No se modifica. Va a comenzar lectura periódica 7...");
    }
}

void loop()
{
    unsigned long currentMillis = millis();

    if (actualizaciones < maxActualizaciones && currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        actualizaciones++;

        RtcDateTime now = rtc.GetDateTime();

        Serial.print("⏱️ [");
        Serial.print(actualizaciones);
        Serial.print("/7] Fecha: ");
        Serial.print(now.Day());
        Serial.print("/");
        Serial.print(now.Month());
        Serial.print("/");
        Serial.print(now.Year());

        Serial.print(" Hora: ");
        Serial.print(now.Hour());
        Serial.print(":");
        Serial.print(now.Minute());
        Serial.print(":");
        Serial.println(now.Second());
    }
}