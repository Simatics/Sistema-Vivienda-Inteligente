#include <EEPROM.h>
#include <Wire.h>
#include <Keypad.h>
#include <Adafruit_LiquidCrystal.h>
#include <Servo.h>

/*
    Alfaro Laínez Pedro Ernesto a119002
    Aquino Escobar Oscar René ae18021
    Martínez Escalante Emerson Adalberto me12007
    Rivera Deleón Allison Nicole rd20019
    Sosa García Elsbeth Ellen sg92004
*/

//=================================================
//                  MAESTRO
//=================================================

#define EEPROM_INICIALIZADA 100

//=================================================
// LCD
//=================================================

Adafruit_LiquidCrystal lcd(0);

const byte FILAS = 4;
const byte COLUMNAS = 4;
int valorTemp = -999;
int itemAnterior = -999;

const char* dispositivoAlerta = "";
const char* msjAlerta = "";

bool estadoAlerta = false;
bool alertaMostrada = false;
int tempAnterior = -999;
int nivelAnterior = -999;
int humedadAnterior = -999;
unsigned long tiempoAlerta = 0;

Servo servo;

//=================================================
// VARIABLES PARA LA SIRENA
//=================================================
unsigned long tiempoInicioSirena = 0; 
unsigned long tiempoTonoSirena = 0;   
bool sirenaActiva = false;            
bool tonoAlto = false;

//=================================================
// TECLADO
//=================================================

char teclas[FILAS][COLUMNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pinesFilas[FILAS] = {A0,A1,A2,A3};
byte pinesColumnas[COLUMNAS] = {5,4,3,2};

Keypad teclado = Keypad(
  makeKeymap(teclas),
  pinesFilas,
  pinesColumnas,
  FILAS,
  COLUMNAS
);

//=================================================
// DATOS RECIBIDOS POR I2C
//=================================================

struct DatosSensores {
  int temperatura;
  int luz;
  int movimiento;
  int humedad;
  int distancia;
};

DatosSensores datos;

//=================================================
// PARAMETROS EEPROM
//=================================================

struct Parametro {
  const char* nombre;
  byte direccionEEPROM;
};

//=================================================
// PARAMETROS ESTADO I2C
//=================================================

struct ParametroEstado {
  const char* nombre;
  int* valorActual;
};

//=================================================
// MODULO
//=================================================

struct Modulo {
  const char* nombre;
  Parametro* parametros;
  byte cantidadParametros;
};

//=================================================
// PARAMETROS VENTILACION
//=================================================

Parametro ventilacion[] = {
  {"Temp Max", 0},
  {"Temp OFF", 1},
  {"Vel Min", 2},
  {"Vel Max", 3},
};

//=================================================
// PARAMETROS CISTERNA
//=================================================

Parametro cisterna[] = {
  {"Nivel Min", 4},
  {"Nivel Max", 5}
};

//=================================================
// PARAMETROS RIEGO
//=================================================

Parametro riego[] = {
  {"Hum Min", 6},
  {"Hum Normal", 7},
  {"Servo", 8}
};

//=================================================
// PARAMETROS LUCES
//=================================================

Parametro lucesParams[] = {
  {"Sensibilidad", 9},
  {"Modo", 10}
};

//=================================================
// PARAMETROS SEGURIDAD
//=================================================

Parametro seguridad[] = {
  {"Activar", 11},
  {"Sensibilidad", 12},
  {"Tiempo", 13}
};

//=================================================
// ESTADOS I2C
//=================================================

ParametroEstado estados[] = {
  {"Temperatura", &datos.temperatura},
  {"Distancia", &datos.distancia},
  {"Humedad", &datos.humedad},
  {"Luz", &datos.luz},
  {"Movimiento", &datos.movimiento}
};

//=================================================
// MENUS
//=================================================

Modulo menus[] = {
  {"Ventilacion", ventilacion, 4},
  {"Cisterna", cisterna, 2},
  {"Riego", riego, 3},
  {"Luces", lucesParams, 2},
  {"Seguridad", seguridad, 3}
};

//=================================
// VALORES POR DEFECTO
//=================================
byte valoresDefault[] = {
  //Ventilador
  30,  // Temp ON P0
  20,  // Temp OFF P1
  150, // Vel Min P2
  255, // Vel Max P3
  
  //Cisterna
  180, // Nivel Min P4
  20,  // Nivel Max P5
  
  //Riego
  30,  // Hum Min P6
  50,  // Hum Normal P7
  90,  // Servo P8
  
  //Luces
  70,  //Sensibilidad P9 
  1,   // Modo P10
  
  //Seguridad perimetral
  1,   // Activar seguridad P11
  1,   // Sirena P12
  10   // Tiempo P13
};

//===========================
// VARIABLES PARA EL MENU
//===========================

enum Pantalla {
  MENU_PRINCIPAL,
  SUBMENU
};

Pantalla estado = MENU_PRINCIPAL;

int itemMenu = 0;
int itemSubMenu = 0;
char tecla;
bool menuEstado = false;
const char* extraMenu = "Estado";
bool modoEdicion = false;
int valorEditando = 0;

//===========================
// MOSTRAR MENU PRINCIPAL
//===========================

void mostrarMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(">");

  if(itemMenu == 5) {
    lcd.print(extraMenu);
  } else {
    lcd.print(menus[itemMenu].nombre);
  }

  if(itemMenu + 1 < 6) {
    lcd.setCursor(0,1);
    lcd.print(" ");
    if(itemMenu + 1 == 5) {
      lcd.print(extraMenu);
    } else {
      lcd.print(menus[itemMenu + 1].nombre);
    }
  }
}

//====================
// MOSTRAR SUBMENU
//====================
void mostrarSubMenu() {
  if(menuEstado) {
    ParametroEstado p = estados[itemSubMenu];

    if((itemAnterior != itemSubMenu) || (valorTemp != *p.valorActual)) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(p.nombre);

      lcd.setCursor(0,1);
      lcd.print("                ");

      lcd.setCursor(0,1);
      lcd.print(*p.valorActual);

      valorTemp = *p.valorActual;
      itemAnterior = itemSubMenu;
    }
  }
  else {
    Parametro p = menus[itemMenu].parametros[itemSubMenu];

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(p.nombre);
    lcd.setCursor(0,1);

    if(modoEdicion) {
      lcd.print("EDIT: ");
      lcd.print(valorEditando);
    }
    else {
      lcd.print(EEPROM.read(p.direccionEEPROM));
    }
  }
}

//===================
// LIMITES MENU
//===================

void limitarMenu() {
  if(itemMenu < 0)  itemMenu = 0;
  if(itemMenu > 5)  itemMenu = 5;
}

//===========================
// LIMITES SUBMENU
//==========================

void limitarSubMenu() {
  int maximo;

  if(menuEstado) {
    maximo = 4;
  }
  else {
    maximo = menus[itemMenu].cantidadParametros - 1;
  }

  if(itemSubMenu < 0)       itemSubMenu = 0;
  if(itemSubMenu > maximo)  itemSubMenu = maximo;
}

//==============================
// FUNCIONES EEPROM
//==============================
void cargarValoresDefault() {
  for(int i = 0; i < 14; i++) {
    EEPROM.write(i, valoresDefault[i]);
  }
}

void restaurarValoresDefault() {
  cargarValoresDefault();  
  EEPROM.write(EEPROM_INICIALIZADA, 123);
}

void inicializarEEPROM() {
  if(EEPROM.read(EEPROM_INICIALIZADA) != 123) {
    cargarValoresDefault();
    EEPROM.write(EEPROM_INICIALIZADA, 123);
  }
}

//====================================
// FUNCION PARA MANEJAR LAS ALERTAS
//====================================
void Alerta(){
    if(estadoAlerta && !alertaMostrada){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(dispositivoAlerta);

      lcd.setCursor(0,1);
      lcd.print(msjAlerta);

      tiempoAlerta = millis();
      alertaMostrada = true;
    }

    if(alertaMostrada && (millis() - tiempoAlerta >= 500)){
      estadoAlerta = false;
      alertaMostrada = false;

      itemAnterior = -999;
      valorTemp = -999;

      if(estado == MENU_PRINCIPAL){
        mostrarMenu();
      }
      else{
        mostrarSubMenu();
      }
    }
}

//=====================================================================
// FUNCION VENTILADOR
//=====================================================================
/*
  El objetivo de la función es enfriar el ambiente de la casa regulando la velocidad 
  de un extractor o ventilador y alertar los cambios térmicos del ambiente.
  
  - Lee la temperatura ambiente usando el sensor TPM36.
  - Si la temperatura <= Temp OFF (P1): La se apaga el ventilador 
  - Si la temperatura está entre Temp OFF (P1) y Temp Max (P0), el ventilador se activa a Vel Min 
  - Si la temperatura >= Temp Max (P0): Enciende a máxima potencia el ventilador
*/

void ventilador() {
  byte tempMax = EEPROM.read(0);
  byte tempOFF = EEPROM.read(1);
  byte velMin  = EEPROM.read(2);
  byte velMax  = EEPROM.read(3);

  if(tempAnterior != datos.temperatura){
    if(datos.temperatura <= tempOFF) {
      analogWrite(11, 0);
      dispositivoAlerta = "Ventilador";
      msjAlerta = "OFF";
      estadoAlerta = true;
    }
    else if(datos.temperatura > tempOFF && datos.temperatura < tempMax) {
      analogWrite(11, velMin);
      dispositivoAlerta = "Ventilador";
      msjAlerta = "ON V_MIN";
      estadoAlerta = true;
    }
    else if(datos.temperatura >= tempMax) {
      analogWrite(11, velMax);
      dispositivoAlerta = "Ventilador";
      msjAlerta = "ON V_MAX";
      estadoAlerta = true;
    }
    tempAnterior = datos.temperatura;
  }
}

//===================================
// FUNCION CISTERNA
//===================================

/*
  El objetivo de la funcion es asegurar que la bomba no trabaje 
  si la cisterna esta seca y alertar la cantidad de fluido en la cisterna
  
  - Mide la distancia al agua usando el sensor ultrasónico HC-SR04.
  - Si la distancia >= Nivel Min (P4): El tanque está vacío; apaga la 
  - bomba inmediatamente por seguridad para evitar trabajo en seco.
  - Si la distancia <= Nivel Max (P5): El tanque está lleno.
  - Rango medio: Mantiene la bomba encendida regulando el flujo.
  
*/


void cisTerna() {
    byte distVacio = EEPROM.read(4); 
    byte distLleno = EEPROM.read(5);

    static bool bombaOperando = false;

    if (nivelAnterior != datos.distancia) {
        if (datos.distancia >= distVacio) {
            digitalWrite(7, LOW);
            bombaOperando = false;
            dispositivoAlerta = "BOMBA";
            msjAlerta = "ALERTA: VACIO";
            estadoAlerta = true;
        }
        else if (datos.distancia <= distLleno) {
              dispositivoAlerta = "BOMBA";
              msjAlerta = "ALERTA: LLENO";
              digitalWrite(7, HIGH);
              estadoAlerta = true;
        }
        else if (datos.distancia < distVacio && datos.distancia > distLleno) {
            if (!bombaOperando) {
                digitalWrite(7, HIGH);
                bombaOperando = true;
                dispositivoAlerta = "BOMBA";
                msjAlerta = "BOMBA MEDIO";
                estadoAlerta = true;
            }
        }
        nivelAnterior = datos.distancia;
    }
}

//===========================
// FUNCION RIEGO
//===========================

/*
  El objetivo de la función es mantener la hidratación óptima del suelo
  y alertar cuando el flujo de agua se abre o se cierra.
  
  - Mide el porcentaje de humedad del suelo usando el sensor higrómetro.
  - Si la humedad <= Hum Min (P6), Significa que el suelo está seco y acciona 
    el servo el movimiento representa la apertura de una valvula de bola real
  - Si la humedad >= Hum Normal (P7) significa que el suelo tiene suficiente agua
    entonces el servomotor vuelve a la posicion de reposo
*/


void riegoJardin(){
    byte humMin    = EEPROM.read(6);
    byte humNormal = EEPROM.read(7);
    byte servoPos  = EEPROM.read(8);

    if(humedadAnterior != datos.humedad){
        if(datos.humedad <= humMin){
            servo.write(0);
            dispositivoAlerta = "RIEGO";
            msjAlerta = "RIEGO ON";
            estadoAlerta = true;
        }
        else if(datos.humedad >= humNormal){
            servo.write(servoPos);
            dispositivoAlerta = "RIEGO";
            msjAlerta = "RIEGO OFF";
            estadoAlerta = true;
        }
        humedadAnterior = datos.humedad;
    }
}

//==============================================
// FUNCION LUCES 
//==============================================

/*

  El objetivo de la función es gestionar el encendido de las luces del patio
  de la casa por medio de un sensor foto resistivo el cual al detectar poca luz
  acciona  una bombilla que representa las luces exteriores.
  
  - Monitorea el nivel de claridad ambiental mediante una fotoresistencia LDR.
  - Modo Auto Compara la lectura del LDR y enciende las luces al anochecer y lo apaga al detectar luz de día.
  - Se puede cambiar al modo manual para poder accionar las luces en caso que sea necesario 
    en este modo ignora las señales del LDR
*/


void luces() {
  byte sensibilidad = EEPROM.read(9);
  byte modo = EEPROM.read(10); 

  static int estadoLuzAnterior = -1;
  int estadoLuzActual = 0; 

  //=============================================
  // MODO 1: AUTOMÁTICO (Depende del sensor LDR)
  //=============================================
  if (modo == 1) {
    estadoLuzActual = (datos.luz < sensibilidad) ? 1 : 0;

    if (estadoLuzAnterior != estadoLuzActual) {
      if (estadoLuzActual == 1) {
        digitalWrite(8, HIGH);
        dispositivoAlerta = "Luces";
        msjAlerta = "AUTO ON";
        estadoAlerta = true;
      }
      else {
        digitalWrite(8, LOW);
        dispositivoAlerta = "Luces"; 
        msjAlerta = "AUTO OFF (Dia)";
        estadoAlerta = true;
      }
      estadoLuzAnterior = estadoLuzActual;
    }
  }
  //=========================================
  // MODO 0: MANUAL 
  //=========================================
  else if (modo == 0) {
    
    if (modoEdicion && itemMenu == 3 && itemSubMenu == 0) { 
      estadoLuzActual = (valorEditando > 0) ? 1 : 0; 
    } else {
      estadoLuzActual = (EEPROM.read(9) > 0) ? 1 : 0;
    }

    if (estadoLuzAnterior != estadoLuzActual) {
      if (estadoLuzActual == 1) {
        digitalWrite(8, HIGH);
        dispositivoAlerta = "Luces"; 
        msjAlerta = "Manual ON";
        estadoAlerta = true;
      }
      else {
        digitalWrite(8, LOW);
        dispositivoAlerta = "Luces"; 
        msjAlerta = "Manual OFF";
        estadoAlerta = true;
      }
      estadoLuzAnterior = estadoLuzActual;
    }
  }
}

//===========================================================
// FUNCION SIRENA (CORREGIDA CON TIEMPO EN MILISEGUNDOS)
//===========================================================

/*

  El objetivo de la función es asegurar la seguridad perimetral detectando
  presencia física sin congelar los procesos del menú ni de los actuadores.
  
  - Monitorea el estado de presencia utilizando un sensor de movimiento PIR.
  - Si la seguridad (P11) y sirena (P12) están activas, dispara la alarma al detectar un intruso.
  
*/


void sirena() {
  byte activarSeguridad = EEPROM.read(11);
  byte sirenaHabilitada = EEPROM.read(12);
  unsigned long tiempoLimiteSegundos = EEPROM.read(13); 

  if (activarSeguridad == 1 && sirenaHabilitada == 1) {

    // Si detecta movimiento, activa la alarma
    if (datos.movimiento == 1) {
      if (!sirenaActiva) {
        dispositivoAlerta = "Seguridad";
        msjAlerta = "INTRUSO";
        estadoAlerta = true;
        sirenaActiva = true;
      }
      tiempoInicioSirena = millis(); 
    }

    // Control del tiempo total y tipo de sonido
    if (sirenaActiva) {
      if (millis() - tiempoInicioSirena < (tiempoLimiteSegundos * 100)) {
        
      
        unsigned long tiempoCiclo = millis() % 1500; 

      
        if (tiempoCiclo < 1000) {
          digitalWrite(10, HIGH); delayMicroseconds(150); 
          digitalWrite(10, LOW);  delayMicroseconds(150);
        } 
        
        else {
          digitalWrite(10, LOW); 
        }
      } 
      else {
        digitalWrite(10, LOW);
        sirenaActiva = false;
      }
    }
  } 
  else {
    digitalWrite(10, LOW);
    sirenaActiva = false;
  }
}

//===========================
// SETUP
//===========================

void setup() {
  inicializarEEPROM();
  
  servo.attach(6);     // Riego
  pinMode(7, OUTPUT);  // cisterna
  pinMode(8, OUTPUT);  // Luces
  pinMode(10, OUTPUT); // Piezo
  pinMode(11, OUTPUT); // Ventilacion
  
  Wire.begin();
  lcd.begin(16,2);
  lcd.setBacklight(HIGH);

  lcd.print(F("UES 2026 EBB115"));
  delay(500);

  lcd.clear();
  lcd.print(F("Bienvenido"));
  delay(500);

  mostrarMenu();
}

//==========================
// LOOP 
//==========================

void loop() {
  Wire.requestFrom(8, sizeof(datos));

  if(Wire.available()) {
    Wire.readBytes((byte*)&datos, sizeof(datos));
  }

  if(menuEstado && estado == SUBMENU) {
    mostrarSubMenu();
  }
  
  ventilador();
  cisTerna();
  riegoJardin();
  luces();
  sirena();
  Alerta();

  tecla = teclado.getKey();

  if(tecla == NO_KEY) {
    return;
  }

  // MENU PRINCIPAL
  if(estado == MENU_PRINCIPAL) {
    if(tecla == 'A') {
      itemMenu++;
      limitarMenu();
      mostrarMenu();
    }
    else if(tecla == 'D') {
      itemMenu--;
      limitarMenu();
      mostrarMenu();
    }
    else if(tecla == '#') {
      estado = SUBMENU;
      itemSubMenu = 0;

      if(itemMenu == 5) {
        menuEstado = true;
      }
      else {
        menuEstado = false;
      }
      mostrarSubMenu();
    }
  }

  // SUBMENU
  else if(estado == SUBMENU) {
    if(menuEstado) {
      if(tecla == 'A') {
        itemSubMenu++;
        limitarSubMenu();
        mostrarSubMenu();
      }
      else if(tecla == 'D') {
        itemSubMenu--;
        limitarSubMenu();
        mostrarSubMenu();
      }
      else if(tecla == '*') {
        estado = MENU_PRINCIPAL;
        mostrarMenu();
      }
    }
    else {
      // MODO EDICION EEPROM
      if(tecla == '#') {
        Parametro p = menus[itemMenu].parametros[itemSubMenu];
        modoEdicion = true;
        valorEditando = EEPROM.read(p.direccionEEPROM);
        mostrarSubMenu();
      }
      else if(tecla == 'C' && modoEdicion) {
        Parametro p = menus[itemMenu].parametros[itemSubMenu];
        EEPROM.write(p.direccionEEPROM, valorEditando);
        modoEdicion = false;

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Guardado");
        delay(700);
        mostrarSubMenu();
      }
      else if(tecla == 'B' && modoEdicion) {
        modoEdicion = false;
        mostrarSubMenu();
      }
      else if(tecla == 'A') {
        if(modoEdicion) {
          valorEditando++;
          if(valorEditando > 255) valorEditando = 255;
        }
        else {
          itemSubMenu++;
          limitarSubMenu();
        }
        mostrarSubMenu();
      }
      else if(tecla == 'D') {
        if(modoEdicion) {
          valorEditando--;
          if(valorEditando < 0) valorEditando = 0;
        }
        else {
          itemSubMenu--;
          limitarSubMenu();
        }
        mostrarSubMenu();
      }
      else if(tecla == '*') {
        modoEdicion = false;
        estado = MENU_PRINCIPAL;
        mostrarMenu();
      }
    }
  }
}