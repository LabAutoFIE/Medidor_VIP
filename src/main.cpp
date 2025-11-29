#include <Arduino.h>           // base p/ Arduino
#include <Wire.h>              // p/ comunicación I2C
#include <SdFat.h>             // p/ tarjeta SD (Greiman)
#include <SPI.h>               // p/ comunicación SPI
#include <INA226_WE.h>         // INA226 (Wollewald Electronics)
#include <LiquidCrystal_I2C.h> // p/ LCD I2C 16 x 2
#include <RtcDS1307.h>         // p/ RTC DS1307 / DS3231 (Makuna)

// --- CONFIGURACIÓN PINES y SENSORES ---
// Creo instancia sensor INA226 c/ dirección p/ defecto (0x40)
INA226_WE ina(0x40);       // Dirección I2C INA226 base: 0x40 hasta 0x4F (16 direcciones posibles)
const int pinPulsador = 2; // PIN D2 p/ pulasor
const int pinRele = 3;     // PIN D3 p/ controlar relé

// --- PANTALLA LCD I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección típica: 0x27 / 16 columnas x 2 filas
bool lcdInicializado = false;

// --- SD ---
SdFat SD;       // p/ acceso SD
SdFile archivo; // p/ manejar archivos individuales

// --- VARIABLES ESTADO y TEMPORIZACIÓN ---
bool medicionActiva = false; // Controla medición está activa o no con millis
bool releActivado = false;   // Estado relé
char nombreArchivo[7];       // Nombre logger "25.csv" → ahorro RAM
// Variables p/ el temporizador de muestreo (10 [s])
unsigned long previousMillis = 0;       // Almacena último tiempo muestreo
unsigned long inicioMedicionMillis = 0; // Marca inicio medición
enum EstadoRele
{
    RELE_APAGADO,
    RELE_ENCENDIDO
};
EstadoRele estadoRele = RELE_APAGADO;
unsigned long tiempoCambioEstado = 0; // Marca último cambio estado relé
// --- CONSTANTES DE TIEMPO ---
const long interval = 10000;               // 10000 [ms] = 10 [s] intervalo muestreo ⚙️
const long releEnciendeMillis = 180000;    // 150000 [ms] = 3 [min] Tiempo medición p/ activar relé ⚙️
const long releTiempoEsperaMillis = 60000; // 60000 [ms] = 1 [min] tiempo espera p/ desactivar relé ⚙️
// --- Calibración Corriente INA226 ---
const float offsetCorriente = -0.05; // Valor típico observado s/carga ⚙️
float corriente = 0.0;               // ✅ Inicialización segura

// --- INSTANCIA RTC ---
RtcDS1307<TwoWire> Rtc(Wire);

// --- P/ CORTE MEDICIÓN
float ultimaTension = 0.0;
float penultimaTension = 0.0;

void setup()
{
    Wire.begin();       // Inicializa BUS I2C
    Serial.begin(9600); // Inicializa RS232 p/ monitoreo 9600 baudios
    lcd.init();         // Inicializa LCD
    lcd.backlight();    // Activa luz fondo
    lcd.clear();
    lcd.setCursor(0, 0);
    // Inicializa RTC
    Rtc.Begin();
    RtcDateTime now = Rtc.GetDateTime(); // Lectura Tiempo actual

    sprintf(nombreArchivo, "%02u.csv", now.Year() % 100); // Nombre archivo
    // Inicializa SD
    if (!SD.begin(10, SD_SCK_MHZ(4))) // Pin CS p/ módulo SD
    {
        Serial.println(F("❌ Fallo inicio SD"));
        lcd.setCursor(0, 1);
        lcd.clear();
        lcd.print(F("❌ Fallo inicio SD"));
        return; // Sale setup -> falla SD
    }
    Serial.println(F(" SD ✅!"));

    // CREO/SOBREESCRIBO NOMBRE ARCHIVO c/RTC
    // Solo escribo encabezado <-> archivo NO existe
    if (!SD.exists(nombreArchivo))
    {
        if (archivo.open(nombreArchivo, O_WRITE | O_CREAT))
        {
            // Establecer fecha/hora modificación c/ RTC
            archivo.timestamp(T_CREATE, now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
            archivo.timestamp(T_WRITE, now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());

            // Escribo encabezado
            archivo.println(F("FechaHora [RTC];Tiempo [s];Tension [V];Corriente [mA];Potencia [mW];Rele 1=activo 0=apagado"));

            // Validaciones antes de cerrar
            if (!archivo.isOpen())
            {
                Serial.println(F("❌ archivo no abierto."));
            }
            if (archivo.getWriteError())
            {
                Serial.println(F("❌ error de escritura en encabezado."));
                archivo.clearWriteError(); // limpia estado
            }

            archivo.close();

            Serial.print(F("Archivo creado c/ encabezado ✅: "));
            Serial.println(nombreArchivo);
        }
        else
        {
            Serial.println(F("❌ Error al crear archivo CSV."));
        }
    }
    else
    {
        Serial.print(F("Archivo existente: "));
        Serial.println(nombreArchivo);
    }

    // CONFIGURACIÓN PINES:
    pinMode(pinPulsador, INPUT_PULLUP); // Pulsador activo en LOW
    pinMode(pinRele, OUTPUT);           // Salida p/ relé
    digitalWrite(pinRele, LOW);         // Relé apagado al inicio

    // --- Inicializo y Calibro sensor INA226 ---
    Wire.endTransmission();
    if (!ina.init())
    {
        Serial.println(F("❌ INA226 no conectado."));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("INA226 ❌"));
        lcd.setCursor(0, 1);
        lcd.print(F("Check I2C addr"));
        delay(5000); // Espera visible
        return;      // Sale setup s/ bloquear
    }
    // --- Calibración sensor: ---
    // Rs  = 0.1 Ω (shunt físico placa CJMCU-226)
    // Corriente máx esperada = 0.8 A → tensión máx shunt ≈ 80 mV
    // Asegura tensión Shunt no exceda 81.9 mV
    ina.setResistorRange(0.1);    //  Resistencia shunt 0.1 Ω ⚙️
    ina.setCorrectionFactor(1.0); // Factor corrección ⚙️
    // Configurar tiempos conversión p/ mayor precisión:
    ina.setAverage(INA226_AVERAGE_16);            // Promedia 16 muestras (1, 4, 16, 64, 128, 256, 512, 1024) ⚙️
    ina.setConversionTime(INA226_CONV_TIME_4156); // Tiempo conversión ≈ 4200 µs ⚙️

    Serial.println(F("INA226 inicializado y calibrado para ≈ 0.8 [A] con Rs = 0.1 [Ω]."));

    // --- Mensaje Final LCD ---
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("INA226 listo !"));
    lcd.setCursor(0, 1);
    lcd.print(F("Oprima pulsador"));

    Serial.println(F("Listo Presione pulsador p/ iniciar medición v 1.5"));
}

// *** FUNCIÓN ARCHIVO CSV: ***
void registrarCSV(unsigned long tiempo, float tension, float corriente, float potencia, int estadoRele)
{
    File archivo = SD.open(nombreArchivo, FILE_WRITE);
    if (archivo)
    {
        // 🕒 Obtener fecha/hora actual del RTC
        RtcDateTime now = Rtc.GetDateTime();
        // Buffer para fecha/hora con dos dígitos
        char fechaHora[20]; // 19+1 (4+5*2+2*-+1*" "=19)
        sprintf(fechaHora, "%04u-%02u-%02u %02u:%02u:%02u",
                now.Year(),
                now.Month(),
                now.Day(),
                now.Hour(),
                now.Minute(),
                now.Second());
        // Escribir fecha/hora absoluta
        archivo.print(fechaHora);
        archivo.print(";");
        // Escribir tiempo relativo y demás variables
        archivo.print(tiempo); // segundos desde inicio
        archivo.print(";");

        // Tensión
        String valorTension = String(tension, 4);
        valorTension.replace('.', ',');
        archivo.print(valorTension);
        archivo.print(";");

        // Corriente
        String valorCorriente = String(corriente, 4);
        valorCorriente.replace('.', ',');
        archivo.print(valorCorriente);
        archivo.print(";");

        // Potencia
        String valorPotencia = String(potencia, 6);
        valorPotencia.replace('.', ',');
        archivo.print(valorPotencia);
        archivo.print(";");

        // Estado del relé
        archivo.println(estadoRele); // 1 = activo, 0 = apagado

        archivo.flush(); // aseguro escritura SD
        archivo.close();
    }
    else
    {
        Serial.println(F("❌ al escribir SD."));
    }
}

// *** FUNCIÓN SALIDA LCD: ***
void mostrarLCD(float tension, float corriente, float potencia, unsigned long tiempo, bool releActivado)
{
    lcd.setCursor(0, 0);
    lcd.print(F("V"));
    lcd.print(tension, 3);
    lcd.print(F(" I"));
    lcd.print(corriente, 3);
    lcd.print(F("     ")); // ← relleno p/ limpiar residuos

    lcd.setCursor(0, 1);
    lcd.print(F("P"));
    lcd.print(potencia, 2);
    lcd.print(F("mW"));
    lcd.print(F(" t"));
    lcd.print(tiempo);
    lcd.print(F(" "));
    lcd.print(estadoRele);
    lcd.print(F("     ")); // ← relleno p/ limpiar residuos
}
void loop()
{
    // --------------------------------------------------------
    // Paso 1: LECTURA DE TENSIÓN
    // Es la base p/ lógica de inicio y detención de la medición
    // --------------------------------------------------------
    float tension = ina.getBusVoltage_V(); // [V]
    // --------------------------------------------------------
    // Paso 2: LÓGICA INICIO.
    // Solo activa <-> medición NO está activa y tensión >= 1,0 [V]
    // --------------------------------------------------------
    static unsigned long tiempoEstable = 0;
    if (!medicionActiva && digitalRead(pinPulsador) == LOW && tension >= 1.0) // ⚙️
    {
        if (millis() - tiempoEstable > 500) // tensión estable x 500 ms
        {
            medicionActiva = true;                 // Activa medición
            inicioMedicionMillis = millis();       // Marca inicio medición ⚠️ millis() se desborda (cada ~50 días)
            previousMillis = inicioMedicionMillis; // reinicia temporizador muestreo
            lcdInicializado = false;               // ← reinicia estado LCD
            Serial.println(F("Medición INICIADA x pulsador ✅"));
            delay(50); // Retardo p/ evitar rebotes pulsador
        }
    }
    else
    {
        tiempoEstable = millis(); // reinicia si no cumple condición
    }

    // --- Lógica Activación / Apagado relé ---
    if (medicionActiva)
    {
        unsigned long tiempoActual = millis();
        unsigned long tiempoTranscurrido = tiempoActual - tiempoCambioEstado;

        if (estadoRele == RELE_APAGADO && tiempoTranscurrido >= releEnciendeMillis)
        {
            // Encender relé
            digitalWrite(pinRele, HIGH);
            estadoRele = RELE_ENCENDIDO;
            tiempoCambioEstado = tiempoActual;
            Serial.println(F("🔔 Relé ENCENDIDO (inicio ciclo medición Voc)"));
        }

        else if (estadoRele == RELE_ENCENDIDO && tiempoTranscurrido >= releTiempoEsperaMillis)
        {
            // Apagar relé
            digitalWrite(pinRele, LOW);
            estadoRele = RELE_APAGADO;
            tiempoCambioEstado = tiempoActual;
            Serial.println(F("⏹️ Relé APAGADO (tiempo medición V, I y P)"));
        }
    }

    // --------------------------------------------------------
    // Paso 3: LÓGICA DETENCIÓN.
    // Si la medición está activa, el relé encendido y la tensión es < 1,0 [V], se detiene
    // --------------------------------------------------------
    if (medicionActiva && estadoRele == RELE_ENCENDIDO)
    {
        // Verifica última y penúltima medición
        if (ultimaTension < 1.0 && penultimaTension < 1.0) // ⚙️ Tensión detención
        {

            Serial.println(F("⚠️ Tensión < 1,0 [V] c/ Relé activo. Medición detenida.")); // Alerta
            medicionActiva = false;                                                      // Desactiva medición
            previousMillis = 0;                                                          // Reinicia contador
            inicioMedicionMillis = 0;                                                    // Reinicia marcador inicio
            lcdInicializado = false;                                                     // ← preparo LCD p/ borrado único próx. medición

            // 🔁 Apaga relé y reinicio estado
            digitalWrite(pinRele, LOW);
            estadoRele = RELE_APAGADO;
            tiempoCambioEstado = 0;

            // 🕒 Obtener fecha/hora actual RTC
            RtcDateTime now = Rtc.GetDateTime();

            // 📝 Abrir archivo y registrar fin
            if (archivo.open(nombreArchivo, O_WRITE | O_APPEND))
            {
                archivo.println(F("Fin de medicion"));

                // 🕒 Actualizar fecha de modificación
                archivo.timestamp(T_WRITE, now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());

                archivo.flush();
                archivo.close();
            }
            else
            {
                Serial.println(F("❌ No se pudo abrir archivo p/ registrar fin"));
            }

            // 🖥️ Actualizar LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Medicion X OFF"));
            lcd.setCursor(0, 1);
            lcd.print(F("Tension < 1,0 V"));
            // 🧭 Mensaje serie
            Serial.println(F("Sistema terminó medición completa de descarga"));
        }
    }
    // --------------------------------------------------------
    // Paso 4: LÓGICA DE MEDICIÓN PERIÓDICA
    // Solo si la medición está activa, y han transcurrido 10 s[s]
    // --------------------------------------------------------
    unsigned long currentMillis = millis();
    unsigned long tiempo = (millis() - inicioMedicionMillis) / 1000;    // Tiempo [s] desde inicio c/ medición
    if (medicionActiva && (currentMillis - previousMillis >= interval)) // interval = 10000 [ms]
    {
        previousMillis = currentMillis; // Actualiza último tiempo temporizador (muestreo)
        if (!lcdInicializado)
        {
            lcd.clear(); // ← solo 1 vez
            lcdInicializado = true;
        }
        // Lectura tensión [V]
        tension = ina.getBusVoltage_V();
        // Actualizar penúltima y última tensión
        penultimaTension = ultimaTension;
        ultimaTension = tension;
        // Lectura corriente [mA]]:
        corriente = ina.getCurrent_mA() - offsetCorriente; // [mA]
        // Lectura Potencia [mW]]:
        float potencia = ina.getBusPower(); // [mW]

        // --------------------------------------------------------
        // ⬇️ FORMATO DE SALIDA ⬇️
        // --------------------------------------------------------
        // ===        Salida Formato legible puerto serie       ===

        Serial.println(F("=== Medición INA226 ==="));
        // Muestra valores x puerto serie
        Serial.print(F("Tiempo [s]: "));
        Serial.println(tiempo); // Muestra tiempo
        Serial.print(F("Tensión [V]: "));
        Serial.println(tension, 3); // Muestra tensión [V]
        Serial.print(F("Corriente [mA]: "));
        Serial.println(corriente, 3); // Muestra corriente [mA]
        Serial.print(F("Potencia [mW]: "));
        Serial.println(potencia, 5); // Muestra potencia [mW]
        Serial.print(F("Relé [0-1]: "));
        Serial.println(estadoRele); // Estado Relé

        // --------------------------------------------------------
        // === VERSIÓN 1.5: Salida SD formato CSV  ===
        registrarCSV(tiempo, tension, corriente, potencia, estadoRele == RELE_ENCENDIDO ? 1 : 0); // Función CSV
        // === SALIDA LCD 16 x 2 ===
        // --------------------------------------------------------
        mostrarLCD(tension, corriente, potencia, tiempo, estadoRele == RELE_ENCENDIDO ? 1 : 0); // Función LCD
    }
}