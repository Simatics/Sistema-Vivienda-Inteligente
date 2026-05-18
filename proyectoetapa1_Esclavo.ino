#include <Wire.h>

//=================================================
//                    ESCLAVO
//=================================================

/*
    Alfaro Laínez Pedro Ernesto a119002
    Aquino Escobar Oscar René ae18021
    Martínez Escalante Emerson Adalberto me12007
    Rivera Deleón Allison Nicole rd20019
    Sosa García Elsbeth Ellen sg92004
*/

//==============================================
// DECLARACION DE VARIABLES
//==============================================

// LM35
float v_temp = A0;
float lectura, volt, temperatura;

// LDR
int v_ldr = A1;
int luz;

// PIR
int v_pir = A2;
int movimiento;

// Humedad
int v_humedad = A3;
int humedad;

// Ultrasonico
const int echoPin = 12;
const int trigPin = 13;

long duracion;
int distancia;

//==============================================
// ESTRUCTURA DE DATOS A ENVIAR
//==============================================

struct DatosSensores {
  int temperatura;
  int luz;
  int movimiento;
  int humedad;
  int distancia;
};

DatosSensores datos;

//===================================================
// FUNCION PARA MANEJAR EL SENSOR DE TEMPERATURA A0
//===================================================

float lectura_temperatura() {
    long sumaLecturas = 0;
    int muestras = 10; 

    for (int i = 0; i < muestras; i++) {
        sumaLecturas += analogRead(v_temp);
        delay(10);
    }

    // Calculamos el promedio de la lectura analógica
    float promedioADC = (float)sumaLecturas / muestras;

    // Convertimos el promedio a voltaje
    float volt = (promedioADC * 5.0) / 1023.0;

    // Convertimos voltaje a temperatura (Lógica para TMP36)
    float temperatura = (volt - 0.5) * 100.0; 

    return temperatura; 
}

//=================================================
// FUNCION PARA MANEJAR LA FOTORESISTENCIA LDR A1
//=================================================

int lectura_ldr() {

  long suma = 0;

  // Promedio de 10 muestras
  for(int i = 0; i < 10; i++) {
    suma += analogRead(v_ldr);
    delay(5);
  }

  // Valor promedio ADC
  luz = suma / 10;

  return luz;
}

//=================================================
// FUNCION PARA MANEJAR EL SENSOR PIR A2
//=================================================

int lectura_pir() {

  movimiento = digitalRead(v_pir);

  return movimiento;
}

//=================================================
// FUNCION PARA MANEJAR EL SENSOR DE HUMEDAD A3
//=================================================

int lectura_humedad() {
    long suma = 0;
    for(int i = 0; i < 10; i++) { // Lee 10 veces
        suma += analogRead(v_humedad);
        delay(10); 
    }
    int promedioADC = suma / 10;

    int porcentaje = map(promedioADC, 0, 876, 0, 100);
  
    return constrain(porcentaje, 0, 100);
  
}

//===============================================================
// FUNCION PARA MANEJAR EL SENSOR ULTRASONICO D12(ECHO) D13(TRG)
//===============================================================

int lectura_ultrasonico() {

  long suma = 0;
  int muestras = 10;

  for(int i = 0; i < muestras; i++) {

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    duracion = pulseIn(echoPin, HIGH);

    distancia = duracion * 0.034 / 2;

    suma += distancia;

    delay(10);
  }

  distancia = suma / muestras;

  return distancia;
}

//==============================================
// FUNCION QUE ENVIA LOS DATOS AL ESCLAVO
//==============================================

void enviarDatos() {

  Wire.write((byte*)&datos, sizeof(datos));
}

//==============================================
// CONFIGURACION INICIAL DE LA TARJETA
//==============================================

void setup() {

  Wire.begin(8);

  // CUANDO EL OTRO ARDUINO PIDA DATOS
  // SE EJECUTA ESTA FUNCION AUTOMATICAMENTE
  Wire.onRequest(enviarDatos);

  Serial.begin(9600);

  pinMode(v_pir, INPUT);

  pinMode(trigPin, OUTPUT);

  pinMode(echoPin, INPUT);
}

//==============================================
// FLUJO DEL PROGRAMA
//==============================================

void loop() {

  //==========================================
  // LEER TODOS LOS SENSORES
  //==========================================

  datos.temperatura = lectura_temperatura();

  datos.luz = lectura_ldr();

  datos.movimiento = lectura_pir();

  datos.humedad = lectura_humedad();

  datos.distancia = lectura_ultrasonico();

  //==========================================
  // MOSTRAR EN MONITOR SERIAL
  //==========================================

  Serial.print("Temp: ");

  Serial.print(datos.temperatura);

  Serial.print(" | LDR: ");

  Serial.print(datos.luz);

  Serial.print(" | PIR: ");

  Serial.print(datos.movimiento);

  Serial.print(" | Humedad: ");

  Serial.print(datos.humedad);

  Serial.print(" | Distancia: ");

  Serial.println(datos.distancia);

  delay(1000);
}