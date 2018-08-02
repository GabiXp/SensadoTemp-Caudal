//FranciscoGM for Cesus
#include <ESP8266WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "HTTPSRedirect.h"
////////////////////////////////////////////////////////////////////////////
#define SENSOR_TEMP_CALIENTE D6
#define SENSOR_TEMP_FRIO D7
#define CAUDALIMETRO D5
#define LED_DESCONECTADO D4
#define LED_CONECTADO D3
#define ENCENDIDO 0
////////////////////////////////////////////////////////////////////////////
const char* ssid = "Speedy-Abel"; //Barrio15
const char* password = "2135210770";
//const char* ssid = "BA WiFi";//BA Wifi
//const char* password = "";
//const char* ssid = "SNWIFI-DETRAS DE TODO";//Sueños bajitos
//const char* password = "47737735";
//////////////////////////////////////////////////////////////////////////////
float tempCalienteSensada,tempFriaSensada;
int litros_Hora,litros_minuto=60,factor_conversion=3.5; // Litros_hora Variable que almacena el caudal (L/hora)
const int measureInterval = 1000; //int medicion caudal 1 seg
volatile int pulsos = 0; // Variable que almacena el número de pulsos
float litros=0; // // Variable que almacena el número de litros acumulados
unsigned long pulsos_Acumulados = 0; // Variable que almacena el número de pulsos acumulados
volatile int contadorPulsos=0;
/////////////////////////////////////////////////////////////////////////////
const char *GScriptId = "AKfycbxvvC6U4j91O-s-a93UIGsgzs9-UBY7cGCU81_7hiLsimGxQFg"; //id google sheets
unsigned long millisActuales,millisPasados=0,mintervalo,mintervaloAnt=0;
const int dataPostDelay = 3500;  // delay subida de info 3.5 segundos
const int dataPostDelayHora = 3600000; //1 hora
/////////////////////////////////////////////////////////////////////////////
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort =  443;
HTTPSRedirect client(httpsPort);
String url = String("/macros/s/") + GScriptId + "/exec?";
const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";
/////////////////////////////////////////////////////////////////////////////
OneWire oneWireC (SENSOR_TEMP_CALIENTE);
OneWire oneWireF (SENSOR_TEMP_FRIO);
DallasTemperature sensorCaliente(&oneWireC);
DallasTemperature sensorFrio(&oneWireF);
/////////////////////////////////////////////////////////////////////////////

void gpio()
{
  sensorCaliente.begin();
  sensorFrio.begin();
  pinMode(LED_CONECTADO,OUTPUT);
  pinMode(LED_DESCONECTADO,OUTPUT);
  digitalWrite(LED_DESCONECTADO,HIGH);
  digitalWrite(LED_CONECTADO,LOW);
}
///////////////////////////////////////////
void ConfInicialWifi()
{
  Serial.begin(115200);Serial.setDebugOutput(true);
  Serial.println("F/ Conectando a wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if (WiFi.status() != WL_CONNECTED) {
  WiFi.begin(ssid,password);
}
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" IP address: ");
  Serial.println(WiFi.localIP());
}
void reconexion_wifi()
{
    client.stop();
    WiFi.disconnect();     
    Serial.println("F/ Conectando a wifi: ");
    Serial.println(ssid);
    Serial.flush();
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid,password);
    }
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }     
}
///////////////////////////////////////////
void ConfFinal()
{
  Serial.print(String("G/ Conectando a "));
  Serial.println(host);
  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Conexion fallida. Reintentando...");
  }

  Serial.println("M/ Estado de Conexion: " + String(client.connected()));
  Serial.flush();
  
  if (!flag){
    Serial.print("No se pudo conectar al servidor: ");
    Serial.println(host);
    Serial.println("Saliendo...");
    Serial.flush();
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
}
////////////////////////////////////////////////////////////////////////
void postData(float TempCaliente,float valorCaudal,float tempFria){
  if(WiFi.status() != WL_CONNECTED)
  {
    reconexion_wifi();
  }
  if (!client.connected())
  { 
    digitalWrite(LED_DESCONECTADO,HIGH);
    digitalWrite(LED_CONECTADO,LOW);
    Serial.println("Conectandose al cliente..."); 
    client.connect(host, httpsPort);
  }
  String urlFinal = url + "&temperaturaCaliente=" + String(TempCaliente) + "&valorCaudal="+String(valorCaudal)+ "&temperaturaFria="+String(tempFria);
  client.printRedir(urlFinal, host, googleRedirHost);
}
////////////////////////////////////////////////////////////////////////
void PostearInfo()
{
  if(millisActuales-millisPasados>=dataPostDelay)
  {
    // Realizo los cálculos
    mintervaloAnt = millisActuales; // Actualizo el nuevo tiempo
    pulsos_Acumulados += contadorPulsos; // Número de pulsos acumulados
    litros_Hora = (contadorPulsos * litros_minuto / factor_conversion); // Q = frecuencia * 60/ 5.5 (L/Hora)
    litros = pulsos_Acumulados*1.0/450; // Cada 450 pulsos son un litro?
    contadorPulsos = 0; // Pongo nuevamente el número de pulsos a cero
    millisPasados=millisActuales;
    postData(tempCalienteSensada,litros,tempFriaSensada); //Posteo de datos && reconexion en caso de desconectado
    contadorPulsos=0;
    pulsos_Acumulados=0;
  }
}
////////////////////////////////////////////////////////////////////////
void Sensados()
{
  sensorCaliente.requestTemperatures();
  tempCalienteSensada = sensorCaliente.getTempCByIndex(0);
  sensorFrio.requestTemperatures();
  tempFriaSensada=sensorFrio.getTempCByIndex(0);
  attachInterrupt(CAUDALIMETRO,pulsoContador,RISING);
}
////////////////////////////////////////////////////////////////////////
void ledConectado()
{
  if(client.connected()&&digitalRead(LED_CONECTADO)!=HIGH) 
  {
    digitalWrite(LED_CONECTADO,HIGH);
    digitalWrite(LED_DESCONECTADO,LOW);
  }
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
void setup() {
  gpio();
  ConfInicialWifi();
  ConfFinal();
  contadorPulsos=0;
  attachInterrupt(CAUDALIMETRO,pulsoContador,RISING);
}

///////////////////////////////////////////////////////////////////////
void loop() 
{
  ledConectado();
  millisActuales=millis();
  Sensados();
  PostearInfo(); //posteo de info <----Calibrar caudalimetro && reset de variables 
}
void pulsoContador()
{
  contadorPulsos++;
}
