#include <Arduino.h> // Librería base p/ Arduino
#include <Wire.h>    // Librería p/ comunicación I2C
#include <INA226.h>  // Librería del sensor INA226 (Rob Tillaart)

// --- CONFIGURACIÓN DE PINES Y SENSORES ---
// Creo instancia sensor INA226 c/ dirección p/ defecto (0x40)
INA226 ina(0x40);          // Dirección I2C por defecto del INA226 es 0x40
const int pinPulsador = 2; // PIN D2 digital donde está conectado el pulsador

// --- VARIABLES DE ESTADO Y TEMPORIZACIÓN ---
bool medicionActiva = false; // Controla si la medición está activa o no
// Variables para el temporizador de muestreo (10 s[s])
unsigned long previousMillis = 0;
const long interval = 10000; // 10000 [ms] = 10 [s]]

void setup()
{
    Wire.begin();                       // Inicializa el bus I2C
    Serial.begin(9600);                 // Inicializa puerto serie p/ monitoreo
    pinMode(pinPulsador, INPUT_PULLUP); // Usa resistencia interna, pulsador activo en LOW

    // VERSIÓN 1.1: Salida formato CSV p/ fácil registro y análisis (Descomentar si se usa el formato CSV)
    // Serial.println("Timestamp[s],Tension[V],Corriente[mA],Potencia[mW]"); // Encabezado CSV (quitar comentario luego probar INA226)

    // --- 1. Escaneo de dispositivos I2C conectados (útil p/ verificar dirección y depuración) ---
    Serial.println("--- Escaneo I2C ---");
    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Dispositivo I2C encontrado en 0x");
            Serial.println(address, HEX);
        }
    }
    Serial.println("--- Fin Escaneo I2C ---");

    // 2. --- Inicializo y Calibración sensor INA226 ---
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
    Serial.println("INA226 inicializado y calibrado para 0.8 [A] con Rs = 0.1 [Ω].");

    // Configurar tiempos de conversión p/ mayor precisión:
    ina.setBusVoltageConversionTime(INA226_4200_us);   // Canal de tensión BUS (más lento = más preciso)
    ina.setShuntVoltageConversionTime(INA226_4200_us); // Canal de tensión Shunt
    ina.setAverage(INA226_16_SAMPLES);                 // Promedia 16 muestras

    Serial.println("Listo. Presione pulsador p/ iniciar medición.");
}

void loop()
{
    // --------------------------------------------------------
    // Paso 1: LECTURA DE TENSIÓN
    // Es la base p/ lógica de inicio y detención de la medición
    // --------------------------------------------------------
    float tension = ina.getBusVoltage(); // [V]
    // --------------------------------------------------------
    // Paso 2: LóÓGICA INICIO.
    // Solo se activa si la emdición NO está activa y tensión >= 1,1 [V]
    // --------------------------------------------------------
    if (!medicionActiva && digitalRead(pinPulsador) == LOW && tension >= 1.1)
    {
        medicionActiva = true; // Activa medición
        Serial.println("✅ Medición INICIADA por pulsador.");
        delay(50); // Retardo simple p/ evitar rebotes pulsador
    }

    // --------------------------------------------------------
    // Paso 3: LÓGICA DETENCIÓN.
    // Si la medición está activa y la tensión es menor a 1,1 [V], se detiene
    // --------------------------------------------------------
    if (medicionActiva && tension < 1.1)
    {
        Serial.println("⚠️ Tensión inferior a 1,1 [V]. ❌ Medición detenida."); // Alerta
        medicionActiva = false;                                                // Desactiva medición
    }
    // --------------------------------------------------------
    // Paso 4: LÓGICA DE MEDICIÓN PERIÓDICA
    // Solo si la medición está activa, y han transcurrido 10 s[s]
    // --------------------------------------------------------
    unsigned long currentMillis = millis();
    if (medicionActiva && (currentMillis - previousMillis >= interval))
    {
        previousMillis = currentMillis; // Actualiza el último tiempo del temporizador (muestreo)
        // Lectura de corriente en miliAmper:
        float corriente = ina.getCurrent_mA(); // [mA]
        // Lectura de potencia en miliWatt:
        float potencia = ina.getPower_mW(); // [mW]

        // --------------------------------------------------------
        // ⬇️ ELEGIR FORMATO DE SALIDA ⬇️
        // --------------------------------------------------------

        // === VERSIÓN 1.0: Salida formato legible puerto serie (comentar sección si se usa formato CSV) ===

        Serial.println("=== Medición INA226 ===");
        // Muestra los valores por puerto serie
        Serial.print("Tensión [V]: "); // Mostrar tensión en Volt
        Serial.println(tension);       // medicionActiva

        Serial.print("Corriente [mA]: "); // Mostrar corriente en miliAmper
        Serial.println(corriente);        // medicionActiva

        Serial.print("Potencia [mW]: "); // Mostrar potencia en miliWatt
        Serial.println(potencia);        // medicionActiva

        // === VERSIÓN 1.1: Salida en formato CSV p/ fácil registro y análisis (descomentar si se usa formato CSV) ===
        // Obtener timestamp aproximado (solo si con módulo RTC de tiempo real como el DS3231, caso contrario usar millis)

        unsigned long tiempo = millis() / 1000; // Tiempo en [s] desde inicio

        // Impresión formato CSV:
        Serial.print(tiempo);
        Serial.print(",");
        Serial.print(tension, 3); // 3 decimales
        Serial.print(",");
        Serial.print(corriente, 3); // 3 decimales
        Serial.print(",");
        Serial.println(potencia, 2); // 2 decimales

        // --------------------------------------------------------
    }
}