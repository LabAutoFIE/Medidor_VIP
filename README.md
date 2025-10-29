# Laboratorio de Automotores:
<div align="center">
  <img width="150" height="150" alt="AutoSolo3_circle" src="https://github.com/user-attachments/assets/38627ca5-195e-4675-b25b-05d910e9b540" />
</div>

# Medidor de corriente potencia y tensión por I2C con CJMCU-226 / INA226:
**Institución:** FIE Facultad de Ingeniería del Ejército "Grl Div Manuel N. Savio"
**Autor:** Prof. Ing. Gerhard E.RAITH  
**Fecha:** [28/10/20225]  
**Versión:** 1.0  
**Idioma:** Español / Inglés

## Descripción técnica:
Este sistema permite la medición de corriente, tensión y potencia mediante el módulo CJMCU-226 basado en el chip INA226 de Texas Instruments. La comunicación se realiza por protocolo I2C con una placa Arduino Uno. El sistema está diseñado para monitoreo energético en dispositivos de laboratorio.
En este caso en particular, se produce la descarga de una pila, registrando la corriente y la tensión de descarga de acuerdo a la RL *(resistencia de carga)*. Así se obtienen las curvas de tensión [V] potencia [W] e intensidad [A] respecto del tiempo [t]

## Estructura Proyecto:
**medidor_vip/**

├── **documents/**
│   └── INA226_TexasInstrumets-DataSheet-EN.pdf
├── **img/**
│   └── direcciones-pin_direcciones-esclavas.png
│   └── esquema_conexion_ardruino-ina.png
│   └── esquema_interno_cjmcu-226.png
│   └── implementacion_cjmcu-226.png
│   └── modulo-cjmcu-226.png
├── **include/**
├── **lib/**
├── **src/**
├── **test/**
├── .gitignore
├── platformio.ini
├── README.md

### 📦 Dependencias gestionadas por PlatformIO:
- Todas las bibliotecas externas (como `INA226`) se instalan automáticamente desde `platformio.ini`.
- No se versiona la carpeta `.pio/` para mantener el repositorio limpio y reproducible.
- Para compilar: `pio run` o abrir con PlatformIO en VS Code.
### 🧪 Validación de entorno PlatformIO:
- Entorno: PlatformIO + VS Code
- Placa: Arduino Uno
- Biblioteca: robtillaart/INA226@^0.6.4 *(desde terminal: pio lib install "robtillaart/INA226")*
- Archivo principal: `src/main.cpp`
- Dependencias gestionadas en `platformio.ini`

### Especificaciones CJMCU-226:
![Modulo CJMCU-226](img/modulo-cjmcu-226.png)
- Tensión de alimentación: 2,7 a 5,5 [V]
- Consumo de corriente: 300 [uA] *(típico)*
- Rango de tensión de bus: 0 a 36 [V]
- Relación de rechazo modo común: 140 [dB] *(CMR)*
- Compensación máxima : 10 [uV] *(Ofset)*
- Error máximo de ganancia: 0,1 [%]
- Opciones de promedio configurables
- 16 direcciones programables
- Informes de corriente, voltaje y potencia
- Paquete DGS (VSSOP) de 10 pines
- Temperatura de funcionamiento: -40 a 125 [°C]

#### Pines:
- **VBUS:** entrada de tensión de bus
- **SDA/SCL:** datos de bus I2C y líneas de señal de reloj
- **ALERTA:** Salida de alarma multifunción
- **A0/A1:** línea de dirección I2C
- **IN+/IN-:** entrada analógica no inversora, entrada analógica inversora

## Objetivos:
- Medición de tensión de bus (hasta 36[V])
- Medición corriente mediante resistencia shunt
- Calcular potencia instantánea
- Exportar datos por puerto serie para análisis

## Componentes:
- Arduino Uno
- Módulo CJMCU-226 (INA226)
- Resistencia shunt de 0.1 [Ω]
- Pila comercial de 1,5 [V]
- Carga (Resistencia para producir la profundidad de descarga objetivo)

## Conexión:
![Esquema de conexión ardruino - INA226](img/esquema_conexion_ardruino-ina.png)

### 🔌 Tabla Conexiones eléctricas:

| Componente | Pin terminal |   Arduino Uno  |              Descripción técnica                    |
|------------|--------------|----------------|-----------------------------------------------------|
| CJMCU-226  |      VCC     |       5V       | Alimentación del sensor desde el regulador de 5V    |
| CJMCU-226  |      GND     |       GND      | Tierra común entre sensor y microcontrolador        |
| CJMCU-226  |      SDA     |       A4       | Línea de datos I2C para comunicación bidireccional  |
| CJMCU-226  |      SCL     |       A5       | Línea de reloj I2C para sincronización de datos     |
| CJMCU-226  |      IN+     | Fuente positiva| Entrada de corriente desde la fuente hacia la carga |
| CJMCU-226  |      IN−     | Carga positiva | Salida hacia el dispositivo bajo prueba             |

## Validación experimental:
Se recomienda realizar pruebas con cargas resistivas conocidas para validar la lectura de corriente. Comparar con multímetro de referencia y registrar desviaciones.

## Calibración:
La calibración se realiza ajustando el valor de la resistencia shunt en el código fuente. Para una resistencia Shunt de 0,1 [Ω] => 0,8 [A]:

### ⚠️ Validación de calibración INA226
- Límite del sensor: 81.92 mV (máx)
- Margen de seguridad: 81.9 mV
- Fórmula: shunt × corriente_máx ≤ 81.9 mV
- Configuración usada: 0.1 Ω, 0.8 A → 80 mV ✅

```cpp
ina.setMaxCurrentShunt(0.8, 0.1);
```

### 🔧 Configuración del sensor INA226
- Dirección I2C: 0x40 (por defecto)
- Resistencia shunt: 0.1 Ω
- Corriente máxima esperada: 0.8 A
- Voltaje máximo en shunt: 80 mV (dentro del límite de 81.9 mV)
- Biblioteca: robtillaart/INA226@0.6.4
- Lecturas:
  - Tensión: `getBusVoltage()` → V
  - Corriente: `getCurrent_mA()` → mA
  - Potencia: `getPower_mW()` → mW

---

### 📦 Exportación y trazabilidad:

## Exportación de datos
Los datos se pueden capturar por puerto serie y exportar a CSV mediante software como PuTTY, CoolTerm o Arduino Serial Plotter.

## Trazabilidad:
Se recomienda documentar cada medición con:
- Fecha y hora
- Condiciones de medición *(elemento, carga, etc.)*
- Configuración del sistema *(diagrama eléctrico)*
- Resultados esperados vs. medidos

---

## 📍 Ubicación y contacto:
**Facultad de Ingeniería del Ejército "Grl. Div. Manuel N. Savio"**  
<img src="https://img.icons8.com/color/48/marker--v1.png" alt="Dirección" width="20" height="20" style="vertical-align:middle;"/> Av. Cabildo 15, C1426AAA Ciudad Autónoma de Buenos Aires, Argentina   
📞 Teléfono: (+54 11) 4779-3300  
<img src="https://img.icons8.com/color/48/new-post.png" alt="Email" width="20" height="20" style="vertical-align:middle;"/> e-mail Institucional: [info@fie.undef.edu.ar](mailto:info@fie.undef.edu.ar)  
<img src="https://img.icons8.com/color/48/new-post.png" alt="Email" width="20" height="20" style="vertical-align:middle;"/> e-mail Laboratorio: [automotores@fie.undef.edu.ar](mailto:automotores@fie.undef.edu.ar)  
🌐 Sitio web: [www.fie.undef.edu.ar](https://www.fie.undef.edu.ar)  
📌 [Google Maps](https://www.google.com/maps?q=Av.+Cabildo+15,+C1426+Ciudad+Aut%C3%B3noma+de+Buenos+Aires,+Argentina)  
<a href="https://web.whatsapp.com/send?phone=5491138569689&text=Hola%2C+quisiera+consultar+sobre+el+Laboratorio+de+Automotores." target="_blank">
  <img src="https://img.icons8.com/color/48/whatsapp--v1.png" alt="WhatsApp" width="20" height="20" style="vertical-align:middle;"/> Mensaje Institucional FIE
</a>  