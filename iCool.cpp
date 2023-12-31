#include <Arduino.h>
#include <StarterKitNB.h>
#include <SparkFun_SHTC3.h>
#include <Wire.h>

#define PIN_VBAT WB_A0
#define SENSOR_PIN  WB_IO6   // Attach AM312 sensor to Arduino Digital Pin WB_IO6

#define VBAT_MV_PER_LSB (0.73242188F) // 3.0V ADC range and 12 - bit ADC resolution = 3000mV / 4096
#define VBAT_DIVIDER_COMP (1.73)      // Compensation factor for the VBAT divider, depend on the board
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)
#define SENSOR_PIN  WB_IO6   // Attach AM312 sensor to Arduino Digital Pin WB_IO6

int gCurrentStatus = 0;         // variable para leer estado actual del pin
int gLastStatus = 0;            // variable para leer último estado del pin

SHTC3 mySHTC3;  // Variable que guarda los datos del sensor
StarterKitNB sk;  // Variable para la funcionalidad del Starter Kit

// NB
String apn = "m2m.entel.cl";
String user = "entelpcs";
String pw = "entelpcs";
String band = "B28 LTE";

// Thingsboard
String ClientIdTB = "grupo11";
String usernameTB = "bbbbb";
String passwordTB = "bbbbb";
String msg = "";
String resp = "";
String Operador = "";
String Banda = "";
String Red = "";


void errorDecoder(SHTC3_Status_TypeDef message)             // Función que facilita la detección de errores
{
  switch(message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}


void setup()
{
  pinMode(SENSOR_PIN, INPUT);   // The Water Sensor is an Input
  pinMode(LED_GREEN, OUTPUT);  // The LED is an Output
  pinMode(LED_BLUE, OUTPUT);   // The LED is an Output
  sk.Setup(false);
  delay(500);
  sk.UserAPN(apn, user, pw);
  delay(500);
  Wire.begin();
  sk.Connect(apn, band);
  Serial.print("Beginning sensor. Result = ");            
  errorDecoder(mySHTC3.begin());                         // Detectar errores
  Serial.println("\n\n");
}

void loop()
{
  if (!sk.ConnectionStatus()) // Si no hay conexion a NB
  {
    sk.Reconnect(apn, band);  // Se intenta reconecta
    delay(2000);
  }

  sk.ConnectBroker(ClientIdTB, usernameTB, passwordTB);  // Se conecta a ThingsBoard
  delay(2000);

  resp = sk.bg77_at((char *)"AT+COPS?", 500, false);
  Operador = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);     // ENTEL PCS
  delay(1000);

  resp = sk.bg77_at((char *)"AT+QNWINFO", 500, false);
  Red = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);          // NB o eMTC
  Banda = resp.substring(resp.lastIndexOf("\"",resp.lastIndexOf("\"")-1), resp.lastIndexOf("\"")+1); 
  delay(1000);
  
  gCurrentStatus = digitalRead(SENSOR_PIN);
  if(gLastStatus != gCurrentStatus)
  {
    if(gCurrentStatus == 0)
    {//0: detected   1: not detected
      Serial.println("IR detected ...");
     digitalWrite(LED_GREEN,HIGH);   //turn on
     digitalWrite(LED_BLUE,HIGH);
    }
    else
    {
      Serial.println("IR no detected.");
      digitalWrite(LED_GREEN,LOW);
      digitalWrite(LED_BLUE,LOW);   // turn LED OF
    }
    gLastStatus = gCurrentStatus;
  }
  else
  {
    delay(100);
  }

  SHTC3_Status_TypeDef result = mySHTC3.update();             // resultado actual si se necesita
  if(mySHTC3.lastStatus == SHTC3_Status_Nominal)              // Actualiza y printea los datos de Humedad y temperatura en °C
  {
    msg = 
    "{\"humidity\":"+String(mySHTC3.toPercent())
    +",\"temperature\":"+String(mySHTC3.toDegC())
    +",\"battery\":"+String(round(analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB)/37)
    +",\"PIR\":"+String(gCurrentStatus)
    +",\"Operador\":"+Operador
    +",\"Banda de frecuencia\":"+Banda
    +",\"Red IoT\":"+Red+"}";
    Serial.println(msg);  
    sk.SendMessage(msg);
  }
  else
  {
    msg = 
    "{\"humidity\":\"ERROR\", \"temperature\":\"ERROR\",\"battery\":"
    +String(round(analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB)/37)
    +",\"Operador\":"+Operador
    +",\"Banda de frecuencia\":"+Banda
    +",\"Red IoT\":"+Red+"}";
    Serial.println(msg);
    sk.SendMessage(msg);
  }

  sk.DisconnectBroker();
  delay(1000);                                                 // Tiempo de espera para cada entrega de datos
}







