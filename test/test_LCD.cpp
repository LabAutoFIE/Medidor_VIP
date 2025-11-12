#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección I2C y tamaño del LCD

const String line1 = "LCD Trabaja OK! Muy bien hecho.";                       // Texto > 16 chars
const String line2 = "Que Bueno! Esto funciona perfecto (se puede mejorar)."; // Texto > 16 chars
// TIEMPOS LCD:
const int initialDelay = 2000; // Tiempo inicial [ms]
const int scrollDelay = 350;   // Velocidad [ms]

void scrollBothLines(String text1, String text2);

void setup()
{
    Wire.begin();
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
}

void loop()
{
    scrollBothLines(line1, line2);
}

// 🔄 Función p/ desplazar texto ambas líneas (c/retardo inicial):
void scrollBothLines(String text1, String text2)
{
    String padded1 = text1 + "                ";
    String padded2 = text2 + "                ";
    size_t maxLen = max(padded1.length(), padded2.length());
    // Mostrar 1eros 16 caracteres estáticos (retardo)
    lcd.setCursor(0, 0);
    lcd.print(padded1.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(padded2.substring(0, 16));
    delay(initialDelay); // ⏳ Espera antes scroll

    // Scroll sincronizado:
    for (size_t i = 0; i <= maxLen - 16; i++)
    {
        lcd.setCursor(0, 0);
        lcd.print(padded1.substring(i, i + 16));
        lcd.setCursor(0, 1);
        lcd.print(padded2.substring(i, i + 16));
        delay(scrollDelay);
    }
}
