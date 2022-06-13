  /* CODE
Descripción: Este código contiene todas las librerías, variables y funciones necesarias 
para el correcto funcionamiento de la estación metereológica Ardunal 4.0. 
Autores: Daniel Correa - Erick Sandoval */

// Simbolos LCD (falta el formato binario de los primeros 6)
byte Tem[8] = {B01110, B01110, B01110, B01110, B11011, B10001, B11011, B01110}; // TEMPERATURE ICON
byte Hum[8] = {B00100, B00100, B01010, B01010, B10001, B10001, B11011, B01110}; // HUMIDITY ICON
byte PreA[8] = {B00000, B00100, B00100, B00100, B10101, B01110, B00100, B11111}; // ATMOS PRESSURE ICON
byte VelV[8] = {B00110, B01001, B00101, B11110, B00000, B11100, B01010, B00100}; // WIND VELOCITY ICON
byte Prec[8] = {B01100, B11111, B11111, B00110, B10000, B10100, B00101, B00001}; // PRECIPITATION ICON
byte RadS[8] = {B00000, B00100, B10001, B01110, B11111, B01110, B10001, B00100}; // SUN RADIATION ICON
byte EvaT[] = {B01001, B10001, B10010, B01010, B01001, B01001, B00000, B11111}; // Et0 ICON
byte SimGrado[8] = {B01110, B01010, B01110, B00000, B00000, B00000, B00000, B00000};


// ===== LIBRERÍAS =====

// === Librería para comunicación I2C ===
#include <Wire.h> 
// === Librería para comunicación SPI (Para la SD) ===
#include <SPI.h>
// === Librería del BME280 ===
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// === Librería de la SD === 
#include <SD.h>
// === Librería del RTC ===
#include <RTClib.h>
// === Librería de la pantalla LCD 16x2 ===
#include <LiquidCrystal_I2C.h>
// === Librería del teclado ===
#include <Keypad.h>

// ===== DECLARACIÓN DE VARIABLES Y CONSTRUCTORES =====

// Pines, constantes y variables para pantalla LCD 16x2
LiquidCrystal_I2C lcd(0x3f,16,2); // Constructor para una pantalla LCD de 16x2 cuya dirección I2C es 0x3f

// Pines, constantes y variables para teclado
const byte Filas = 4; // Número de filas del teclado matricial
const byte Columnas = 4; // Número de columnas del teclado matricial
char Teclas[Filas][Columnas] = {{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}}; // Declaración de la matriz de caracteres del teclado matricial
byte rowPins[Filas] = {22, 23, 24, 25}; // Pines del arduino a los que se van a conectar las filas del teclado matricial
byte colPins[Columnas] = {26, 27, 28, 29}; // Pines del arduino a los que se van a conectar las columnas del teclado matricial
Keypad Teclado = Keypad( makeKeymap(Teclas), rowPins, colPins, Filas, Columnas); // Constructor para el teclado matricial 

// Pines, constantes y variables para SD
// El modulo de SD no necesita ninguna declaración de variables o contructores para su funcionamiento

// Pines, constantes y variables para BME280
Adafruit_BME280 bme; //Constructor para sensor BME280

// Pines, constantes y variables para RTC
RTC_DS3231 rtc; //Constructor para módulo RTC

// Pines, constantes y variables para Anemómetro
// A es por Anemometro
int sensorPinA = A0; // Pin del arduino al que se va a conectar el cable de datos del anemómetro  
float sensorValueA = 0; // Variable que almacena los datos enviados por el anemómetro en términos de resolución (10 bits de resolución del arduino)
float sensorVoltageA = 0; // Variable para almacenar los datos enviados por el anemómetro pero en términos de voltaje
float WindSpeed = 0;  // Variable que almacena la velocidad del viento calculada mediante la fórmula del datasheet 
float voltageMaxA = 2.0;  // Voltaje máximo que puede entregar el sensor, según el datasheet

// Pines, constantes y variables para Piranómetro
// P es por Piranómetro
int sensorPinP = A6; // Pin del arduino al que se va a conectar el cable de datos del piranómetro  
int sensorValueP = 0; // Variable que almacena los datos enviados por el piranómetro en términos de resolución (10 bits de resolución del arduino)
float sensorVoltageP = 0; // Variable para almacenar los datos enviados por el piranómetro pero en términos de voltaje
float Irradiacion = 0; // Variable que almacena la radiación solar calculada mediante la fórmula del datasheet

// Pines, constantes y variables para Pluviómetro
byte estado1pluvi=1; // Variable que establece que el pluviómetro no ha enviado pulsos.
byte estado2pluvi=0; // Variable que establece que el pluviómetro ha enviado pulsos.
int contadorpluvi=0; // Variable para realizar el conteo de los pulsos del pluviómetro
int pinpluvi=2; // Pin del arduino al que se va a conectar el cable de datos del pluviómetro (Pin digital 2)
float mll = 0.2; // Factor de conversión. Para convertir de pulsos a mm. 
float Lluvia = 0; // Variable para almacenar los datos pero en unidades de mm, no en pulsos.
float Lluvia_dia = 0; // Variable para almacenar la precipitación total diaria.

// Arreglo con todos los datos como texto: 
// El siguiente comentario es la forma como van a rotularse los datos en el archivo .txt
// {"Fecha, Hora, Temp Prom (°C), Temp Min (°C), Temp Máx (°C), Humedad Prom (%), Humedad Min (%), Humedad Máx (%), Presión atmosférica (Pa), Velocidad del viento (m/s), Radiación solar (W/m2), Precipitación (mm), ET0 (mm), ETc (mm)"}
String Datos[] = {"", "", "", "", "", "", "", "", "", "", "", "", "", ""};

// Variables para calcular Kc para el cultivo:
float ValoresKc[3] = {0.00, 0.00, 0.00}; // Arreglo con los valores Kc ingresados para mostrarlos en pantalla. 
float Kc = 0.0; // Variable que almacena el valor de Kc
float KcIngresado[3] = {0.00, 0.00, 0.00}; // Arreglo que guarda el valor de Kc que ingresa el usuario.
int Counter = -2; // Determina el número de digitos, aunque deba ser cero, el arduino capta varias vences la señal de teclado (usualmente 2) por lo que toca compensar esas lecturas falsas 
int i = -1;

// Variables para fecha de siembra
int FSiembra[8] = {0,0,0,0,0,0,0,0};
int Position = -2;
int diaSiembra = 0;
int mesSiembra = 0;
int anoSiembra = 0;
int DiaJulianoS = 0; 
int ValorFS[3] = {0,0,0};

// Variablees para la duración de las etapas del cultivo.
int Duraciones[4] = {0,0,0,0};
int ValoresDuraciones[4] = {0,0,0,0};
int Posicion = -2;
int C = 0;
int Data[3] = {0,0,0};

// Variables para latitud
int Grad = 0;
int Min = 0;
float Seg = 0.0;
int I = 0;
int J = 0;
int K;
int numero = -1; 

// Variables para calcular et0
float et0 = 0.0; // Variable que almacena el valor de la ETo
float et0_dia = 0; // Variable para almacenar el valor diario de la ETo.
float Albedo = 0.23;
float DiaJuliano = 0.0;
float AltViento = 2;
float Altitud = 1001.1;
float Pi = 3.141592;
float etc = 0.0;
float etc_dia = 0.0;

// Valores que se deben calcular para resolver la ecuación de Pennman-Monteith
float Ubicacion = 0.0;
float DeclinacionSol = 0.0;
float DistRelativa = 0.0;
float AnguloSolar = 0.0;
float RadExtraterrestre = 0.0; // Ra
float RadSolarGlobal = 0.0; // Rso
float PresSatVapor = 0.0; // es
float PreParcial = 0.0; // ea
float RadOndaLarga = 0.0; // RnL 
float RadOndaCorta = 0.0; // Rns
float RadNeta = 0.0; // Rn
float PendCurvaPresion = 0.0; // delta
float PresionAtm = 0.0; // Pa
float CtePsicometrica = 0.0; // Y
float VelVientoAjustada = 0.0;

// Banderas para que lectura y guardado de los datos
bool Bandera1 = 0; // Variable que permite que se haga UNA SOLA LECTURA cada 10 segundos
bool Bandera2 = 0; // Variable que permite que se haga UN SOLO GUARDADO DE DATOS cada hora
bool Bandera3 = 0; // Variable que permite que se haga UN SOLO GUARDADO DE DATOS cada dia

//Acumuladores para calcular los promedios de cada variable
float Acum_Temp = 0;
float Acum_Hum = 0;
float Acum_Vel = 0;
float Acum_Irra = 0;

// Contador para calcular los promedios de cada variable
int Cantidad_Lecturas = 0;

// Contador para calcular los promedios diarios de cada variable
int Contador_dia = 0;
int Contador_dia_pira = 0;

// Variables para almacenar los promedios de cada variable
float Temp_prom = 0;
float Hum_prom = 0;
float Vel_prom = 0;
float Irra_prom = 0;

//Contadores de Nan
int NanTemperatura = 0;
int NanHumedad = 0;

// Variables para almacenar los promedios diarios de cada variable

float Temp_prom_dia= 0;
float Hum_prom_dia = 0;
float Vel_prom_dia = 0;
float Irra_prom_dia = 0;

// Variables para calcular el mín y el máx de Temperatura y Humedad

float Temp_min = 100;
float Temp_max = 0;
float Hum_min = 101;
float Hum_max = 0;

// Variables para calcular el mín y el máx diarios de Temperatura y Humedad

float Temp_min_dia = 100;
float Temp_max_dia = 0;
float Hum_min_dia = 101;
float Hum_max_dia = 0;


// Variables para la interfaz
int Pantalla = 1; // Varaible que hace el paso de "Bienvenida" a "Menú principal".
int Interfaz = 0; // Variable que permite cambiar entre "Ver datos", "Descargar" y "Fecha y hora".
int Acceder = 0; // Variable que permite entrar a cualquier interfaz sin que se dañe la lógica del teclado.
int Interfaz_Variables = 0; // Variable que permite cambiar entre las pantallas que se encuentran dentro de la interfaz "Ver datos".
int Interfaz_ET = 0; // Variable que permite cambiar entre las diferentes pantallas de ET (ingresar KC, Fecha siembra y duración etapas.
int AccederET = 0; // Variable para ingresar a la pantalla que permite al usuario Ingresar el Kc, ingrsar la fecha de siembra y la duración de las etapas del cultivo.
int ConfirmacionFH = 0; // Variable que permite ingresar a la pantalla de ver FH o para modificar FH.
int ConfirmacionLAT = 0; // Variable que permite ingresar a la pantalla de ver Latitud o modificar Latitud.
int Modificando = 0; // Bandera para evitar errores (Cruces entre funciones CambioPantalla y Arreglo) al momento que el usuario digite la nueva F y H.
int Modify = 0; // Bandera para evitar errores (Cruces entre los estados de Escoger kc del cultivo o ingresar el kc del cultivo).
int Modifying = 0; // Bandera para evitar errores (Cruces entre los estados Mostrar LAT o Modificar LAT).
int cantidad = 0; // Determina el número de elementos del arreglo que tiene la F y H.
int Dato = 0; // Es el número recibido por teclado convertido a entero.
int dia = 0; // Almacena el día, para modificar el RTC.
int mes = 0; // Almacena el mes, para modificar el RTC.
int ano = 0; // Almacena el año, para modificar el RTC.
int ano2 = 0; // Almacena el año, para mostrar en pantalla (2 dígitos).
int hora = 0; // Almacena la hora, para modificar el RTC.
int minuto = 0; // Almacena el minuto, para modificar el RTC.
int F_H[] = {0,0,0,0,0,0,0,0,0,0}; // Arreglo con todos los datos que se ingresan por teclado para modificar F_H.
int Seg_Ingresado[] = {0,0,0,0,0}; // Arreglo que guarda los segundos de la latitud ingresada.
int Lat_Ingresado[] = {0,0,0,0}; // Arreglo que guarda los datos de grados y minutos de la latitud ingresada.

// Variables para dia Juliano
int Dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};

// Variable para sensor final de carrera
int pulsador = 3;

// Variables para medir voltaje y corriente
float Bateria = 0.0;  

void setup() {
  Serial1.begin(9600);

// === PANTALLA ===
  lcd.init();

  lcd.createChar(0, Tem);
  lcd.createChar(1, Hum);
  lcd.createChar(2, PreA);
  lcd.createChar(3, VelV);
  lcd.createChar(4, Prec);
  lcd.createChar(5, RadS);
  lcd.createChar(6, EvaT);
  lcd.createChar(7, SimGrado);

// === RTC ===
  if (! rtc.begin()) {
      lcd.backlight();
      lcd.setCursor(2,0);
      lcd.print("NO HAY RTC");
  }     

// ===== BME280 =====
  if (!bme.begin(0x76)) {
      lcd.backlight();
      lcd.setCursor(2,0);
      lcd.print("NO BME");
  }
   
// === SD ===
  if (!SD.begin()) {
      lcd.backlight();
      lcd.setCursor(2,0);
      lcd.print("NO SD");
  }

// Creacion de archivos en la sd

  String Encabezado = "Fecha, Hora, Temp Prom (°C), Temp Min (°C), Temp Max (°C), Humedad Prom (%), Humedad Min (%), Humedad Max (%), Presion atmosférica (Pa), Velocidad del viento (m/s), Radiación solar (W/m2), Precipitación (mm), ET0 (mm), ETc (mm)";
  File dataFile = SD.open("Datos.txt", FILE_WRITE);
  if (dataFile){
    dataFile.println(Encabezado);
    dataFile.close();
  }
  File dataFile2 = SD.open("Resumen.txt", FILE_WRITE);
  if (dataFile2){
    dataFile2.println(Encabezado);
    dataFile2.close();
  }
  File dataFile3 = SD.open("Dia.txt", FILE_WRITE);
  if (dataFile3){
    dataFile3.println(Encabezado);
    dataFile3.close();
  }

// === Anemómetro ===
  // No necesita ninguna inicialización en el setup

// === Pluviómetro ===
  // No necesita ninguna inicialización en el setup

// === Piranómetro ===
  // No necesita ninguna inicialización en el setup

// === Teclado ===
  // No necesita ninguna inicialización en el setup
  
} // Cierre del setup


// ===== FUNCIONES =====

// === FUNCIONES PARA EL GUARDADO DE DATOS ===

void GuardarFecha(DateTime date){ // Esta función guarda la fecha y hora como strings en el arreglo Datos
  Datos[0] = String(date.day()) + "/" + String(date.month()) + "/" + String(date.year());
  Datos[1] = String(date.hour()) + ":" + String(date.minute()) + ":" + String(date.second());
}

void GuardarDatosEnArreglo(float dato, int posicion){ // Esta función guarda los datos recolectados como strings en el arreglo Datos 
  Datos[posicion] = String(dato);
}

void GuardarDatosEnSD(File dataFile, String datos[]){ // Esta función toma los datos del arreglo Datos y los pone en un archivo Datos.txt en la SD 
  if (dataFile){
    for ( int i = 0; i < 13; i++){
      dataFile.print(datos[i]);
      dataFile.print(", ");
    }
    dataFile.print(datos[13]);
    dataFile.println();
  }
}

// === FUNCIONES PARA LOS SENSORES ===

void FinalCarrera(int pulsador){ // Esta funcion determina cuando se pulsa el final de carrera y apaga el backlight de la pantalla.
  if (digitalRead(pulsador)==LOW){
    lcd.noBacklight();
  }
  else{
    lcd.backlight();
  }
}

void medirBME(){ // Esta función mide Temp y Hum, acumula sus valores y calcula el minimo y el máximo de estas dos variables
  float Humedad = bme.readHumidity();
  float Temperatura = bme.readTemperature()*0.8201 + 3.2555;

  // No se consideran las mediciones erroneas de tipo Nan
  if (isnan(Humedad)) {
    Humedad = 0;
    NanHumedad = NanHumedad + 1;
  }
  else{
    if (Humedad > Hum_max){
      Hum_max = Humedad;
    }
    if (Humedad < Hum_min){
      Hum_min = Humedad;
    }
    Acum_Hum = Acum_Hum + Humedad;
  }

  if (isnan(Temperatura)) {
    Temperatura = 0;
    NanTemperatura = NanTemperatura + 1;
  }
  else{
    if (Temperatura > Temp_max){
      Temp_max = Temperatura;
    }
    if (Temperatura < Temp_min){
      Temp_min = Temperatura;
    }
    Acum_Temp = Acum_Temp + Temperatura;
  }
}

void medirWS(){ // Esta función calcula el valor de la velocidad del viento y va acumulando los datos para posteriormente calcular el promedio
  sensorValueA = analogRead(sensorPinA);
  sensorVoltageA = sensorValueA * 0.004882814; // 0.004882814 = 5V/1024;
  if (sensorVoltageA <= 0.4){  WindSpeed = 0; }
  else { WindSpeed = (sensorVoltageA - 0.4)*32.4/(voltageMaxA - 0.4)*1.523 + 0.0515; }; // Por datasheet
  Acum_Vel = Acum_Vel + WindSpeed;
}

void medirIR(){ // Esta función calcula el valor de la irradiación solar y va acumulando los datos para posteriormente calcular el promedio
  sensorValueP = analogRead(sensorPinP);
  sensorVoltageP = sensorValueP * 0.004882814; //0.004882814 = 5V/1024;
  Irradiacion = sensorVoltageP *1.0026 / 0.0016667 + 4.3413; //Factor de conversión es 1.67 mV por cada W/m^2. 3V sería el tope: 1800W/m^2.
  Acum_Irra = Acum_Irra + Irradiacion;
}

void medirNA(float factor){// Esta función retorna el Nivel de Agua (NA) medida por el pluviometro 
  float value = digitalRead(pinpluvi);  
  if (estado1pluvi == 1){
    if (value == HIGH){
      estado2pluvi = 1; 
      estado1pluvi = 0;
    }
  }
  if (estado2pluvi == 1){
    if(value == LOW){ 
      contadorpluvi++; 
      estado1pluvi = 1; 
      estado2pluvi = 0; 
    }
  }
  Lluvia = contadorpluvi*factor*1.587 + 0.0021; 
}

// ===== FUNCIONES DE PANTALLA =====

void Bienvenida(int Pantalla){ // Esta es la primera función que se ejecuta al encender la estación, muestra el mensaje de bienvenida y, 3s después, muestra la pantalla del menú principal
    if (Pantalla){
      lcd.backlight();
      lcd.setCursor(2,0);
      lcd.print("BIENVENIDO A");
      lcd.setCursor(2,1);
      lcd.print("ARDUNAL  4.0");
      delay(3000); // Evitar lecturas mientras los sensores se estabilizan
      lcd.clear();
      Principales(Interfaz);
    } 
}

void CambioPantalla(char Tecla, DateTime date){ // Función que permite cambiar de una interfaz a otra. Hacia la derecha (->) y hacia la izquiera (<-).
    if (Acceder == 0){
      if (Tecla == '#'){
        Interfaz = (Interfaz + 1)%4;
        lcd.clear();
        Principales(Interfaz);
      }
      if (Tecla == '*'){
        Interfaz = (Interfaz - 1);
        if (Interfaz < 0){
          Interfaz = 3;
        }
        Interfaz = (Interfaz)%4;
        lcd.clear();
        Principales(Interfaz);
      }
    }

    if (Acceder == 1){
      if (Tecla == '#'){
        Interfaz_Variables = (Interfaz_Variables + 1)%7;
        Interfaz_ET = (Interfaz_ET + 1)%7;
        lcd.clear();
      }
      
      if (Tecla == '*'){
        Interfaz_Variables = (Interfaz_Variables - 1);
        Interfaz_ET = (Interfaz_ET - 1);
        if (Interfaz_Variables < 0){
          Interfaz_Variables = 6;
        }
        if (Interfaz_ET < 0){
          Interfaz_ET = 6;
        }
        Interfaz_Variables = (Interfaz_Variables)%7;
        Interfaz_ET = (Interfaz_ET)%7;
        lcd.clear();            
      }
      if (Modificando == 0){
        if (Tecla == '1'){
          ConfirmacionFH = 1;
        }
        if (Tecla == '2'){
          ConfirmacionFH = 2;
          Modificando = 1;
        }
      }
      if (Modifying == 0){
        if (Tecla == '1'){
          ConfirmacionLAT = 1;
          Modifying = 1;
        }
        if (Tecla == '2'){
          ConfirmacionLAT = 2;
          Modifying = 1;
        }
      }
      if (Tecla){
        Pantalla_Secundaria(Tecla, Interfaz_Variables, Interfaz_ET, Interfaz, AccederET, ConfirmacionFH, ConfirmacionLAT, date);
      }
    }
}

void Principales(int Interfaz){ // Función que permite cambiar entre las pantallas principales: "Ver datos", "Descargar", "Fecha y hora" y "Latitud".
  if (Interfaz == 0){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(6,0);
    lcd.print("MENU");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(2,1);
    lcd.print("1.VER DATOS");
  }
  if (Interfaz == 1){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(6,0);
    lcd.print("MENU");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(6,1);
    lcd.print("2.ET");
  }
  if (Interfaz == 2){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(6,0);
    lcd.print("MENU");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(0,1);
    lcd.print("3.MODIFICAR F&H");
  }
  if (Interfaz == 3){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(6,0);
    lcd.print("MENU");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(3,1);
    lcd.print("4.LATITUD");
  }
}      

void Variables(int Interfaz_Variables){ // Función que permite cambiar entre las pantallas de las variables sensadas.
  lcd.clear();
  if (Interfaz_Variables == 0){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(0));
    lcd.setCursor(4,0);
    lcd.print("TEMPERAT.");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(4,1);
    lcd.print(dht.readTemperature());
    lcd.setCursor(9,1);
    lcd.print(" C");
    }
  if (Interfaz_Variables == 1){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(3,0);
    lcd.write(byte(1));
    lcd.setCursor(5,0);
    lcd.print("HUMEDAD");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(4,1);
    lcd.print(dht.readHumidity());
    lcd.setCursor(9,1);
    lcd.print(" %");
    }
  if (Interfaz_Variables == 2){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(2));
    lcd.setCursor(4,0);
    lcd.print("PRESION A");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(3,1);
    lcd.print(bme.readPressure());
    lcd.setCursor(10,1);
    lcd.print(" Pa");
    }
  if (Interfaz_Variables == 3){
    medirWS();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(3));
    lcd.setCursor(4,0);
    lcd.print("V. VIENTO");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(3,1);
    lcd.print(WindSpeed);
    lcd.setCursor(8,1);
    lcd.print(" m/s");
    }
  if (Interfaz_Variables == 4){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(4));
    lcd.setCursor(4,0);
    lcd.print("PRECIPIT.");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(4,1);
    lcd.print(Lluvia);
    lcd.setCursor(9,1);
    lcd.print(" mm");
    }
  if (Interfaz_Variables == 5){
    medirIR();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(5));
    lcd.setCursor(4,0);
    lcd.print("RAD.SOLAR");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(2,1);
    lcd.print(Irradiacion);
    lcd.setCursor(8,1);
    lcd.print(" W/m2");
    }
  if (Interfaz_Variables == 6){
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("<");
    lcd.setCursor(2,0);
    lcd.write(byte(6));
    lcd.setCursor(6,0);
    lcd.print("ETo");
    lcd.setCursor(15,0);
    lcd.print(">");
    lcd.setCursor(6,1);
    lcd.print(et0);// En la pantalla se muestra la et0 de la hora que ya pasó.
    }              
}

void Acces(char Tecla, DateTime date){ // Función en donde se define la tecla "A" para acceder a cualquier opción y la "B" para retroceder. 
  if (Tecla == 'A'){
    if (Acceder = 1){
      AccederET++;
    }
    Acceder = 1;
    Pantalla_Secundaria(Tecla, Interfaz_Variables, Interfaz_ET, Interfaz, AccederET, ConfirmacionFH, ConfirmacionLAT, date);
  }
  if (Tecla == 'B'){
    Acceder = 0;
    ConfirmacionFH = 0;
    ConfirmacionLAT = 0;
    AccederET = 0;
    Modify = 0;
    Modifying = 0;
    Modificando = 0;
    cantidad = 0;
    numero = -1;
    Counter = -2;
    KcIngresado[0] = 0.00;
    KcIngresado[1] = 0.00;
    KcIngresado[2] = 0.00;
    Posicion = -2;
    Position = -2;
    C = 0;
    I = 0;
    i = -1;

    //Reset fecha-siembra ingresado
    FSiembra[0] = 0;
    FSiembra[1] = 0;
    FSiembra[2] = 0;
    FSiembra[3] = 0;
    FSiembra[4] = 0;
    FSiembra[5] = 0;
    FSiembra[6] = 0;
    FSiembra[7] = 0;
    
    lcd.clear();
    Principales(Interfaz);
  }
}

void Evapotranspiracion(int AccederET, int Interfaz_ET, char Tecla){ // Función que muestra las pantallas para que ingrese por teclado los valores de Kc, las duraciones y la fecha de siembra del cultivo para calcular la ETc.
  if (AccederET == 1){
    lcd.clear();
    if (Interfaz_ET == 0){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("INGRESAR");
      lcd.setCursor(3,1);
      lcd.print("VALORES Kc");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 1){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("INGRESAR");
      lcd.setCursor(0,1);
      lcd.print("FECHA DE SIEMBRA");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 2){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("INGRESAR");
      lcd.setCursor(3,1);
      lcd.print("DURACIONES");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 3){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("MOSTRAR");
      lcd.setCursor(3,1);
      lcd.print("VALORES KC");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 4){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("MOSTRAR");
      lcd.setCursor(0,1);
      lcd.print("FECHA DE SIEMBRA");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 5){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("MOSTRAR");
      lcd.setCursor(3,1);
      lcd.print("DURACIONES");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
    if (Interfaz_ET == 6){
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("<");
      lcd.setCursor(4,0);
      lcd.print("MOSTRAR");
      lcd.setCursor(6,1);
      lcd.print("ET");
      lcd.setCursor(15,0);
      lcd.print(">");
      }
  }
  else {
    lcd.clear();
    if (Interfaz_ET == 0){
      IngresarKc(Tecla);
      }
    else if (Interfaz_ET == 1){
      FechaSiembra(Tecla);
      }
    else if (Interfaz_ET == 2){
      DuracionEtapas(Tecla);
      }
    else if (Interfaz_ET == 3){
      MostrarKc();
      }
    else if (Interfaz_ET == 4){
      MostrarFS();
      }
    else if (Interfaz_ET == 5){
      MostrarDuraciones();
      }
    else if (Interfaz_ET == 6){
      ETc();
      mostrar_ETc();
      }
  }
}

void Fecha_Hora(char Tecla, int ConfirmacionFH, DateTime date){ // Función que permite visualizar la FyH o modificarlas.
  if (ConfirmacionFH == 0){
    lcd.clear();
    lcd.backlight();  
    lcd.setCursor(1,0);
    lcd.print("1. MOSTRAR F_H");
    lcd.setCursor(0,1);
    lcd.print("2. MODIFICAR F_H");    
  }
  if (ConfirmacionFH == 1){
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("FECHA ");
    lcd.setCursor(6,0);
    lcd.print(date.day());
    lcd.setCursor(8,0);
    lcd.print("/");
    lcd.setCursor(9,0);
    lcd.print(date.month());
    lcd.setCursor(11,0);
    lcd.print("/");
    lcd.setCursor(12,0);
    lcd.print(date.year());
    lcd.setCursor(1,1);
    lcd.print("HORA   ");
    lcd.setCursor(9,1);
    lcd.print(date.hour());
    lcd.setCursor(11,1);
    lcd.print(":");
    lcd.setCursor(12,1);
    lcd.print(date.minute());    
  }
  if (ConfirmacionFH == 2){
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("DD/MM/AA");
    lcd.setCursor(11,0);
    lcd.print("HH:MM");
    lcd.setCursor(2,1);
    lcd.print("/");
    lcd.setCursor(5,1);
    lcd.print("/");
    lcd.setCursor(13,1);
    lcd.print(":");
    Arreglo(Tecla, date);
  }
}

void Latitud(char Tecla, int ConfirmacionLAT){ // Función que permite visualizar la Latitud o modificarla.
  if (ConfirmacionLAT == 0){
    lcd.clear();
    lcd.backlight();  
    lcd.setCursor(1,0);
    lcd.print("1. MOSTRAR LAT");
    lcd.setCursor(0,1);
    lcd.print("2. MODIFICAR LAT");    
  }
  if (ConfirmacionLAT == 1){
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(4,0);
    lcd.print("LATITUD");
    lcd.setCursor(2,1);
    lcd.print(Grad);
    lcd.setCursor(4,1);
    lcd.write(byte(7));
    lcd.setCursor(5,1);
    lcd.print(Min);
    lcd.setCursor(7,1);
    lcd.print("'");
    lcd.setCursor(8,1);
    lcd.print(Seg);
    lcd.setCursor(13,1);
    lcd.print("\"");
  }
  if (ConfirmacionLAT == 2){
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(2,0);
    lcd.print("GRA/MIN/SEG");
    lcd.setCursor(3,1);
    lcd.print("/");
    lcd.setCursor(6,1);
    lcd.print("/");
    IngresarLat(Tecla);
  }
}

void Plantilla(){
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(2,0);
    lcd.print("GRA/MIN/SEG");
    lcd.setCursor(2,1);
    lcd.print(Grad);
    lcd.setCursor(4,1);
    lcd.write(byte(7));
    lcd.setCursor(5,1);
    lcd.print(Min);
    lcd.setCursor(7,1);
    lcd.print("'");
    lcd.setCursor(8,1);
    lcd.print(Seg);
    lcd.setCursor(13,1);
    lcd.print("\"");
}

void IngresarKc(char Tecla){ // Función que permite ingresar los valores inicial, medio y avanzado del Kc.
    if (Tecla){
      if (Counter > -1){
        KcIngresado[Counter/3] = KcIngresado[Counter/3] + (String(Tecla).toInt())*(pow(10,-(Counter%3))); 
      }
      Counter ++;
      if (Counter > 8){
          Counter = -2;
          ValoresKc[0] = KcIngresado[0];
          ValoresKc[1] = KcIngresado[1];
          ValoresKc[2] = KcIngresado[2];
          KcIngresado[0] = 0.00;
          KcIngresado[1] = 0.00;
          KcIngresado[2] = 0.00;
          lcd.clear();
          lcd.setCursor(4,0);
          lcd.print("GUARDADO");
          lcd.setCursor(4,1);
          lcd.print("EXITOSO!");
      }
      else{
        lcd.clear();
        lcd.backlight();
        lcd.setCursor(0,0);
        lcd.print(" INI  MED  AVA ");
        lcd.setCursor(1,1);
        lcd.print(KcIngresado[0]);
        lcd.setCursor(6,1);
        lcd.print(KcIngresado[1]);
        lcd.setCursor(11,1);
        lcd.print(KcIngresado[2]);
      }
    }
}

void FechaSiembra(char Tecla){ // Función recibe la fecha de siembra ingresada por el usuario.
  if (Tecla){
    FSiembra[Position] = (String(Tecla).toInt());
    Position++;
    diaSiembra = FSiembra[0]*10 + FSiembra[1];
    mesSiembra = FSiembra[2]*10 + FSiembra[3];
    anoSiembra = FSiembra[4]*1000 + FSiembra[5]*100 + FSiembra[6]*10 + FSiembra[7];
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(3,0);
    lcd.print("DD/MM/AAAA");
    // Aqui se muestra lo digitado por el usuario.
    lcd.setCursor(3,1);
    lcd.print(diaSiembra);
    lcd.setCursor(5,1);
    lcd.print("/");
    lcd.setCursor(6,1);
    lcd.print(mesSiembra);
    lcd.setCursor(8,1);
    lcd.print("/");
    lcd.setCursor(9,1);
    lcd.print(anoSiembra);
    if (Position > 7){
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("GUARDADO");
      lcd.setCursor(4,1);
      lcd.print("EXITOSO!");
      Position = -2;
      ValorFS[0] = diaSiembra;
      ValorFS[1] = mesSiembra;
      ValorFS[2] = anoSiembra;
      FSiembra[0] = 0;
      FSiembra[1] = 0;
      FSiembra[2] = 0;
      FSiembra[3] = 0;
      FSiembra[4] = 0;
      FSiembra[5] = 0;
      FSiembra[6] = 0;
      FSiembra[7] = 0;
    }
  }
}

void DuracionEtapas(char Tecla){ // Función recibe las dureciones de las estapas del cultivo, esto lo ingresa por el usuario.
  if (Tecla){
    if (C < 4){
      int b = 0;
      Data[Posicion] = (String(Tecla).toInt());
      for (int k = 0; k < Posicion + 1; k++){
        int pot = 1;
        for (int l = 0; l < Posicion-k; l++){
          pot = pot*10;
        }
        b = b + Data[k]*pot;
      }
      Duraciones[C] = b;
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("INI DES MED AVA");
      lcd.setCursor(0,1);
      lcd.print(Duraciones[0]);
      lcd.setCursor(4,1);
      lcd.print(Duraciones[1]);
      lcd.setCursor(8,1);
      lcd.print(Duraciones[2]);
      lcd.setCursor(12,1);
      lcd.print(Duraciones[3]);
      Posicion += 1;
      if (Posicion > 2 || Tecla == 'C'){
        Posicion = 0;
        C++;
      }
    }
    if (C == 4){
      ValoresDuraciones[0] = Duraciones[0];
      ValoresDuraciones[1] = Duraciones[1];
      ValoresDuraciones[2] = Duraciones[2];
      ValoresDuraciones[3] = Duraciones[3];
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("GUARDADO");
      lcd.setCursor(4,1);
      lcd.print("EXITOSO!");
    }
  }
}

void MostrarKc(){ // Función que muestra los valores de Kc almacenados.
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print(" INI  MED  AVA ");
    lcd.setCursor(1,1);
    lcd.print(ValoresKc[0]);
    lcd.setCursor(6,1);
    lcd.print(ValoresKc[1]);
    lcd.setCursor(11,1);
    lcd.print(ValoresKc[2]);
}

void MostrarFS(){ // Función que muestra la fecha de siembra del cultivo.
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(3,0);
    lcd.print("DD/MM/AAAA");
    // Aqui se muestra lo digitado por el usuario.
    lcd.setCursor(3,1);
    lcd.print(ValorFS[0]);
    lcd.setCursor(5,1);
    lcd.print("/");
    lcd.setCursor(6,1);
    lcd.print(ValorFS[1]);
    lcd.setCursor(8,1);
    lcd.print("/");
    lcd.setCursor(9,1);
    lcd.print(ValorFS[2]);
}

void MostrarDuraciones(){ // Función que muestra las duraciones de las etapas del cultivo.
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("INI DES MED AVA");
    lcd.setCursor(0,1);
    lcd.print(ValoresDuraciones[0]);
    lcd.setCursor(4,1);
    lcd.print(ValoresDuraciones[1]);
    lcd.setCursor(8,1);
    lcd.print(ValoresDuraciones[2]);
    lcd.setCursor(12,1);
    lcd.print(ValoresDuraciones[3]);
}

void ETc(){ // Función que actualiza el valor de Kc y realiza el cálculo de la ETc.
  Kc = 0.0;
  DiaJ(mes, ano2);
  DiaJS(ValorFS[1], ValorFS[2]); // Dia Juliano correspondiente a la fecha de siembra
  int CantidadDias = int(DiaJuliano) - DiaJulianoS + 365*(ano - anoSiembra); // Esta variable tiene la cantidad de días desde que se sembró el cultivo
  if (CantidadDias <= ValoresDuraciones[0]){
    Kc = float(ValoresKc[0]);
  }
  else if (CantidadDias <= ValoresDuraciones[0] + ValoresDuraciones[1]){
    float factor = (float(ValoresKc[1]) - float(ValoresKc[0]))/(float(ValoresDuraciones[1]));
    Kc = float(ValoresKc[0]) + factor*float(CantidadDias - ValoresDuraciones[0]);
  }
  else if (CantidadDias <=  ValoresDuraciones[0] + ValoresDuraciones[1] + ValoresDuraciones[2]){
    Kc = ValoresKc[1];
  }
  else {
    float factor = (float(ValoresKc[2]) - float(ValoresKc[1]))/float(ValoresDuraciones[3]);
    Kc = float(ValoresKc[1]) + factor*float(CantidadDias - (ValoresDuraciones[0] + ValoresDuraciones[1] + ValoresDuraciones[2]));
  }
  etc = Kc * et0;
} 

void mostrar_ETc(){ // Función que la evapotranspiracion del cultivo.
  float et = Kc*et0;
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Kc = ");
  lcd.setCursor(9,0);
  lcd.print(Kc);
  lcd.setCursor(2,1);
  lcd.print("ETc = ");
  lcd.setCursor(8,1); 
  lcd.print(et);
}

void ETo(){ // Función que realiza el cálculo de la ETo
  Ubicacion = float(Grad) + float(Min)/60 + Seg/3600; // Grados
  DeclinacionSol = 0.4093*sin((2*Pi)*(DiaJuliano/365)-1.405);// radianes
  DistRelativa = 1 + 0.033*cos((2*Pi)*(DiaJuliano/365)); // radianes
  AnguloSolar = acos(-1*tan(Ubicacion*Pi/180)*tan(DeclinacionSol)); //radianes
  RadExtraterrestre = 37.6*DistRelativa*(AnguloSolar*sin(Ubicacion*Pi/180)*sin(DeclinacionSol) + cos(Ubicacion*Pi/180)*cos(DeclinacionSol)*sin(AnguloSolar));// Mj/m2dia
  RadSolarGlobal = RadExtraterrestre*(0.75 + 2*(12.5 + Altitud)/100000); //MJ/m2dia
  PresSatVapor = (0.6108*exp((Temp_min*17.27)/(237.3 + Temp_min)) + 0.6108*exp((Temp_max*17.27)/(237.3 + Temp_max)))/2; // KPa
  PreParcial = (0.6108*exp((Temp_min*17.27)/(237.3 + Temp_min))*(Hum_max/100) + 0.6108*exp((Temp_max*17.27)/(237.3 + Temp_max))*(Hum_min/100))/2; // KPa
  RadOndaLarga = (4.903/1000000000) * (((pow(Temp_min + 273.3,4) + pow(Temp_max + 273.3,4))/2) * (0.34 - 0.14*pow(PreParcial,0.5)) * (1.35*(3.6*Irradiacion*0.001/RadSolarGlobal) - 0.35));// MJ/m2dia
  RadOndaCorta = (1-Albedo)*3.6*Irradiacion*0.001; // MJ/m2dia
  RadNeta = RadOndaCorta - RadOndaLarga; // MJ/m2dia
  PendCurvaPresion = (4098.1 * (0.6108*exp((17.27*Temp_prom)/(Temp_prom + 237.3)))) / pow((Temp_prom + 237.3), 2); //KPa°C
  PresionAtm = 101.3 * pow((293.1 - 0.0065*Altitud)/293.0, 5.26); // KPa
  CtePsicometrica = 0.000665*PresionAtm;
  VelVientoAjustada = WindSpeed * (4.87/log(67.8*AltViento - 5.42));
  et0 = (0.408*PendCurvaPresion*(RadNeta) + CtePsicometrica*(900/(Temp_prom + 273.3))*VelVientoAjustada*(PresSatVapor - PreParcial)) / (PendCurvaPresion + CtePsicometrica*(1 + 0.34*VelVientoAjustada));
}

void Arreglo(char Tecla, DateTime date){ // Función que modifica el arreglo de FyH según los valores que digite el usuario. Al final, muestra la nueva FyH y actualiza el módulo RTC con dichos valores.
  if (Tecla){
    if (cantidad > 0){
      Dato = String(Tecla).toInt();
      F_H[cantidad-1] = Dato;
      ModificarF_H();
      FechaPantalla();
    }
    cantidad += 1;
  }
  if (cantidad == 11){
    rtc.adjust(DateTime(ano, mes, dia, hora, minuto, 0));
    DateTime now = rtc.now();
    ConfirmacionFH = 1;
    Fecha_Hora(Tecla, ConfirmacionFH, now);
    cantidad = 0;
    Modificando = 0;
    DiaJ(mes, ano2);
    F_H[0] = 0;
    F_H[1] = 0;
    F_H[2] = 0;
    F_H[3] = 0;
    F_H[4] = 0;
    F_H[5] = 0;
    F_H[6] = 0;
    F_H[7] = 0;
    F_H[8] = 0;
    F_H[9] = 0;
  }
}

void ModificarF_H(){ // Función que pone los valores de FyH ingresados por el usuario en el formato adecuado para que al RTC se actualice.
  dia = (F_H[0]*10)+(F_H[1]);
  mes = (F_H[2]*10)+(F_H[3]);
  ano = 2000 + (F_H[4]*10)+(F_H[5]);
  ano2 = (F_H[4]*10)+(F_H[5]);
  hora = (F_H[6]*10)+(F_H[7]);
  minuto = (F_H[8]*10)+(F_H[9]);
}

void FechaPantalla(){ // Función que muestra la FyH en la pantalla.
  // Esto es fijo
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("DD/MM/AA");
  lcd.setCursor(11,0);
  lcd.print("HH:MM");
  lcd.setCursor(2,1);
  lcd.print("/");
  lcd.setCursor(5,1);
  lcd.print("/");
  lcd.setCursor(13,1);
  lcd.print(":");
  // Esto es lo que varía, según lo que digite el usuario
  lcd.setCursor(0,1);
  lcd.print(dia);
  lcd.setCursor(3,1);
  lcd.print(mes);
  lcd.setCursor(6,1);
  lcd.print(ano2);
  lcd.setCursor(11,1);
  lcd.print(hora);
  lcd.setCursor(14,1);
  lcd.print(minuto);
}

void DiaJ(int Mes, int anio2){ // Funcion que calcula el dia juliano de la fecha actual.
  DiaJuliano = 0.0;
  int b;
  for ( b = 0 ; b < Mes - 1 ; b++){
     DiaJuliano = DiaJuliano + Dias[b];
  }
  DiaJuliano = DiaJuliano + dia;
  if ((anio2 % 4 == 0) && (DiaJuliano > 59)) {
    DiaJuliano = DiaJuliano + 1;
  }
}

void DiaJS(int Mes, int anio2){ // Funcion que calcula el dia juliano de la fecha de siembra
  DiaJulianoS = 0;
  int b;
  for ( b = 0 ; b < Mes - 1; b++){
     DiaJulianoS = DiaJulianoS + Dias[b];
  }
  DiaJulianoS = DiaJulianoS + ValorFS[0]; // ValorFS[0] se refiere a los dias del mes de siembra
  if ((anio2 % 4 == 0) && (DiaJulianoS > 59)) {
    DiaJulianoS = DiaJulianoS + 1;
  }
}

void Pantalla_Secundaria(char Tecla, int Interfaz_Variables, int Interfaz_ET, int Interfaz, int AccederET, int ConfirmacionFH, int ConfirmacionLAT, DateTime date){ // Función que permite hacer la transición de una pantalla principal a una secundaria.
  if (Interfaz == 0){
    Variables(Interfaz_Variables);
  }
  else if (Interfaz == 1){
    //Serial.println("Pantalla Sec");
    Evapotranspiracion(AccederET, Interfaz_ET, Tecla);
  }
  else if (Interfaz == 2){
    Fecha_Hora(Tecla, ConfirmacionFH, date);
  }
  else{
    Latitud(Tecla, ConfirmacionLAT);
  }
}

void IngresarSeg(char Tecla){ // Función que permite que el valor de segundos (en la latitud) ingresado por el usuario se convierta en un valor flotante para realizar el cálculo de Pennman-Monteith.
    if (Tecla == 'D'){
       I --;
       J = I;
    }
    else if (I > -1 && I < 4) {
      Seg = 0.0;
      Seg_Ingresado[I] = String(Tecla).toInt();
      if (J == 0){
        for (K=I; K>-1; K--){
          Seg = Seg_Ingresado[K]*(pow(10,I - K)) + Seg;            
        }
      }
      else if (J != 0){
        for (K=J; K>J-I-1; K--){
          Seg = Seg_Ingresado[J-K]*(pow(10,K)) + Seg;
        }
      }
    }
    else if ((I > 3) || (Tecla == 'A')){
      numero = -1;
      ConfirmacionLAT = 1;
      Modifying = 0;
      I = -1;
      J = 0;               
    }
  I++;
}

void IngresarLat(char Tecla){  // Función que convierte la latitud ingresada por el usuario en valores utilizables por el código para el cálculo de Pennman-Monteith.
  if (Tecla){
    if ((numero > -1) && (numero < 4)){
      Lat_Ingresado[numero] = String(Tecla).toInt();
    }
    else if (numero > 3){
      IngresarSeg(Tecla);
    }
    numero ++;
    Grad = Lat_Ingresado[0]*10 + Lat_Ingresado[1];
    Min = Lat_Ingresado[2]*10 + Lat_Ingresado[3];
    Plantilla();
  }
}

// === FUNCIONES PARA TRANSMISIÓN ===
void Leer(){
  char Lectura;
  File dataFileSend = SD.open("Datos.txt", FILE_READ);
  for (int i = 0; i < dataFileSend.size(); i++){
    Lectura = dataFileSend.read();
    Serial1.print(Lectura);
  }
  dataFileSend.close();
}

void Leer2(){
  char Lectura;
  File dataFileSend2 = SD.open("Dia.txt", FILE_READ);
  for (int i = 0; i < dataFileSend2.size(); i++){
    Lectura = dataFileSend2.read();
    Serial1.print(Lectura);
  }
  dataFileSend2.close();
}

void Borrar(){
  if (SD.exists("Datos.txt")) {
    SD.remove("Datos.txt");
  }  
  if (SD.exists("Dia.txt")) {
    SD.remove("Dia.txt");
  }  
}

void Enviar() {
  String data;
  if (Serial1.available()) {
    //Serial1.println("¡Hola desde el puerto Serial1!");
    data = Serial1.readString();
    //Serial.println(data);
    if (data == "Horarios\r\n"){
      Leer();
      }
    else if (data == "Diarios\r\n"){
      Leer2();
    }
    else if (data == "Borrar\r\n"){
      Borrar();
      Serial1.println("Archivos Borrados");
      Archivos();
    }
    else{
      Serial1.println("Comando no válido");
    }
  }
}

void Archivos(){ // Funcion que crea los archivos para almacenar los datos.
  String Encabezado = "Fecha, Hora, Temp Prom (°C), Temp Min (°C), Temp Max (°C), Humedad Prom (%), Humedad Min (%), Humedad Max (%), Presion atmosférica (Pa), Velocidad del viento (m/s), Radiación solar (W/m2), Precipitación (mm), Evapotranspiración (mm)";
  File dataFile = SD.open("Datos.txt", FILE_WRITE);
  if (dataFile){
    dataFile.println(Encabezado);
    dataFile.close();
    Serial1.println("Datos.txt creado");
  }
  File dataFile2 = SD.open("Resumen.txt", FILE_WRITE);
  if (dataFile2){
    dataFile2.println(Encabezado);
    dataFile2.close();
    Serial1.println("Resumen.txt creado");
  }
  File dataFile3 = SD.open("Dia.txt", FILE_WRITE);
  if (dataFile3){
    dataFile3.println(Encabezado);
    dataFile3.close();
    Serial1.println("Dia.txt creado");
  }
}

void VoltageRead(){ // Función que lee el voltaje de la bateria
  Bateria = (analogRead(A1)*5.0/1023.0)/0.33602151; // 0.33602151 es el factor de conversion del divisor de tensión.
}

void loop() {    

  // ===== RTC =====
  DateTime now = rtc.now();

  // ===== PANTALLA =====
  Bienvenida(Pantalla);
  Pantalla = 0;
  char Tecla = Teclado.getKey();
  Acces(Tecla, now);
  CambioPantalla(Tecla, now);
  FinalCarrera(pulsador);
  Enviar();


  // ===== MEDICIONES =====
    // === Lectura y guardado de datos del Pluviómetro ===
  medirNA(mll); 
  GuardarDatosEnArreglo(Lluvia,11);
  
  if ((now.second()%10 == 0) && Bandera1 == false){
      
    // ===== DHT11 =====
      medirBME(); 
      GuardarDatosEnArreglo(Temp_min,3);
      GuardarDatosEnArreglo(Temp_max,4);
      GuardarDatosEnArreglo(Hum_min,6);
      GuardarDatosEnArreglo(Hum_max,7);
           
    // ===== Anemómetro =====
      medirWS();
    
    // ===== Piranómetro =====
      medirIR();
   
    Cantidad_Lecturas += 1;
    Bandera1 = true;
    VoltageRead();
    
    else if((now.second()%10 != 0) && Bandera1 == true){
      Bandera1 = false;  
    }

// ===== ALMACENAMIENTO DE DATOS EN SD =====

    if (now.minute() == 0 && now.second() == 0 && (Bandera2 == false)){  // Para guardar cada hora
      ETc();
      GuardarFecha(now);

      // se captura maximos y minimos de Temperatura y humedad para el dia.
      // De entre los valores horarios se buscan los max y min.
      if (Temp_max > Temp_max_dia){
        Temp_max_dia = Temp_max;
      }
      if (Temp_min < Temp_min_dia){
        Temp_min_dia = Temp_min;
      }
      if (Hum_min < Hum_min_dia){
        Hum_min_dia = Hum_min;
      }
      if (Hum_max > Hum_max_dia){
        Hum_max_dia = Hum_max;
      }       

      // Calculo de promedios
      float Presion = bme.readPressure();
      Temp_prom = Acum_Temp/(Cantidad_Lecturas - NanTemperatura);
      Temp_prom_dia = Temp_prom_dia + Temp_prom;
      Hum_prom = Acum_Hum/(Cantidad_Lecturas - NanHumedad);
      Hum_prom_dia = Hum_prom_dia + Hum_prom;
      Vel_prom = Acum_Vel/Cantidad_Lecturas;
      Vel_prom_dia = Vel_prom_dia + Vel_prom;
      Irra_prom = Acum_Irra/Cantidad_Lecturas;

      // Se guardan los datos de radiacion entre las 6:00 y las 18:00
      if(now.hour() > 5 && now.hour() < 18){
        Irra_prom_dia = Irra_prom_dia + Irra_prom;
        Contador_dia_pira += 1;
      }

      Lluvia_dia = Lluvia_dia + Lluvia;
      ETo(); // En la pantalla se muestra la et0 de la hora que ya pasó.
      et0_dia = et0_dia + et0;
      etc_dia = etc_dia + etc;

      // Se estructuran los datos antes de guardarlos en la SD
      GuardarDatosEnArreglo(Presion,8);
      GuardarDatosEnArreglo(Temp_prom,2);
      GuardarDatosEnArreglo(Hum_prom,5);
      GuardarDatosEnArreglo(Vel_prom,9);
      GuardarDatosEnArreglo(Irra_prom,10);
      GuardarDatosEnArreglo(et0,12);
      GuardarDatosEnArreglo(etc,13);

      // Se guardan los datos en la memoria SD
      File dataFile = SD.open("Datos.txt", FILE_WRITE);
      GuardarDatosEnSD(dataFile, Datos);
      dataFile.close();

      // Cierre del if para evitar que se repitan de nuevo las instrucciones en el mismo segundo
      Bandera2 = true;
      Contador_dia += 1;
    }
    
    else if(now.minute() == 0 && now.second() != 0 && (Bandera2 == true)){ // Cada hora
      // Reset de las variables y banderas.
      Bandera2 = false;
      Acum_Temp = 0;
      Acum_Hum = 0;
      Acum_Vel = 0;
      Acum_Irra = 0;
      Hum_max = 0;
      Hum_min = 101;
      Temp_max = 0;
      Temp_min = 100;
      contadorpluvi = 0;
      Cantidad_Lecturas = 0;
      NanHumedad = 0;
      NanTemperatura = 0;
    }
    
    if (now.hour() == 0 && now.minute() == 0 && now.second() == 0 && (Bandera3 == false)){  // Para guardar cada dia
      float Presion = bme.readPressure();

      // Se calculan los datos a la vez que son promediados.
      GuardarDatosEnArreglo(Temp_prom_dia/Contador_dia,2);
      GuardarDatosEnArreglo(Temp_min_dia,3);
      GuardarDatosEnArreglo(Temp_max_dia,4);
      GuardarDatosEnArreglo(Hum_prom_dia/Contador_dia,5);
      GuardarDatosEnArreglo(Hum_min_dia,6);
      GuardarDatosEnArreglo(Hum_max_dia,7);
      GuardarDatosEnArreglo(Presion,8);
      GuardarDatosEnArreglo(Vel_prom_dia/Contador_dia,9);
      GuardarDatosEnArreglo(Irra_prom_dia/Contador_dia_pira,10);
      GuardarDatosEnArreglo(Lluvia_dia,11); 
      GuardarDatosEnArreglo(et0_dia,12);
      GuardarDatosEnArreglo(etc_dia,13);

      // Se guardan los datos en la SD
      File dataFile2 = SD.open("Dia.txt", FILE_WRITE);
      GuardarDatosEnSD(dataFile3, Datos);
      dataFile2.close();

      // Cierre del if para evitar que se repitan de nuevo las instrucciones en el mismo segundo
      Bandera3 = true;
    }
    else if(now.hour() == 0 && now.minute() == 0 && now.second() != 0 && (Bandera3 == true)){ // Cada dia
      // Reset de las variables y banderas.
      Bandera3 = false;
      Hum_prom_dia = 0;
      Hum_max_dia = 0;
      Hum_min_dia = 101;
      Temp_prom_dia = 0;
      Temp_max_dia = 0;
      Temp_min_dia = 100;
      Vel_prom_dia = 0;
      Irra_prom_dia = 0;
      Lluvia_dia = 0;
      et0_dia= 0.0;
      etc_dia = 0.0;
      Contador_dia = 0;
      Contador_dia_pira = 0;
    }
} // Cierre del Loop
