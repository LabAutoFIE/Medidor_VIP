#include <Arduino.h>           // Librería base p/ Arduino
#include <Wire.h>              // Librería p/ comunicación I2C
#include <LiquidCrystal_I2C.h> // Librería p/ pantalla LCD I2C 16 x 2

// --- PANTALLA LCD I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección típica: 0x27 / 16 columnas y 2 filas

void setup()
{
    Serial.begin(9600);
    Serial.println("🔍 Validación modular I2C + LCD + Serial");

    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("LCD OK");

    for (byte addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("I2C detectado en 0x");
            Serial.println(addr, HEX);
        }
    }
}

void loop()
{
    // Vacío
}