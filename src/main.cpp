//MM
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
// http://www.steves-internet-guide.com/mqtt-protocol-messages-overview/
// https://docs.platformio.org/en/latest/platforms/espressif32.html#cpu-frequency
// https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode

//watchdog: https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/

//#include "autoconnect_stuff.h"
#include <WiFiClientSecure.h>
//#include <MQTTClient.h>
//#include <NTPClient.h>
//#include "WiFi.h"
//#include <WiFiUdp.h>
//#include <SPI.h>
//#include <LoRa.h>
/////#include <time.h>
//#include <AutoConnect.h>
//#include <WebServer.h>
#include <HardwareSerial.h>
HardwareSerial atSerial(1);

#include <OneWire.h>
#include <DS18B20.h>

#define ONE_WIRE_BUS 15

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);


const uint ServerPort = 23;
WiFiServer Serverx(ServerPort);
WiFiClient RemoteClient;




#include "certs.h"
#include "configs.h"
//#include <autoconnect_stuff.h>


#include <NTPClient.h> //updatetime
#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <time.h>
#include <AutoConnect.h>
#include "helpers/OTAClient.h"
OTAClient OTA;


#include <ESP32Ping.h>  //to check if there is internet, not only wifi
//#include <esp_task_wdt.h>   //watchdog

#include <EEPROM.h>   //para guardar la fecha si se traba
#define EEPROM_SIZE 10
byte timebackup=0;


WiFiUDP ntpUDP;
//WiFiClientSecure net = WiFiClientSecure();
//MQTTClient client = MQTTClient(256);
NTPClient timeClient(ntpUDP);





static const char AUX_TIMEZONE[] PROGMEM = R"(
{
  "title": "TimeZone",
  "uri": "/timezone",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "Sets the time zone to get the current local time.",
      "style": "font-family:Arial;font-weight:bold;text-align:center;margin-bottom:10px;color:DarkSlateBlue"
    },
    {
      "name": "timezone",
      "type": "ACSelect",
      "label": "Select TZ name",
      "option": [],
      "selected": 10
    },
    {
      "name": "newline",
      "type": "ACElement",
      "value": "<br>"
    },
    {
      "name": "start",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/start"
    }
  ]
}
)";

typedef struct {
  const char* zone;
  const char* ntpServer;
  int8_t      tzoff;
} Timezone_t;

static const Timezone_t TZ[] = {
 
 
  { "America/Chicago", "north-america.pool.ntp.org", -6 },
  { "America/Denver", "north-america.pool.ntp.org", -7 },

};

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer Server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer Server;
#endif

AutoConnect       Portal(Server);
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4
AutoConnectAux    Timezone;


// void rootPage() {
//   String  content =
//     "<html>"
//     "<head>"
//     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
//     "<script type=\"text/javascript\">"
//     "setTimeout(\"location.reload()\", 1000);"
//     "</script>"
//     "</head>"
//     "<body>"
//     "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Hello, world</h2>"
//     "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
//     "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
//     "<p></p><p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
//     "</body>"
//     "</html>";
//   static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
//   struct tm *tm;
//   time_t  t;
//   char    dateTime[26];

//   t = time(NULL);
//   tm = localtime(&t);
//   sprintf(dateTime, "%04d/%02d/%02d(%s) %02d:%02d:%02d.",
//     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
//     wd[tm->tm_wday],
//     tm->tm_hour, tm->tm_min, tm->tm_sec);
//   content.replace("{{DateTime}}", String(dateTime));
//   Server.send(200, "text/html", content);
// }

// void startPage() {
//   // Retrieve the value of AutoConnectElement with arg function of WebServer class.
//   // Values are accessible with the element name.
//   String  tz = Server.arg("timezone");

//   for (uint8_t n = 0; n < sizeof(TZ) / sizeof(Timezone_t); n++) {
//     String  tzName = String(TZ[n].zone);
//     if (tz.equalsIgnoreCase(tzName)) {
//       configTime(TZ[n].tzoff * 3600, 0, TZ[n].ntpServer);
//       Serial.println("Time zone: " + tz);
//       Serial.println("ntp server: " + String(TZ[n].ntpServer));
//       break;
//     }
//   }

//   // The /start page just constitutes timezone,
//   // it redirects to the root page without the content response.
//   Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
//   Server.send(302, "text/plain", "");
//   Server.client().flush();
//   Server.client().stop();
// }
//////////------------
//////////////////////////// TASKS FOR EACH CORE //////////////////////////////////
TaskHandle_t Task1;
TaskHandle_t Task2;
//create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0


                                                                                                              /////   declaracion de variables  //////////


const unsigned int BACKUPSIZE=200;
int o=0;
unsigned long currenttimearray[BACKUPSIZE];
float payloadbag[BACKUPSIZE][16];


int multiplos=30;            //(sin unidad) cuantos "TiempoLectura" leo continuamente antes de reportar a la nube
int reportacada=230;         //(milisegundos) x por multilplos por TiempoLectira. es el tiempo maximo de espera si la lectura es cero y no reporta 

const int TiempoLectura=1000;      //


float vbat=0;
#define lipocheck 36  //GPIO36 analog pin para medir voltaje de bateria 
#define flowpin 34   //digital pin para medir el sensor de flujo
#define flowled 21  //antes era gpio2 porque el devmodule de menos pines tiene led, falta agregar uno para la nueva version
#define ledgreen 32 //32 para esp32 grandes, 33 para chicos
#define ledred 33   //33 para esp32 grandes, 32 para chicos
#define bot 26
#define railEnable 25
#define enserialport 2


unsigned long previousMillistimerbkp;
unsigned long previousMillistimerOTA;

int flagnohayluz;
int publishnow=0;
float flow=0;
int pulses=0;
const int sensormodel=1;
const float calib = 1.0;
long currentTime;
long backupcurrentTime;
long timercurrentTime;
float counterstatus=0;
int intcounterstatus=0;


float flowtemp1;
float flowtotal; //para guardar en memoria usar: RTC_DATA_ATTR , por ejemplo RTC_DATA_ATTR int bootCount = 0;
float flowacum;
const int publishconfig=1;
int flagonline=0;
int initialmessages=2;
long zeit,prezeit;
int flagpubcount;
const int maxwait=400;
int flagpub;
int buttonflag=0;
int timeupdate=0;
int flagsend=0;
int httpCode =0;
float flowtemp=0;
int x=0;
long zeit2=0;
int avg_time_ms;

unsigned long timer = 0; //para hacer currentTime backup en eeprom cada hora
unsigned long timerOTA = 0; //para checar si hay actualizacion (cada 60 seg)
unsigned long timerReset = 0; //para checar si hay actualizacion (cada 60 seg)

int i=0;//temp
bool acEnable;

//---------atSERIAL-----------
char bufserial[40];
int lengthpayload;
int validator = 0;
//const byte whitelistlength=5;
//int whiteID[whitelistlength];

float bufhotflow;
float bufhottempIN;
float bufhottempOUT;
float bufsolarbat;
float bufnivo1;
float bufnivo1temp;
float bufnivo1bat;
float bufsolarcount;
float bufboilercount;
float bufnivocount;
int sensorID;
byte caso=0;



const float A[10][3]=       { //sensor1, input , output           sensor1 = DN20 flow sensor    input=pulsos en 1seg, output=mililitros
                            { 1,    135.4 , 320.5 },//originalmente: 135.4,320.5
                            { 1,     71.5 , 175.4 },//71.5,175.4
                            { 1,     40.6 , 103.6 },//40.6,103.6
                            { 1,     14.0 ,  44.1 },//14,44.1
                            { 1,      3.2 ,  16.6 },//3.2,16.6
                            //sensor 2     
                            { 2,       0 ,    0 },
                            { 2,       0 ,    0 },
                            { 2,       0 ,    0 },
                            { 2,       0 ,    0 },
                            { 2,       0 ,    0 }
};

void updatecurrentTime()
{

Serial.print("updatecurrentTime: ");

//EEPROM.read(0);//leer

//EEPROM.write(0, ledState);
//EEPROM.commit();

  //Serial.print("timebackup=");Serial.println(timebackup);
  Serial.println(currentTime);


  timeClient.begin();
  timeClient.setTimeOffset(0);
  timeClient.update();
  currentTime = timeClient.getEpochTime();
  timeClient.end();

  // if (currentTime < 1600000000)
  // {
  //   timeClient.begin();
  //   timeClient.setTimeOffset(0);
  //   timeClient.update();
  //   timeClient.end();
  // }

 // Serial.print("currentTime: Time.now: ");
  //Serial.println(currentTime); //rtc.nowEpoch();
  if (1600000000 < currentTime && currentTime < 2000000000)
  {
    backupcurrentTime = currentTime;
    timercurrentTime = millis();
  }
  else
  {
    String currentTimeStr;
    currentTime = backupcurrentTime + (millis() - timercurrentTime) / 1000;
    if (currentTime < 1600000000)
    {
      String readbackup = "";
      for (int i = currentTimeStr.length(); i > 0; i--)
      { //imprime lo que guardo para confirmar
        
        Serial.print(EEPROM.read(i));
        readbackup = readbackup + EEPROM.read(i); //leer
      }
      unsigned long dfdfa = currentTimeStr.substring(0).toInt();
      Serial.print("recovered: ");
      Serial.println(dfdfa);
      currentTime=dfdfa;
      timebackup = 0;
      readbackup = "";
    }
  }

  if(timebackup==1 && currentTime > 1600000000){
    Serial.print("backing up currentTime: ");Serial.println(currentTime);
    String currentTimeStr = String(currentTime);
    for (int i=currentTimeStr.length();i>0;i--){  //length deberia ser siempre 10 (por ej 1.600.000.000)
      byte x = currentTimeStr.substring(i, i).toInt();
      EEPROM.write(i, x);
      EEPROM.commit();
      //Serial.print(i);
    }
    String readbackup="";
    for (int i=currentTimeStr.length();i>0;i--){   //imprime lo que guardo para confirmar
      EEPROM.read(i);
      readbackup = readbackup + EEPROM.read(i);       //leer
    }
    unsigned long dfdfa = currentTimeStr.substring(0).toInt();
    Serial.print("saved: ");Serial.println(dfdfa);
    timebackup=0;
    readbackup="";
  }
  else
  {
    //Serial.println("timebackup=0 or currentTime is invalid");
    timebackup=0;
  }
  

}

void APmode()
{

  //disableCore0WDT();
  Serial.println("AP mode");
  Serial.println("Creating portal and trying to connect...");

  //Config.immediateStart = true;
  Config.autoReconnect = true;
  Config.hostName = "M3TR " + String(M3TRid);
  Config.portalTimeout = 60000;
  Config.apid = "M3TR " + String(M3TRid);
  //Config.apip = 192.168.0.1;
  Config.retainPortal = false; //testito
  Portal.config(Config);
  //esp_task_wdt_reset();

  // Establish a connection with an autoReconnect option.
  bool acEnable;
  acEnable = Portal.begin();

  if (acEnable)
  {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    Portal.handleClient();
  }

  if (!acEnable)
  {
    Serial.println("portal eeeeeennnnnnddddd");
    //WiFi.disconnect();
    delay(100);
    //esp_task_wdt_reset();
    //connectToWiFi(1);
    //WiFi.mode(WIFI_STA);
    //WiFi.begin();
    WiFi.reconnect();
    //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // WiFi.setHostname("M3TR");

    //portal.handleClient();
  }
} // end AP mode

void addtobasket() //checa que pueda subor datos a internet, si no entonces guarda en ram cada medicion hasta que pueda subir los datos pendientes
{
  Serial.print("addtobasket ");

  updatecurrentTime();

  if (caso == 0)
  {
    counterstatus++;
    if (counterstatus > 9999)
    {
      counterstatus = 0;
    }
  }

  if (flagonline == 0)
  {
    intcounterstatus = counterstatus;
    counterstatus = intcounterstatus;
    counterstatus = counterstatus + 0.1;
  }
  else
  {
    intcounterstatus = counterstatus;
    counterstatus = intcounterstatus;
  }



  o++;
  Serial.print("location 'o': ");
  Serial.println(o);

  reportacada = 230;
  multiplos = 30;
  if (o > 50)
  {
    reportacada = 15;
    multiplos = 120;
  } // cada  2 min con flujo y cada 0.5 horas sin flujo
  if (o > 100)
  {
    reportacada = 12;
    multiplos = 600;
  } // cada 10 min con flujo y cada 2 horas sin flujo
  if (o > 150)
  {
    reportacada = 16;
    multiplos = 900;
  } // cada 15 min con flujo y cada 4 horas sin flujo



  if (o == BACKUPSIZE - 1)
  {
    o = 1;
  }
  else
  {
    //  [0]          [1]       [2]         [3]       [4]     [5]               [6]
    //currenttime,  flowtemp, flowtotal,flowacum,   vbat,   counterstatus,    has data?

    currenttimearray[o] = currentTime;
    payloadbag[o][0] = flowtemp1;
    payloadbag[o][1] = flowtotal;
    payloadbag[o][2] = flowacum;
    payloadbag[o][3] = vbat;
    payloadbag[o][4] = counterstatus;
    payloadbag[o][5] = 1;

    //extras from lora sensors:
    payloadbag[o][6] = bufhotflow;        bufhotflow=0;
    payloadbag[o][7] = bufhottempIN;      bufhottempIN=-1;      //deberia dejarlo con la temp anterior para que no joda los promedios
    payloadbag[o][8] = bufhottempOUT;     bufhottempOUT=-1;       //voy a dejarlo un tiempo a ver como se comporta
    payloadbag[o][9] = bufsolarbat;       bufsolarbat=-1;
    payloadbag[o][10] = bufnivo1;         bufnivo1=-1;
    payloadbag[o][11] = bufnivo1temp;     bufnivo1temp=-1;
    payloadbag[o][12] = bufnivo1bat;      bufnivo1bat=-1;
    payloadbag[o][13] = bufsolarcount;    bufsolarcount=-1;
    payloadbag[o][14] = bufboilercount;   bufboilercount=-1;
    payloadbag[o][15] = bufnivocount;     bufnivocount=-1;
    
 
  //casos: 1=solar or boiler only, 2=boiler backup, 3=heater todo
  //M3TR2xxx,flujo=0, tempin, tempout, vbat, count
  //SOLAR or BOILER if ID is  "M3TR1xxx",  flow,   tempin=0, tempout,  vbat,  count
  //BOILER bkp if ID is       "M3TR2xxx",  flow=0, tempin,   tempout,  vbat,  count
  //HEATER TODO if ID is      "M3TR3xxx",  flow,   tempin,   tempout,  vbat,  count
  


    Serial.print("nr: ");
    Serial.print(o);
    Serial.println(" saved:  ");

    Serial.print("               currenttime: ");
    Serial.println(currentTime);
    Serial.print("               flowtemp M3TR: ");
    Serial.println(flowtemp1);
    Serial.print("               flowtotal: ");
    Serial.println(flowtotal);
    Serial.print("               flowacum: ");
    Serial.println(flowacum);
    Serial.print("               vbat: ");
    Serial.println(vbat);
    Serial.print("               counterstatus: ");
    Serial.println(counterstatus);
    Serial.print("               o: ");
    Serial.println(o);

    flowtotal = 0;
  } //end else
} //end addtobasket

void checkserial() //recibe un mensaje del LoRa receiver y confirma que el ID sea white, entrega validator=0(si no) o validator=2(si si)
{
  //caracteres invalidos para recibir: "*"
  validator = 0;
  //COMs
  //1 ping, link
  //2 unlink
  //3 anymode=true, link
  //4 set whitelist, link
  //5 clear whitelist, anymode, link
  while (atSerial.available())
  {

    char inChar = (char)atSerial.read();
    //byte inChar2=inChar -'0';
    //Serial.println(inChar2);
    //Serial.println("recibido");
    if (inChar == '?')
    {
      //Serial.print("M3TR_unique_id"); Serial.println(M3TR_unique_id); 
      int cien, diez, uno;
      atSerial.print("4");   //provide whitelist: se asigna un ID para solar y uno para nivo, el whiteID es el mismo que el ID del M3TR.
      if(M3TR_unique_id < 10){
        cien = 0;
        diez = 0;
        uno = M3TR_unique_id; 
      }
      if(M3TR_unique_id >= 10){
        cien = 0;
        diez = M3TR_unique_id/10;
        uno = M3TR_unique_id-diez*10; 
      }
      if(M3TR_unique_id >= 100){
        cien = M3TR_unique_id/100;//Serial.println(cien); 
        diez = (M3TR_unique_id-cien*100)/10;//Serial.println(diez); 
        uno = M3TR_unique_id-cien*100-diez*10;//Serial.println(uno); 
      }

      atSerial.print("1");    //solar
      atSerial.print(cien);   //solar
      atSerial.print(diez);
      atSerial.print(uno);

      atSerial.print("2");    //boiler bkp
      atSerial.print(cien);   //solar
      atSerial.print(diez);
      atSerial.print(uno);

      atSerial.print("3");    //heater all
      atSerial.print(cien);   //solar
      atSerial.print(diez);
      atSerial.print(uno);

      atSerial.print("4");    //nivo
      atSerial.print(cien);   //solar
      atSerial.print(diez);
      atSerial.print(uno);
      atSerial.print("@");   //finish 

      Serial.println("--- Lora receptor conectado ---");
    }
    if (inChar == '(') //incoming message
    {
      Serial.print("(");
      long readtimer = 0;
      long readtimerpre = millis();
      for (int i = 0; i < 40; i++)
      {
        bufserial[i] = 0;
      } //cleanup
      //receive and load message in buffer
      //while (inChar != ')' && readtimer < 1000)
      lengthpayload = 0;
      while (atSerial.available() && readtimer < 1000)
      {
        inChar = (char)atSerial.read();
        bufserial[lengthpayload] = inChar;
        lengthpayload++;
        Serial.print(inChar);
        if (lengthpayload > 40)
        {
          Serial.println("cadena demasiado larga");
          validator = 0;
          readtimerpre = 0; //exit
          lengthpayload = 0;
        }
        else
        {
          if (inChar == ')')
          {
            validator = 1;
            readtimerpre = 0;
            //Serial.println(")");
          } //exit
        }
        readtimer = millis() - readtimerpre;
      } //end while
      lengthpayload = 0;

      //SOLAR if ID is "M3TR1xxx",  flow,  temp,  vbat,  count
      //NIVO  if ID is "M3TR0xxx",  level, 0,     vbat,  count
      if (validator == 1)
      {
        Serial.print(" Validando ID prefix: ");
        validator = 0;
        if (bufserial[0] == 'M' && bufserial[1] == '3' && bufserial[2] == 'T' && bufserial[3] == 'R')
        {
          Serial.print(" OK, type: ");
          if (bufserial[4] == '1' ){Serial.print("1 (solar), whiteID: ");}
          if (bufserial[4] == '2' ){Serial.print("2 (boiler bkp), whiteID: ");}
          if (bufserial[4] == '3' ){Serial.print("3 (heater todo), whiteID: ");}
          if (bufserial[4] == '4' ){Serial.print("4 (nivo), whiteID: ");}
          int cien = bufserial[5]- '0'; //convert from chat to int
          int diez = bufserial[6]- '0';
          int uno = bufserial[7]- '0';
          int sensorID = cien * 100 + diez * 10 + uno;
          for (int i = 0; i <= whitelistlength; i++)
          {
            if (sensorID == whiteID[i])
            {
              validator = 2;
              Serial.print("OK: ");
              Serial.print(sensorID);
              i=whitelistlength+1;
              validator=2;
            }
          }
          if(validator==0){Serial.print(" invalid.");}
        }
      } //end if validator
    }   // end if '('
    else
    {
      Serial.print(inChar);
    }
  }     //end serial available
} //end serialcheck

void sensors()
{
  if (validator == 2)
  {
    Serial.println("LoRa payload analysis:");
    validator = 0;
    //nota: id es bufserial[5] * 100 + bufserial[6] * 10 + bufserial[7];
    //01234567
    //SOLAR if ID is "M3TR1xxx",flow,tempIN=0,tempOUT,vbat,count
    //               (M3TR1123 ,1234, 0      ,123    ,123, 1234,-12)      =26 char
    //BOILER BKP  if ID is "M3TR2xxx",flow=0,tempIN,tempOUT,vbat,count
    //          (M3TR2123 , 0  ,123 ,123 ,123,1234,-12)     =25 caracteres
    //HEATER TODO  if ID is "M3TR3xxx",flow,tempIN,tempOUT,vbat,count
    //          (M3TR3123 ,123  ,123 ,123 ,123,1234,-12)     =25 caracteres
    //NIVO  if ID is "M3TR4xxx",level,temp,vbat,count
    //          (M3TR2123 ,123  ,123 ,123 ,1234,-12)     =25 caracteres

      //float hotflowbkp = payloadbag[i][6];
      //float hottempbkpIN = payloadbag[i][7];
      //float hottempbkpOUT = payloadbag[i][8];
      //float solarbatbkp = payloadbag[i][9];
      //float nivo1bkp = payloadbag[i][10];
      //float nivo1tempbkp = payloadbag[i][11];
      //float nivo1batbkp = payloadbag[i][12];

    if (bufserial[4] == '1')  //solar or boiler only
    {
      //bufhotflow////////////////////////////////////////////////////
      bufhotflow = 0; byte x = 0;
      for (int i = 9; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1) { int uno = bufserial[9] - '0'; bufhotflow = uno;}
      if (x == 2) { int diez = bufserial[9] - '0'; int uno =  bufserial[10] - '0'; bufhotflow = diez * 10 + uno;}
      if (x == 3) { int cien = bufserial[9] - '0'; int diez = bufserial[10] - '0'; int uno = bufserial[11] - '0'; bufhotflow = cien * 100 + diez * 10 + uno;}
      if (x == 4) { int mil = bufserial[9] - '0';  int cien = bufserial[10] - '0'; int diez = bufserial[11] - '0'; int uno = bufserial[12] - '0'; bufhotflow = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhotflow=bufhotflow/100;      // 1234 -> 12.34
      Serial.print(" bufhotflow: ");
      Serial.print(bufhotflow);

      //bufhottemp IN////////////////////////////////////////////////////
      bufhottempIN = 0;
      byte i2 = x + 10;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempIN = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempIN = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempIN = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempIN = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempIN=bufhottempIN/10;  //564 -> 56.4
      Serial.print(" bufhottempIN: ");
      Serial.print(bufhottempIN);

      //bufhottemp OUT////////////////////////////////////////////////////
      bufhottempOUT = 0;
      byte i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempOUT = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempOUT = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempOUT = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempOUT = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempOUT=bufhottempOUT/10;  //564 -> 56.4
      Serial.print(" bufhottempOUT: ");
      Serial.print(bufhottempOUT);

      //bufsolarbat////////////////////////////////////////////////////
      bufsolarbat = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufsolarbat = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufsolarbat = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufsolarbat = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufsolarbat = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" bufsolarbat: ");
      Serial.print(bufsolarbat);

      //counter////////////////////////////////////////////////////
      unsigned int count = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';count = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';count = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';count = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';count = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufsolarcount=count;
      Serial.print(" countserial: ");
      Serial.print(bufsolarcount);


      //rssi////////////////////////////////////////////////////
      int rssi = 0;
      i3 = i2 + x + 2;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ')'){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';rssi = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';rssi = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';rssi = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';rssi = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" rssi: ");
      Serial.println(rssi);
      Serial.println("");

      //if bufhotflow <= 0 && bufhottempIN >= 0 && bufhottempIN < 99 && bufhottempOUT >= 0 && bufhottempOUT < 99 && bufsolarbat >= 0
      //validar y guardar
      if (bufhotflow >= 0 && bufhottempOUT >= -10 && bufhottempOUT < 99) //validar coherencia de datos
      {
        payloadbag[i][6] = bufhotflow;
        payloadbag[i][7] = bufhottempIN;
        payloadbag[i][8] = bufhottempOUT;
        payloadbag[i][9] = bufsolarbat;
        payloadbag[i][13] = bufsolarcount;
        caso=1;
        addtobasket();

      }

    } //end if bufserial4=1 (solar)

    if (bufserial[4] == '2')  //boiler bkp
    {
      //bufhotflow////////////////////////////////////////////////////
      bufhotflow = 0; byte x = 0;
      for (int i = 9; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1) { int uno = bufserial[9] - '0'; bufhotflow = uno;}
      if (x == 2) { int diez = bufserial[9] - '0'; int uno =  bufserial[10] - '0'; bufhotflow = diez * 10 + uno;}
      if (x == 3) { int cien = bufserial[9] - '0'; int diez = bufserial[10] - '0'; int uno = bufserial[11] - '0'; bufhotflow = cien * 100 + diez * 10 + uno;}
      if (x == 4) { int mil = bufserial[9] - '0';  int cien = bufserial[10] - '0'; int diez = bufserial[11] - '0'; int uno = bufserial[12] - '0'; bufhotflow = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhotflow=bufhotflow/100;      // 1234 -> 12.34
      Serial.print(" bufhotflow: ");
      Serial.print(bufhotflow);

      //bufhottemp IN////////////////////////////////////////////////////
      bufhottempIN = 0;
      byte i2 = x + 10;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempIN = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempIN = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempIN = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempIN = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempIN=bufhottempIN/10;  //564 -> 56.4
      Serial.print(" bufhottempIN: ");
      Serial.print(bufhottempIN);

      //bufhottemp OUT////////////////////////////////////////////////////
      bufhottempOUT = 0;
      byte i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempOUT = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempOUT = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempOUT = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempOUT = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempOUT=bufhottempOUT/10;  //564 -> 56.4
      Serial.print(" bufhottempOUT: ");
      Serial.print(bufhottempOUT);

      //bufsolarbat////////////////////////////////////////////////////
      bufsolarbat = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufsolarbat = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufsolarbat = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufsolarbat = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufsolarbat = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" bufsolarbat: ");
      Serial.print(bufsolarbat);

      //counter////////////////////////////////////////////////////
      unsigned int count = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';count = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';count = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';count = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';count = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufboilercount=count;
      Serial.print(" countboiler: ");
      Serial.print(bufboilercount);

      //rssi////////////////////////////////////////////////////
      int rssi = 0;
      i3 = i2 + x + 2;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ')'){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';rssi = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';rssi = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';rssi = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';rssi = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" rssi: ");
      Serial.println(rssi);
      Serial.println("");

      //validar y guardar
      if (bufhotflow == 0 && bufhottempIN >= 0 && bufhottempIN < 99 && bufhottempOUT >= 0 && bufhottempOUT < 99 && bufsolarbat >= 0) //validar coherencia de datos
      {
        payloadbag[i][6] = bufhotflow;
        payloadbag[i][7] = bufhottempIN;
        payloadbag[i][8] = bufhottempOUT;
        payloadbag[i][9] = bufsolarbat;
        payloadbag[i][14] = bufboilercount;
        caso=2;
        addtobasket();

      }

    } //end if bufserial4=1 (solar)

    if (bufserial[4] == '3')  //heater todo
    {
      //bufhotflow////////////////////////////////////////////////////
      bufhotflow = 0; byte x = 0;
      for (int i = 9; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1) { int uno = bufserial[9] - '0'; bufhotflow = uno;}
      if (x == 2) { int diez = bufserial[9] - '0'; int uno =  bufserial[10] - '0'; bufhotflow = diez * 10 + uno;}
      if (x == 3) { int cien = bufserial[9] - '0'; int diez = bufserial[10] - '0'; int uno = bufserial[11] - '0'; bufhotflow = cien * 100 + diez * 10 + uno;}
      if (x == 4) { int mil = bufserial[9] - '0';  int cien = bufserial[10] - '0'; int diez = bufserial[11] - '0'; int uno = bufserial[12] - '0'; bufhotflow = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhotflow=bufhotflow/100;      // 1234 -> 12.34
      Serial.print(" bufhotflow: ");
      Serial.print(bufhotflow);

      //bufhottemp IN////////////////////////////////////////////////////
      bufhottempIN = 0;
      byte i2 = x + 10;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempIN = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempIN = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempIN = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempIN = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempIN=bufhottempIN/10;  //564 -> 56.4
      Serial.print(" bufhottempIN: ");
      Serial.print(bufhottempIN);

      //bufhottemp OUT////////////////////////////////////////////////////
      bufhottempOUT = 0;
      byte i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ',') { i = 40; }
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufhottempOUT = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufhottempOUT = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufhottempOUT = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufhottempOUT = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufhottempOUT=bufhottempOUT/10;  //564 -> 56.4
      Serial.print(" bufhottempOUT: ");
      Serial.print(bufhottempOUT);

      //bufsolarbat////////////////////////////////////////////////////
      bufsolarbat = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufsolarbat = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufsolarbat = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufsolarbat = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufsolarbat = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" bufsolarbat: ");
      Serial.print(bufsolarbat);

      //counter////////////////////////////////////////////////////
      unsigned int count = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';count = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';count = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';count = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';count = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufsolarcount=count;
      Serial.print(" countheater: ");
      Serial.print(bufsolarcount);

      //rssi////////////////////////////////////////////////////
      int rssi = 0;
      i3 = i2 + x + 2;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ')'){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';rssi = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';rssi = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';rssi = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';rssi = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" rssi: ");
      Serial.println(rssi);
      Serial.println("");

      //validar y guardar
      if (bufhotflow >= 0 && bufhottempIN >= -10 && bufhottempIN < 99 && bufhottempOUT >= -10 && bufhottempOUT < 99 && bufsolarbat >= 0) //validar coherencia de datos
      {
        payloadbag[i][6] = bufhotflow;
        payloadbag[i][7] = bufhottempIN;
        payloadbag[i][8] = bufhottempOUT;
        payloadbag[i][9] = bufsolarbat;
        payloadbag[i][13] = bufsolarcount;
        caso=3;
        addtobasket();
      }

    } //end if bufserial4=1 (solar)

    if (bufserial[4] == '4')  //nivo
    {
      //bufnivo1:
      bufnivo1 = 0;
      byte x = 0;
      for (int i = 9; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[9] - '0';bufnivo1 = uno;}
      if (x == 2){int diez = bufserial[9] - '0';int uno = bufserial[10] - '0';bufnivo1 = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[9] - '0';int diez = bufserial[10] - '0';int uno = bufserial[11] - '0';bufnivo1 = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[9] - '0';int cien = bufserial[10] - '0';int diez = bufserial[11] - '0';int uno = bufserial[12] - '0';bufnivo1 = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" bufnivo1: ");
      Serial.print(bufnivo1);

      //bufnivo1temp
      bufnivo1temp = 0;
      byte i2 = x + 10;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}// checar que 30 sean suficientes y no me falten mas
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufnivo1temp = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufnivo1temp = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';bufnivo1temp = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufnivo1temp = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufnivo1temp=bufnivo1temp/10;
      Serial.print(" bufnivo1temp: ");
      Serial.print(bufnivo1temp);

      //bufnivo1bat
      bufnivo1bat = 0;
      byte i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';bufnivo1bat = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';bufnivo1bat = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0'; bufnivo1bat = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';bufnivo1bat = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" bufnivo1bat: ");
      Serial.print(bufnivo1bat);

      //counter
      unsigned int count = 0;
      i3 = i2 + x + 1;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ','){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';count = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';count = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';count = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';count = mil * 1000 + cien * 100 + diez * 10 + uno;}
      bufnivocount=count;
      Serial.print(" countnivo: ");
      Serial.print(bufnivocount);

      //rssi
      int rssi = 0;
      i3 = i2 + x + 2;
      i2 = i3;
      x = 0;
      for (int i = i2; i < 40; i++)
      {
        if (bufserial[i] == ')'){i = 40;}
        x++;
      }
      x--; //para corregir la cuenta de x
      if (x == 1){int uno = bufserial[i2] - '0';rssi = uno;}
      if (x == 2){int diez = bufserial[i2] - '0';int uno = bufserial[i2 + 1] - '0';rssi = diez * 10 + uno;}
      if (x == 3){int cien = bufserial[i2] - '0';int diez = bufserial[i2 + 1] - '0';int uno = bufserial[i2 + 2] - '0';rssi = cien * 100 + diez * 10 + uno;}
      if (x == 4){int mil = bufserial[i2] - '0';int cien = bufserial[i2 + 1] - '0';int diez = bufserial[i2 + 2] - '0';int uno = bufserial[i2 + 3] - '0';rssi = mil * 1000 + cien * 100 + diez * 10 + uno;}
      Serial.print(" rssi: ");
      Serial.println(rssi);
      Serial.println("");
      //filtro de coherencia de datos (que no sean negativos o fuera de rango)

      if (bufnivo1 >= 0 && bufnivo1temp >= -10 && bufnivo1temp < 99 && bufnivo1bat >= 0) //validar coherencia de datos
      {
        payloadbag[i][10] = bufnivo1;
        payloadbag[i][11] = bufnivo1temp;
        payloadbag[i][12] = bufnivo1bat;
        payloadbag[i][15] = bufnivocount;
        caso=4;
        addtobasket();
      }
    }
  } //end if validator==2
} //end sensors

void connectToWiFi(int x)
{
  //  esp_task_wdt_reset();
  //Serial.println("Connect to Wifi");
  if (x == 1)
  {
    zeit2 = 0;
    x = 0;
  }


  if (WiFi.status() == WL_CONNECTED && millis() > zeit2)
  {
    Serial.print("pinging.. ");
    //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts
    //esp_task_wdt_add(NULL); //add current thread to WDT watch
    if (Ping.ping("www.google.com", 2) == 1) //bool ret = Ping.ping("www.google.com",10); //repeticiones
    {
      //esp_task_wdt_disable();
      // disableCore0WDT();
      avg_time_ms = Ping.averageTime();
      Serial.print(avg_time_ms);
      zeit2 = 10000 + millis();
      if (avg_time_ms > 1000)
      {
        digitalWrite(ledred, HIGH);
        digitalWrite(ledgreen, LOW);
        flagonline = 0;
      }
      else
      {
        digitalWrite(ledred, LOW);
        digitalWrite(ledgreen, HIGH);

        if (flagonline == 0)
        {
          // publishnow=0;//si antes estaba offline publica de una vez
        }
        flagonline = 1;
        Serial.print("updatecurrentTime.. ");
        updatecurrentTime();
      }
    }
    else
    {
      // disableCore0WDT();
      Serial.print("no pong");
      flagonline = 0;
      digitalWrite(ledred, HIGH);
      digitalWrite(ledgreen, LOW);
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    //WiFi.mode( WIFI_MODE_NULL );
    //////WiFi.disconnect();
    delay(100);
    Serial.println("no wifi ");
    digitalWrite(ledred, HIGH);
    digitalWrite(ledgreen, LOW);
    flagonline = 0;
    Serial.print("[INFO]: Start setup to internet connection ");
    //WiFi.setHostname("M3TR");
    //////WiFi.mode(WIFI_STA);

    //////WiFi.begin();
    //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Only try 30 times to connect to the WiFi

    int retries = 5;
    while (WiFi.status() != WL_CONNECTED && retries > 1)
    {
      Serial.print(" m3tr wifi reconnecting...");
      WiFi.begin(WIFI_SSID_m3tr, WIFI_PASSWORD2);
      WiFi.reconnect();
      delay(1000);
      Serial.print(retries);
      retries--;
      checkserial();
      sensors(); 
    }

    retries = 25;
    while (WiFi.status() != WL_CONNECTED && retries > 1)
    {
      //WiFi.disconnect();
      Serial.print(" wifi reconnecting...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      WiFi.reconnect();
      delay(1000);
      Serial.print(retries);
      retries--;
      checkserial();
      sensors(); 
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      flagonline = 0;
      Serial.println("[ERROR]: Could not connect to WiFi");
      APmode();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      //giveup=20;
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
      // bool ret = Ping.ping("www.google.com", 10);
      //float avg_time_ms = Ping.averageTime();
      if (avg_time_ms < 1000)
      {
        Serial.print("ping: ");
        Serial.println(avg_time_ms);
        digitalWrite(ledred, LOW);
        digitalWrite(ledgreen, HIGH);
        Serial.println();
        Serial.print("[INFO]: Connected to internet. IP: ");
        Serial.print(WiFi.localIP());
        Serial.print(", SSID: ");
        Serial.print(WiFi.SSID());
        Serial.print(", RSSI: ");
        Serial.print(WiFi.RSSI());

        flagonline = 1;
      }
    }
  }

  timeupdate++;
  if (timeupdate > 60)
  {
    Serial.println("updatecurrentTime");
    updatecurrentTime();
    timeupdate = 0;
  }
}//end connectowifi

void buttoncheck() //if pushbutton pressed for more than x times delay(millis), then clear wifi credentials and launch softAP
{
  int d = 0;
  int led = 1;
  if (digitalRead(bot) == LOW)
  { //if1
    buttonflag = 1;
    Serial.println("button ");
    while (d < 30) //x times
    {
      //esp_task_wdt_reset();
      if (digitalRead(bot) == LOW)
      {
        Serial.print("! ");
        if (led == 1)
        {
          digitalWrite(ledred, HIGH);
          digitalWrite(ledgreen, LOW);
          led = 0;
        }
        else
        {
          digitalWrite(ledred, LOW);
          digitalWrite(ledgreen, LOW);
          led = 1;
        }
        delay(50); //delay(millis)
        d++;
      }
      else
      {
        buttonflag = 0;
        d = 51;
        Serial.print(" clear button ");
      }
    } //end while

    if (buttonflag == 0)
    {
      Serial.println(" returning..");
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, HIGH);
      publishnow = 0; //if publishnow==1 then publish normally, if publishnow==0 then publish now.
                      // attachInterrupt(bot,interruptbutton, FALLING);
      return;
    }

    if (buttonflag == 1) //if button still pressed blink faster
    {
      d = 0;
      while (d < 100) //x times
      {
        //esp_task_wdt_reset();
        if (digitalRead(bot) == LOW)
        {
          Serial.print("! ");
          if (led == 1)
          {
            digitalWrite(ledred, HIGH);
            digitalWrite(ledgreen, LOW);
            led = 0;
          }
          else
          {
            digitalWrite(ledred, LOW);
            digitalWrite(ledgreen, LOW);
            led = 1;
          }
          delay(20); //delay(millis)
          d++;
        }

        else
        {
          buttonflag = 0;
          d = 100;
          Serial.print(" clear button ");
        }
      } //END WHILE
    }

    if (buttonflag == 0)
    {
      Serial.print(" flowacum reset to cero,"); //if released go back but clear flowacum
      flowacum = 0;
      Serial.println(" returning..");
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, HIGH);
      publishnow = 0; //if publishnow==1 then publish normally, if publishnow==0 then publish now.
                      // attachInterrupt(bot,interruptbutton, FALLING);
      return;
    }
    if (buttonflag == 1)
    { //if still pressed, clear wifi credentials
      digitalWrite(ledred, LOW);
      digitalWrite(ledgreen, LOW);
      delay(1000);
      //detachInterrupt(bot);
      //resetWifi();
    }
  }
}//end buttoncheck

void vbatcheck(){
    digitalWrite(railEnable, HIGH); //LOW=ON
    delay(20);
    vbat=analogRead(lipocheck);
    vbat=vbat*4/2267;           //from raw to volts
    
    if(flagnohayluz==0){
        if(vbat<=4.2){publishnow=0; flagnohayluz=1;Serial.print(" no hay luz");} 
    }
    if(flagnohayluz==1){
        if(vbat>=4.3){publishnow=0; flagnohayluz=0;}
    }

    if (vbat < 3.65)
    {
      digitalWrite(railEnable,HIGH);  //LOW=ON
    }
    if (vbat > 3.7)
    {
      digitalWrite(railEnable,LOW);  //LOW=ON
    }
    
    Serial.print(" vbat: ");Serial.print(vbat);
}

void tempread(){
  
  sensor.requestTemperatures();
  Serial.print("TEMPREAD is:");
  flowtemp1 = sensor.getTempC();
  Serial.println(flowtemp1); 
  if (flowtemp1<0){flowtemp1=0;Serial.print(" tempread error (value<0), replacing with 0");}
}




void sendData(String params)
{
  Serial.println("sendData sending...");
  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;

  digitalWrite(ledgreen,LOW);
  digitalWrite(ledred,LOW);

  Serial.print(url);
  Serial.print("Making a request");
  http.begin(url, root_ca); //Specify the URL and certificate
  Serial.print("http.get:");
  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer 
  //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
  httpCode = http.GET();
  Serial.print (" DONE");
  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //disableCore0WDT();
  http.end();
  Serial.print(" with reply: ");
  Serial.println (httpCode);
  
  
  // if (httpCode == 302 || httpCode == 200)
  // {
  //  // flowtotal = 0; //reset flowshort (flowtotal) para que pueda ir contando nuevamente mientras google responde
  // }
  // else
  // {
  // Serial.print(url);
  // Serial.print("sending same message agaiin...");
  // http.begin(url, root_ca); //Specify the URL and certificate
  // Serial.print("http.get:");
  // //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  // //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer 
  // //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
  // httpCode = http.GET();
  // Serial.println("A VER");
  // //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  // //disableCore0WDT();
  // http.end();
  // }
  
  
  if (httpCode == 302 || httpCode == 200 || httpCode == -11)
  {
      digitalWrite(ledgreen,HIGH);
      digitalWrite(ledred,LOW);
   // flowtotal = 0; //reset flowshort (flowtotal) para que pueda ir contando nuevamente mientras google responde
  }
  else
  {
    //flagonline=0;
    //counterstatus--;
   // onlinecheck3();
  Serial.println("sendData sending again...");
  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;

  Serial.print(url);
  Serial.print("Making a request");
  http.begin(url, root_ca); //Specify the URL and certificate
  Serial.print("http.get:");
  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts}
  //hacer un timer manual, si pasan mas de unos 20 seg haz backup de currenttimer 
  //esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
  httpCode = http.GET();
  Serial.print (" DONE");
  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //disableCore0WDT();
  http.end();
  Serial.print(" with reply: ");
  Serial.println (httpCode);
  }

  if (httpCode == 302 || httpCode == 200 || httpCode == -11)
  {
    digitalWrite(ledgreen,HIGH);
    digitalWrite(ledred,LOW);
    // flowtotal = 0; //reset flowshort (flowtotal) para que pueda ir contando nuevamente mientras google responde
  }
  else
  {
    digitalWrite(ledgreen,LOW);
    digitalWrite(ledred,HIGH);
    flagonline = 0;
    //counterstatus--;
   //onlinecheck3();
  }

  flowtotal = 0;
}//end senddata

void publicadorm3tr()
{
  Serial.println(" publicador");
  digitalWrite(ledred, HIGH);
  digitalWrite(ledgreen, LOW);
  updatecurrentTime(); //timestamp en formato epoch para enviar en el paquete
  addtobasket();


  if (publishconfig == 1)
  { 
    // pubSubPayload = String::format("%d,%d,%.2f,%.2f,%.2f,%.2f", config, currentTime, flowtotal , flowtemp1, flowacum , vbat);
    Serial.print("payload: ");
    Serial.print("config: ");
    Serial.print(publishconfig);
    Serial.print(" flowtotal: ");
    Serial.print(flowtotal);
    Serial.print(" temp1: ");
    Serial.print(flowtemp1);
    Serial.print(" vbat: ");
    Serial.print(vbat);
    Serial.print(" ");
  }
    flagsend=1;
} //end publicador

void empaquetador(){       //milis          //promedia las mediciones de cada periodo y publica
    if (Particle.connected()) {digitalWrite(ledred,LOW); digitalWrite(ledgreen,HIGH);}  
    if (Particle.disconnected()){ digitalWrite(ledred,HIGH); digitalWrite(ledgreen,LOW); }                     //red
    if(initialmessages==0){flaginit=1;}                     //publica varias mediciones inicialmente para confirmar llegada de datos a la nube
    else{flaginit=0; initialmessages--;delay(10000);I1a=I2a=I3a=I4a=I5a=Wh1=Wh2=Wh3=Wh4=Wh5=0;}
    
    int periodo = 10000;//antes 9985
    
    if((zeit-prezeit) >     1000 * processflag)     { processflag++; Particle.process(); Serial.print("."); }       
    if((zeit-prezeit) >      periodo   *  2  * publishnow * onetimer)     { onetimer++; wificheck(); }        //cada minuto checa wifi
    
    
    
    if((zeit-prezeit) >      periodo   *   reportacada * flaginit * publishnow)     {  //cada x milisegundos promedia las mediciones que ha estado agregando a addup() para promediar
        
        wificheck();
        
        
        if (variabilidad == 1){wificheck();}
       
        processflag=1;
        onetimer=1;
        average();              //promedia todas las lecturas realizadas durante el periodo
        
        float temp1 = fabs(V1pre-Vrms1);
        float temp2 = fabs(I1pre-I1a);
        
        float temp3 = fabs(V2pre-Vrms2);
        float temp4 = fabs(I2pre-I2a);
        
        float temp5 = fabs(V3pre-Vrms3);
        float temp6 = fabs(I3pre-I3a);
        
        float temp7 = fabs(I4pre-I4a);
        
        float temp8 = fabs(I5pre-I5a);
        
        
        if(publishnow==0){
            Wh1 = Wh1 + Vrms1 * I1a * (zeit-prezeit)/1000/3600  ;
            Wh2 = Wh2 + Vrms2 * I2a * (zeit-prezeit)/1000/3600  ;
            Wh3 = Wh3 + Vrms3 * I3a * (zeit-prezeit)/1000/3600  ;
            Wh4 = Wh4 + Vrms1 * I4a * (zeit-prezeit)/1000/3600  ;
            Wh5 = Wh5 + Vrms2 * I5a * (zeit-prezeit)/1000/3600  ;
            publishnow=1;flagpub=2;
            }
            else{
                Wh1 = Wh1 + Vrms1 * I1a * periodo/1000/3600  *reportacada;
                Wh2 = Wh2 + Vrms2 * I2a * periodo/1000/3600  *reportacada;
                Wh3 = Wh3 + Vrms3 * I3a * periodo/1000/3600  *reportacada;
                Wh4 = Wh4 + Vrms1 * I4a * periodo/1000/3600  *reportacada;
                Wh5 = Wh5 + Vrms2 * I5a * periodo/1000/3600  *reportacada;
            }
        
        //if(config == 12){           Wh4 = Wh4 + Vrms1 * I4a * periodo/1000/3600 *reportacada;  } 
        //else if(config == 13){      Wh4 = Wh4 + Vrms2 * I4a * periodo/1000/3600 *reportacada;  } 
        //else if(config == 14){      Wh4 = Wh4 + Vrms3 * I4a * periodo/1000/3600 *reportacada;  } 
       // else if(config == 15){      Wh4 = Wh4 + Vrms1 * I4a * periodo/1000/3600 *reportacada; Wh5 = Wh5 + Vrms2 * I5a * periodo/1000/3600 *reportacada;  } 
        
        //Wh4 = Wh4 + Vrms? * Irms4 * 9985/1000/3600;
 
        if (flagpub!=2){flagpub=0;}//solo en caso de initialmessages (publishnow=0)
        if(variabilidad==1){
            byte add=0;
            
            if      (temp1 > 3.0) {flagpub++; }             // Volts de variación
            else if (temp2 > deltai) {flagpub++; digitalWrite(ledgreen,LOW);}      // Amps de variación
            else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas
        
            if      (temp3 > 3.0) {flagpub++; }             // Volts de variación
            else if (temp4 > deltai) {flagpub++; digitalWrite(ledgreen,LOW);}      // Amps de variación
            else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas
        
            if      (temp5 > 3.0) {flagpub++; }             // Volts de variación
            else if (temp6 > deltai) {flagpub++; }      // Amps de variación
            else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas
        
            if      (temp7 > deltai) {flagpub++; }             // Volts de variación
            else if (temp8 > deltai) {flagpub++; }      // Amps de variación
            else     {add=1;}                               // si pasan mas x veces sin reportar cambios, manda medicion de todas formas
        
            if(add==1){flagpubcount++;}
            if( flagpubcount > maxwait) {flagpub=1; flagpubcount=0; }
        }//end if variablilidad
        else {
            publicador(); flagpubcount=0; Wh1=Wh2=Wh3=Wh4=Wh5=0; 
        }
        
        if (variabilidad == 1){
            if (flagpub >= 1 || flaginit==0)   
            {   
                V1pre=Vrms1; I1pre=I1a; 
                V2pre=Vrms2; I2pre=I2a;
                V3pre=Vrms3; I3pre=I3a;
                I4pre=I4a;
                I5pre=I5a;
                flagpub=0;
                publicador(); flagpubcount=0; Wh1=Wh2=Wh3=Wh4=Wh5=0; 
            }
        }

        Vrmsprom1 = Irmsprom1 = Powerprom1 =  0;
        Vrmsprom2 = Irmsprom2 = Powerprom2 =  0;
        Vrmsprom3 = Irmsprom3 = Powerprom3 =  0;
                    Irmsprom4 = Powerprom4 =  0;       //reset variables
                    Irmsprom5 = Powerprom5 =  0;
        prezeit=zeit;    //reset tiempo de 1 seg
    }//end if
}//end empaquetador

void empaquetadorm3tr(){       //milis          //promedia las mediciones de cada periodo y publica
  Serial.print("empaquetador ");
  int flaginit=0;

  // if (WiFi.status() == WL_CONNECTED)
  // {
  //   // bool ret = Ping.ping("www.google.com", 10);
  //   //float avg_time_ms = Ping.averageTime();
  //   if (avg_time_ms < 1000)
  //   {
  //     Serial.print("Wifi OK, ");
  //     digitalWrite(ledred, LOW);
  //     digitalWrite(ledgreen, HIGH);
  //     flagonline=1;
  //   }
  // }

  // else
  // {
  //   Serial.print("Wif NOT ready, ");
  //   digitalWrite(ledred, HIGH);
  //   digitalWrite(ledgreen, LOW);

  //   flagonline = 0;
  // } //WiFi.connect();}

  // //if(flagonline==0){
  // //  publishnow=0;
  // //  flagonline=1;
  // //}

  if(initialmessages==0){flaginit=1;}                     //publica varias mediciones inicialmente para confirmar llegada de datos a la nube
  else{flaginit=0; initialmessages--;delay(1000);}
      
  flowtotal = flowtotal + flow/1000;
  flowacum = flowacum+flowtotal;

  //old if(flow>=0.1){publishnow=0;}
  if(flow>=0.1 || bufhotflow>=0.1 || bufnivo1 > 0.0 ){publishnow=0;}

    
  Serial.print (" flowtotal: ");Serial.print(flowtotal); Serial.print (" flowacum: ");Serial.print(flowacum);
  long periodo = multiplos*1000;
  if((zeit-prezeit) >      periodo   *   reportacada * flaginit * publishnow)     { 

//https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover


    //old if((flowtotal >= 0.0) || flagpubcount > maxwait ) { flagpub=1; } 
    if((flowtotal >= 0.0) || flagpubcount > maxwait ) { flagpub=1; }    
    else {
      if(publishnow==0){ flagpub=1; }
      else {flagpub=0; flagpubcount++;}
    }
        
    if (flagpub == 1) {
      vbatcheck();
      tempread();
      //en vez de publicador() voy a mandar el dato a una cubeta como si siempre estuviera offline. el otro core va a estar sacando de la cubeta cuando hay ainternet y si no se espera.
      publicador(); flagpubcount=0; //antes estaba aqui flowtotal=0; pero lo movi adespues de sendData
    } 
    publishnow=1; 
    prezeit=zeit;
  }//end if
  
  Serial.println(" end empaquetador ");
}//end empaquetador

void area(){
    //en lugar de obtener unicamente max y min, se calcula el area bajo las curvas de corriente ya que no siempre siguen una forma senoidal y pueden generar resultados erroneos.
    iarea1=iarea2=iarea3=iarea4=iarea5=0;
    
    for(unsigned int m=0;m<resolution;m++){
     long istep=0;
     
     //ojo para reducir "ruido" de corrientes pequenas, probar quitando la resistencia de 1M ohm entre el pin analigico del photon que mide las corrientes y tambien probar aumentar el  if (istep<2){istep=0;} a  istep< 3 o 4
     
     istep=u[m]-2048; istep=abs(istep);  if (istep<atenuacion){istep=0;} iarea1=iarea1+istep;
     istep=v[m]-2048; istep=abs(istep);  if (istep<atenuacion){istep=0;} iarea2=iarea2+istep;
     istep=w[m]-2048; istep=abs(istep);  if (istep<atenuacion){istep=0;} iarea3=iarea3+istep;
     istep=t[m]-2048; istep=abs(istep);  if (istep<atenuacion){istep=0;} iarea4=iarea4+istep;
     istep=s[m]-2048; istep=abs(istep);  if (istep<atenuacion){istep=0;} iarea5=iarea5+istep;
    }
}//end area

void sync1(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-1-");
    byte dowhile = 0;
    byte go=0;
    syncstatus1=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go1");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A0);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus1=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A0);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus1=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!1");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync1
void sync2(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-2-");
    byte dowhile = 0;
    byte go=0;
    syncstatus2=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go2");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A1);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus2=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A1);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus2=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!2");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync2
void sync3(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-3-");
    byte dowhile = 0;
    byte go=0;
    syncstatus3=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go3");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A2);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus3=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A2);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus3=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!3");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync3
void sync4(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-3-");
    byte dowhile = 0;
    byte go=0;
    syncstatus4=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go3");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A0);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus4=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A0);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus4=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!3");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync3
void sync5(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-3-");
    byte dowhile = 0;
    byte go=0;
    syncstatus5=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go3");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A1);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus5=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A1);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus5=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!3");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync3
void sync6(){               //este syn sirve muy bien, solo falta ajustarlo para que empiece mas cerca de 2048 o bien hacer el analisis de power factor buscando el valor de 2048 y a partir de aho hacer el conteo de cursor.
    //Serial.println("-3-");
    byte dowhile = 0;
    byte go=0;
    syncstatus6=0;   
    prezeitsync=micros();
    do
    {
        //Serial.println("go3");
        while(go==0){
            int ramp=0;
            while(ramp<100){
                //Serial.println(ramp);
                analogread = analogRead(A2);
                if(analogread>1800 && analogread<2000){ramp=101;}
                else {ramp++;}
                
                zeitsync=micros();
                if((zeitsync-prezeitsync) > 30000) { dowhile=1; ramp=102; go=1; syncstatus6=2;}//syncstatus=2 significa que no pudo sincronizarse
            }
            
            if(ramp==101){
                int temporal = analogRead(A2);    
                if(temporal>analogread){
                    go=1;
                    dowhile=1;
                    syncstatus6=1;              //syncstatus=1 significa que si pudo sincronizarse
                    //Serial.println("!3");
                }
                else 
                {
                    delay(5);    
                }
            }//END IF
        }//end while ramp
    } while(dowhile == 0);
}//end sync3

void maxmin(){
    vmax1=vmax2=vmax3=0; 
    vmax4=vmax5=vmax6=0; 
    imax1=imax2=imax3=imax4=imax5=0; 
   
    vmin1=vmin2=vmin3=5000;
    vmin4=vmin5=vmin6=5000;
    imin1=imin2=imin3=imin4=imin5=5000;
    
    //ojo, ya no se necesita calcular max/min de las corrientes porque ahora se hace con area...
   
    
    for(unsigned int m=0;m<resolution;m++){
      vmax1=max(x[m],vmax1);
      vmin1=min(x[m],vmin1);
      vmax2=max(y[m],vmax2);
      vmin2=min(y[m],vmin2);
      vmax3=max(z[m],vmax3);
      vmin3=min(z[m],vmin3);
      
      vmax4=max(xx[m],vmax4);
      vmin4=min(xx[m],vmin4);
      vmax5=max(yy[m],vmax5);
      vmin5=min(yy[m],vmin5);
      vmax6=max(zz[m],vmax6);
      vmin6=min(zz[m],vmin6);
      
      imax1=max(u[m],imax1);    
      imax2=max(v[m],imax2);    
      imax3=max(w[m],imax3);   
      imax4=max(t[m],imax4);    
      imax5=max(s[m],imax5);    
      
      imin1=min(u[m],imin1);    
      imin2=min(v[m],imin2);    
      imin3=min(w[m],imin3);   
      imin4=min(t[m],imin4);   
      imin5=min(s[m],imin5);   
    }
}//end maxmin1

void powerfactor(){
   // Serial.println("ppowerfactor");
    int pfable1=0;
    int pfable2=0;
    int pfable3=0;
    int pfable4=0;
    int pfable5=0;
    int m=0;
    int n=0;
    cursorv1=cursori1=cursorv2=cursori2=cursorv3=cursori3=cursori4=cursori5=cursorv1_=cursorv2_=cursorv3_=0;
    //pf1=pf2=pf3=pf4=pf5=1.01;

    // if(syncstatus1==1 && vmax1 > 2200 && vmin1 < 1900 && imax1 > 2055  &&  imin1 < 2040) { pfable1=1; }       //si hay suficiente amplitud en las curvas y sync funciono bien
    // if(syncstatus2==1 && vmax2 > 2200 && vmin2 < 1900 && imax2 > 2055  &&  imin2 < 2040) { pfable2=1; }
    // if(syncstatus3==1 && vmax3 > 2200 && vmin3 < 1900 && imax3 > 2055  &&  imin3 < 2040) { pfable3=1; }
    // if(syncstatus4==1 && vmax4 > 2200 && vmin4 < 1900 && imax4 > 2055  &&  imin4 < 2040) { pfable4=1; }         //config=15
    // if(syncstatus5==1 && vmax5 > 2200 && vmin5 < 1900 && imax5 > 2055  &&  imin5 < 2040) { pfable5=1; }         //config=15
    
    if(syncstatus1==1)  { pfable1=1; }       //si hay suficiente amplitud en las curvas y sync funciono bien
    if(syncstatus2==1)  { pfable2=1; }
    if(syncstatus3==1)  { pfable3=1; }
    if(syncstatus4==1)  { pfable4=1; }         //config=15
    if(syncstatus5==1)  { pfable5=1; }         //config=15
    //if(syncstatus6==1 && vmax6 > 2200 && vmin6 < 1900 && imax5 > 2055  &&  imin5 < 2040) { pfable6=1; }   //buga no se si sea imax 5 y imin5
    
    
     //para calcular power factor:
    if(pfable1==1){
        m=0; cursorv1=0;
        while(x[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
        while(x[m]>2048){ cursorv1++; m++; }            //cuenta las mediciones que sean mayores a 2048, deberian ser aprox 47 o resolution/2
       // Serial.print(" cursorv1: ");Serial.print(cursorv1);Serial.print(" m= ");Serial.println(m);
        n=0; cursori1=0;
        while(u[n]<2048){ n++; }                        //descarta valores iniciales menores a 2048
        while(u[n]>2048){ cursori1++; n++; }
        //Serial.print("cursori1: ");Serial.print(cursori1);Serial.print(" n= ");Serial.println(n);
      
        // if(m>40 && m<60 && abs(m-n)<15){
        //     float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
        //     pf1 = cos(pfactor);Serial.print("pf1: ");Serial.print(pf1);
        // }
        
       // if(m>40 && m<60){
            float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pf1 = cos(pfactor);Serial.print("pf1: ");Serial.print(pf1);
       // }
        
        
 

        //debugging power factor: temporal
         Serial.println("");
        for(int i=0;i<resolution;i++)
        {
            Serial.print(x[i]);Serial.print(",");
        }
        Serial.println("");
        
        for(int i=0;i<resolution;i++)
        {
            Serial.print(u[i]);Serial.print(",");
        }
        Serial.println("");
        
    }
    //ojo, falta decidir que mostrar si pfableX==0
    
    
    //voltaje 2 y corriente 2
    if(pfable2==1){
        m=0;
        while(y[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
     //   Serial.print("m= ");Serial.println(m); 
        while(y[m]>2048){ cursorv2++; m++; }            //cuenta las mediciones que sean mayores a 2048, deberian ser aprox 47 o resolution/2
        n=0;
        while(v[n]<2048){ n++; }                        //descarta valores iniciales menores a 2048
      //  Serial.print("m= ");Serial.println(m); 
        while(v[n]>2048){ cursori2++; n++; }            //cuenta las mediciones que sean mayores a 2048, dependiendo del desfase sera mayor o menor a 47
        // if(m>40 && m<60 && abs(m-n)<15){
        //     float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
        //     pf2 = cos(pfactor);Serial.print(" pf2: ");Serial.print(pf2);
        // }
        
        
        if(m>40 && m<60 ){
            float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pf2 = cos(pfactor);Serial.print(" pf2: ");Serial.print(pf2);
        }
        
    }


    //voltaje 3 y corriente 3
    if(pfable3==1){
        m=0;
        while(z[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
     //   Serial.print("m= ");Serial.println(m); 
        while(z[m]>2048){ cursorv3++; m++; }            //cuenta las mediciones que sean mayores a 2048, deberian ser aprox 47 o resolution/2
        n=0;
        while(w[n]<2048){ n++; }                        //descarta valores iniciales menores a 2048
     //   Serial.print("m= ");Serial.println(m); 
        while(w[n]>2048){ cursori3++; n++; }            //cuenta las mediciones que sean mayores a 2048, dependiendo del desfase sera mayor o menor a 47
        // if((m-n)<15){
        //     float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
        //     pf3 = cos(pfactor);Serial.print(" pf3: ");Serial.print(pf3);
        // }
        
            
            float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
            pf3 = cos(pfactor);Serial.print(" pf3: ");Serial.print(pf3);
        
    }




    if(config==15){
        if(pfable4==1){
            m=0;
            while(xx[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
         //   Serial.print("m= ");Serial.println(m); 
            while(xx[m]>2048){ cursorv1_++; m++; }    
            n=0;
            while(t[n]<2048){ n++; }          // t o s?             //descarta valores iniciales menores a 2048
          //  Serial.print("m= ");Serial.println(m); 
            while(t[n]>2048){ cursori4++; n++; } 
            if((m-n)<15){
                float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
                pf4 = cos(pfactor);Serial.print(" pf4: ");Serial.print(pf4);
            }
        }

        
        if(pfable5==1){
            m=0;
            while(yy[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
         //   Serial.print("m= ");Serial.println(m); 
            while(yy[m]>2048){ cursorv2_++; m++; }    
            n=0;
            while(s[n]<2048){ n++; }          // t o s?             //descarta valores iniciales menores a 2048
          //  Serial.print("m= ");Serial.println(m); 
            while(s[n]>2048){ cursori5++; n++; } 
            if((m-n)<15){
                float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
                pf5 = cos(pfactor);Serial.print(" pf5: ");Serial.println(pf5);
            }
        }
    }
    
    
    
    if(config==17){
        if(pfable4==1){
            m=0;
            while(yy[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
         //   Serial.print("m= ");Serial.println(m); 
            while(yy[m]>2048){ cursorv2_++; m++; }    
            n=0;
            while(t[n]<2048){ n++; }          // t o s?             //descarta valores iniciales menores a 2048
          //  Serial.print("m= ");Serial.println(m); 
            while(t[n]>2048){ cursori4++; n++; } 
            if((m-n)<15){
                float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
                pf4 = cos(pfactor);Serial.print(" pf4: ");Serial.print(pf4);
            }
        }

        
        if(pfable5==1){
            m=0;
            while(zz[m]<2048){ m++; }                        //descarta valores iniciales menores a 2048
         //   Serial.print("m= ");Serial.println(m); 
            while(zz[m]>2048){ cursorv3_++; m++; }    
            n=0;
            while(s[n]<2048){ n++; }          // t o s?             //descarta valores iniciales menores a 2048
          //  Serial.print("m= ");Serial.println(m); 
            while(s[n]>2048){ cursori5++; n++; } 
            if((m-n)<15){
                float pfactor= 3.1416/47*(m-n-1);//47 es aprox la mitad de resolution = 94, n-1 es para ajustar por que la medicion de corriente es intercalada con la de voltaje.
                pf5 = cos(pfactor);Serial.print(" pf5: ");Serial.println(pf5);
            }
        }
    }
    
    
    
}//end powerfactor

void readcycle(){
    

    
    
    
    // pinMode(A3, INPUT_PULLDOWN);                 //I5
    // pinMode(A4, INPUT_PULLDOWN);                 //I1 antes 4
    // pinMode(A5, INPUT_PULLDOWN);                 //I2 antes 3
    // pinMode(A6, INPUT_PULLDOWN);                 //I3 antes 2
    // pinMode(A7, INPUT_PULLDOWN);                 //I4 antes 1
    
    
    switch(config){
      case 1:          //f1 y c1            
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            u[r] = analogRead(I1); 
            y[r] = z[r] = v[r] = w[r] = t[r] = s[r] = 2048;
          }
      break;

      case 2:           //f1 y c1, luego lee f1 y c2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            u[r] = analogRead(I1);
          }
          sync1();
          for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V1);
            v[r] = analogRead(I2);
          }
      break;
      
      case 5:           //f1 y c1, luego lee f1 y c2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            u[r] = analogRead(I1);
          }
          sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            v[r] = analogRead(I2);
          }
          sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            w[r] = analogRead(I3);
          }
          sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            t[r] = analogRead(I4);
          }
          sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);    
            s[r] = analogRead(I5);
          }

      break;

      case 6:          //f1 y c1, f2 y c2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
      break;

      case 7:          //f1->c1, f2->c2, c3=generador ligado a f1
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V1);
            w[r] = analogRead(I3);
          }
      break;

      case 8:         //f1->c1, f2->c2, c3=generador ligado a f2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V2);
            w[r] = analogRead(I3);
          }
      break;

      case 11:             //f1->c1, f2->c2, f3->c3
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
      break;

      case 12:            //f1->c1, f2->c2, f3->c3, c4=generador ligado a f1
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V1);
            t[r] = analogRead(I4);
          }
      break;
      case 13:             //f1->c1, f2->c2, f3->c3, c4=generador ligado a f2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            yy[r] = analogRead(V2);
            t[r] = analogRead(I4);
          }
      break;
      case 14:             //f1->c1, f2->c2, f3->c3, c4=generador ligado a f3
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            zz[r] = analogRead(V3);
            t[r] = analogRead(I4);
          }
      break;
       case 15:             //f1->c1, f2->c2, f3->c3,     c4 a f1, c5 a f2
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1); //Serial.print(u[r]);  Serial.print(" "); 
            u[r] = analogRead(I1);
          }
          
          ///////
          //Serial.print(" L1:   "); 
          for(unsigned int r=0; r<resolution; r++){
          //Serial.print(x[r]);  Serial.print(","); 
          }
          //Serial.println(" "); 
          //-------
          
           //Serial.print(" I1:   "); 
          for(unsigned int r=0; r<resolution; r++){
          // Serial.print(u[r]);  Serial.print(","); 
          }
         // Serial.println(" "); 
          ///////
          
          
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
          
          ///////
          //Serial.print(" L2:   "); 
          //for(unsigned int r=0; r<resolution; r++){
          // Serial.print(y[r]);  Serial.print(","); 
         // }
         // Serial.println(" "); 
          
           //-------
          
          //Serial.print(" I2:   "); 
          for(unsigned int r=0; r<resolution; r++){
          // Serial.print(v[r]);  Serial.print(","); 
          }
          //Serial.println(" "); 
          ///////
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
          ///////
        //  Serial.print(" L3:   "); 
        //  for(unsigned int r=0; r<resolution; r++){
      //     Serial.print(z[r]);  Serial.print(","); 
      //    }
      //    Serial.println(" "); 
          
           ///////
          
         
        sync4();
        for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V1);
            t[r] = analogRead(I4);
          }
         sync5();
        for(unsigned int r=0; r<resolution; r++){
            yy[r] = analogRead(V2);
            s[r] = analogRead(I5);
          }
      break;
       case 16:             //f1->c1, f2->c2, f3->c3,       c4 a f1, c5 a f3
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I3);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
        sync4();
        for(unsigned int r=0; r<resolution; r++){
            xx[r] = analogRead(V1);
            t[r] = analogRead(I4);
          }
          sync6();
        for(unsigned int r=0; r<resolution; r++){
            zz[r] = analogRead(V3);
            s[r] = analogRead(I5);
          }
      break;
       case 17:             //f1->c1, f2->c2, f3->c3,           c4 a f2, c5 a f3
        sync1();
        for(unsigned int r=0; r<resolution; r++){
            x[r] = analogRead(V1);
            u[r] = analogRead(I1);
          }
        sync2();
        for(unsigned int r=0; r<resolution; r++){
            y[r] = analogRead(V2);
            v[r] = analogRead(I2);
          }
        sync3();
        for(unsigned int r=0; r<resolution; r++){
            z[r] = analogRead(V3);
            w[r] = analogRead(I3);
          }
        sync5();
        for(unsigned int r=0; r<resolution; r++){
            yy[r] = analogRead(V2);
            t[r] = analogRead(I4);
          }
           sync6();
        for(unsigned int r=0; r<resolution; r++){
            zz[r] = analogRead(V3);
            s[r] = analogRead(I5);
          }
      break;

    }//end switch
}//end readcycle

void addup(){
    Vrmsprom1=Vrmsprom1+Vrms1;             //cada paso por loop haz otra medicion y sumala a la anterior, al final cuando se termine el timer para pubnub se divide entre el total de medicones
    Irmsprom1=Irmsprom1+I1a;
    Powerprom1=Powerprom1+Power1;
                Imax1prom=Imax1prom+imax1;
                Imin1prom=Imin1prom+imin1;
    if(pf1!=0){pfactorprom1=pfactorprom1+pf1;  pf1add++;   }

    Vrmsprom2=Vrmsprom2+Vrms2;             //cada paso por loop haz otra medicion y sumala a la anterior, al final cuando se termine el timer para pubnub se divide entre el total de medicones
    Irmsprom2=Irmsprom2+I2a;
    Powerprom2=Powerprom2+Power2;
                Imax2prom=Imax2prom+imax2;
                Imin2prom=Imin2prom+imin2;
    if(pf2!=0){pfactorprom2=pfactorprom2+pf2;  pf2add++;   }

    Vrmsprom3=Vrmsprom3+Vrms3;             //cada paso por loop haz otra medicion y sumala a la anterior, al final cuando se termine el timer para pubnub se divide entre el total de medicones
    Irmsprom3=Irmsprom3+I3a;
    Powerprom3=Powerprom3+Power3;
                Imax3prom=Imax3prom+imax3;
                Imin3prom=Imin3prom+imin3;
     if(pf3!=0){pfactorprom3=pfactorprom3+pf3;  pf3add++;   }

    if(config == 12 || config == 13 || config == 14){
      Irmsprom4=Irmsprom4+I4a;
      Powerprom4=Powerprom4+Power4;
      //pfactorprom4=pfactorprom4+pfactor4;
    }
     if(config == 15 || config == 16 || config == 17){
      Irmsprom4=Irmsprom4+I4a;
      Powerprom4=Powerprom4+Power4;
                Imax4prom=Imax4prom+imax4;
                Imin4prom=Imin4prom+imin4;
       if(pf4!=0){pfactorprom4=pfactorprom4+pf4;  pf4add++;   }
      
      Irmsprom5=Irmsprom5+I5a;
      Powerprom5=Powerprom5+Power5;
                Imax5prom=Imax5prom+imax5;
                Imin5prom=Imin5prom+imin5;
       if(pf5!=0){pfactorprom5=pfactorprom5+pf5;  pf5add++;   }
    }
    p++;

}  //end addup             //realiza varias lecturas por segundo y suma los resultados para ser promediados despues

void gather(){
    
    readcycle();                        
    maxmin(); 
    area();
    powerfactor();
    addup();
}


void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  delay(3000);
  Serial.println("Configuring WDT core 1...");
  //esp_task_wdt_init(3, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  //esp_task_wdt_reset();
  for(;;){
    Serial.println("----------core 1-----------");
    vbatcheck();
    zeit=millis();                  //timer para guardar dato o publicar
   

  //xoc
    gather();                      //realiza aprox 94 mediciones de voltaje y corriente, procesa y calcula v, i, pf de cada fase
    power();                        //procesa y filtra el array para obtener Voltaje, Corriente, Potencia y factor de potencia
    addup();                        //realiza y suma tantas mediciones como pueda durante el intervalo entre cada periodo de registro
    empaquetador();                 //promedia las mediciones de cada intervalo y publica si cumple los criterios
    buttoncheck();
   //end xoc
  }
}

void looppublisher()
{
  for (int i = BACKUPSIZE; i > 0; i--)
  {
    if (payloadbag[i][5] == 1)
    {
      Serial.print("sending nr:"); Serial.println(i);
      payloadbag[i][5] = 0;
      long currentTimebkp = currenttimearray[i];
      float flowtempbkp = payloadbag[i][0];
      flowtotal = payloadbag[i][1];
      float flowacumbkp = payloadbag[i][2];
      float vbatbkp = payloadbag[i][3];
      float counterstatusbkp = payloadbag[i][4];

       
      float hotflowbkp = payloadbag[i][6];
      float hottempbkpIN = payloadbag[i][7];
      float hottempbkpOUT = payloadbag[i][8];
      float solarbatbkp = payloadbag[i][9];
      float nivo1bkp = payloadbag[i][10];
      float nivo1tempbkp = payloadbag[i][11];
      float nivo1batbkp = payloadbag[i][12];
      float solarcountbkp = payloadbag[i][13];
      float boilercountbkp = payloadbag[i][14];
      float nivocountbkp = payloadbag[i][15];
      

      //very old sendData("Timestamp_Device=" + String(currentTimebkp) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtempbkp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacumbkp) + "&vbat=" + String(vbatbkp) + "&counterstatus=" + String(counterstatusbkp));
      //less old   sendData("Timestamp_Device=" + String(currentTimebkp) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtempbkp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacumbkp) + "&vbat=" + String(vbatbkp) + "&counterstatus=" + String(counterstatusbkp) + "&hotflow=" + String(hotflowbkp)+ "&hottemp=" + String(hottempbkp)+ "&solarbat=" + String(solarbatbkp)+ "&nivo1=" + String(nivo1bkp) + "&nivo1temp=" + String(nivo1tempbkp)+ "&nivo1bat=" + String(nivo1batbkp)); 
      
      if(caso==0){    //datos de M3TR, no de los sensores extra
        sendData("Timestamp_Device=" + String(currentTimebkp) + "&config_id=" + String(caso)+ "&device_id=" + String(M3TRid) + "&temp=" + String(flowtempbkp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacumbkp) + "&vbat=" + String(vbatbkp) + "&counterstatus=" + String(counterstatusbkp)); 
        //sendData("Timestamp_Device=" + String(currentTimebkp) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtempbkp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacumbkp) + "&vbat=" + String(vbatbkp) + "&counterstatus=" + String(counterstatusbkp) + "&hotflow=" + String(hotflowbkp)+ "&hottemp=" + String(0)+ "&solarbat=" + String(solarbatbkp)+ "&nivo1=" + String(nivo1bkp) + "&nivo1temp=" + String(nivo1tempbkp)+ "&nivo1bat=" + String(nivo1batbkp)); 
        //ejemplo:  https://script.google.com/macros/s/AKfycbz_pcCj8ovohrlNnCZdJ2IwtwzR-uG23ejIsSIlm56_a1PpTMLy/exec?Timestamp_Device=123&config_id=0&device_id=3312&temp=10&flowshort=1&flowacum=2&vbat=3&counterstatus=4  if(caso==1 || caso==2 || caso==3){    //datos de sensores extra via LoRa ("solar", or "boiler bkp" or "heater todo")
        }
      if(caso==1 ||caso==3){    //datos de sensores extra via LoRa (solar y heater)
        sendData("Timestamp_Device=" + String(currentTimebkp) + "&config_id=" + String(caso) + "&flow=" + String(hotflowbkp) + "&tempIN=" + String(hottempbkpIN)+ "&tempOUT=" + String(hottempbkpOUT) + "&vbat=" + String(solarbatbkp)+ "&count=" + String(solarcountbkp));// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
        //ejemplo https://script.google.com/macros/s/AKfycbz_pcCj8ovohrlNnCZdJ2IwtwzR-uG23ejIsSIlm56_a1PpTMLy/exec?Timestamp_Device=123&config_id=2&flow=3312&tempIN=10&tempOUT=1&vbat=5
      }
      if(caso==2){    //datos de sensores extra via LoRa (boiler)
        sendData("Timestamp_Device=" + String(currentTimebkp) + "&config_id=" + String(caso) + "&flow=" + String(hotflowbkp) + "&tempIN=" + String(hottempbkpIN)+ "&tempOUT=" + String(hottempbkpOUT) + "&vbat=" + String(solarbatbkp)+ "&count=" + String(boilercountbkp));// + "&count=" + String(0)); //count todavia no esta soportado, no se si se necesita
        //ejemplo https://script.google.com/macros/s/AKfycbz_pcCj8ovohrlNnCZdJ2IwtwzR-uG23ejIsSIlm56_a1PpTMLy/exec?Timestamp_Device=123&config_id=2&flow=3312&tempIN=10&tempOUT=1&vbat=5
      }
      if(caso==4){    //datos de sensores extra via LoRa (nivo)
        sendData("Timestamp_Device=" + String(currentTimebkp) + "&config_id=" + String(caso)+ "&level=" + String(nivo1bkp) + "&tempIN=" + String(nivo1tempbkp) + "&vbat=" + String(nivo1batbkp)+ "&count=" + String(nivocountbkp));// + "&counterstatus=" + String(counterstatusbkp)); 
      }
      caso=0;
      


      if (flagonline == 1)
      {
        o--;
        if(o==0){i = 0;}//exit
      }
      else
      {
        payloadbag[i][5] = 1; //mark as unsent
      }
      i = 0; //exit
    }
    else
    {
       //Serial.print("nadaaaaaa maaaaas");
    }
  }//end for
}//end looppublisher

void railcheck()
{
  digitalWrite(railEnable, HIGH); //LOW=ON
  delay(20);
  vbat=analogRead(lipocheck);
  vbat=vbat*4/2100;           //from raw to volts, antes 2267
  if (vbat > 3.6)
  {
    digitalWrite(railEnable, LOW); //LOW=ON
  }
}

void OTAcheck(){
  ///////////////////////////////////////// ota //////////////////////////////////////////
int x=M3TRver;
Serial.println("OTA ___ M3TR ID: " + String(M3TR_unique_id)+" , version: "+  String(x));

//String bin = "/firmware" + String(version + 1) + ".bin";
String bin = "/firmware" + String(M3TR_unique_id) +"."+  String(x + 1) + ".bin"; //ejemplo: firmware4.9.bin
Serial.print("Looking for version: ");
Serial.println(String(x + 1) +"on firmware: "+ bin); 

//OTA.update("/G0_firmware_v1.3.bin");
OTA.update(bin);
}

void setup() /////////////////    SETUP    ///////////////////
  {
    //delay(200);
    Serial.begin(115200);
    //delay(300);
    Serial.print("\n\n");
    Serial.println("Bienvenido a M3TR! putas");
    //delay(100);
    //Serverx.begin();
  
    //atSerial.println("Texto");

    EEPROM.begin(EEPROM_SIZE);

    //delay(100);
    Serial.println("-----------SETUP-----------");
    pinMode(flowpin, INPUT_PULLUP);
    pinMode(flowled, OUTPUT);
    pinMode(railEnable, OUTPUT);
    pinMode(enserialport, OUTPUT);
    digitalWrite(railEnable, HIGH); //LOW=ON
    digitalWrite(enserialport, LOW); //LOW=ON
    atSerial.begin(9600, SERIAL_8N1, 17, 16);
    delay(100);
    atSerial.print("Bienvenido enviado por atSerial");
    whiteIDlist();


    pinMode(ledred, OUTPUT);
    digitalWrite(ledred, LOW);
    pinMode(ledgreen, OUTPUT);
    digitalWrite(ledgreen, LOW);

    pinMode(bot, INPUT_PULLUP);
    pinMode(14, INPUT_PULLDOWN);//que es esto?

    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, HIGH);
    delay(300);
    digitalWrite(ledred, HIGH);
    digitalWrite(ledgreen, LOW);
    delay(300);



    Serial.println("Configuring WDT core 0...");

    /*
  #define lipocheck 36  //GPIO36 analog pin para medir voltaje de bateria 
#define flowpin 34   //digital pin para medir el sensor de flujo
#define flowled 2
#define ledgreen 32
#define ledred 35
#define bot 26
*/

    // xTaskCreatePinnedToCore(
    //                   Task1code,   /* Task function. */
    //                   "Task1",     /* name of task. */
    //                   10000,       /* Stack size of task */
    //                   NULL,        /* parameter of the task */
    //                   1,           /* priority of the task */
    //                   &Task1,      /* Task handle to keep track of created task */
    //                   0);          /* pin task to core 0 */
    // delay(500);

    //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
    Serial.println("Configuring Task2Core...");
    xTaskCreatePinnedToCore(
        Task2code, /* Task function. */
        "Task2",   /* name of task. */
        10000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &Task2,    /* Task handle to keep track of created task */
        1);        /* pin task to core 1 */
    delay(500); 

  delay(100);
  //esp_task_wdt_reset();

  //connectToWiFi(0);
  Serial.println("");

  //connectToAWS();
  delay(50);

  //initLora(); 
  delay(50);

  // Enable saved past credential by autoReconnect option,
  // even once it is disconnected.
  //Config.immediateStart = true;
  /////Config.autoReconnect = true;
  //Config.hostName = "M3TR";
  /////Config.portalTimeout = 60000;
  /////Config.apid = "M3TR " + String(M3TRid);
  //Config.apip = 192.168.0.1;
  /////Config.retainPortal = false;     //testito


  
  /////Portal.config(Config);
///////---------
  // // Load aux. page
  // Timezone.load(AUX_TIMEZONE);
  // // Retrieve the select element that holds the time zone code and
  // // register the zone mnemonic in advance.
  // AutoConnectSelect&  tz = Timezone["timezone"].as<AutoConnectSelect>();
  // for (uint8_t n = 0; n < sizeof(TZ) / sizeof(Timezone_t); n++) {
  //   tz.add(String(TZ[n].zone));
  // }

  // Portal.join({ Timezone });        // Register aux. page
//////////------
  // Behavior a root path of ESP8266WebServer.
  /////Server.on("/", rootPage);
  /////Server.on("/start", startPage);   // Set NTP server trigger handler

  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  

  // Serial.println("wifi begin setup");
  // //WiFi.begin();
  /////WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // for (int a = 30; a > 0; a--)
  // {
  //   digitalWrite(ledred, LOW);
  //   digitalWrite(ledgreen, HIGH);
  //   delay(100);
  //   digitalWrite(ledred, LOW);
  //   digitalWrite(ledgreen, LOW);
  //   delay(100);
  // }
  //esp_task_wdt_reset();

 ///// connectToWiFi(1);
  //if (WiFi.status() != WL_CONNECTED){
  //  autoconnect_stuff();
  //}

  
 ///// Serial.println("Configurando timeClient...");
  /////timeClient.begin();
  /////timeClient.setTimeOffset(0);
  /////timeClient.update();
  /////Serial.print(timeClient.getEpochTime());




  

//railcheck();
    sensor.begin(); //ds18b20 temp sensor (pin gpio15)


} //end setup

void CheckForConnections()
{
  if (Serverx.hasClient())
  {
    // If we are already connected to another computer, 
    // then reject the new connection. Otherwise accept
    // the connection. 
    if (RemoteClient.connected())
    {
      Serial.println("Connection rejected");
      Serverx.available().stop();
    }
    else
    {
      Serial.println("Connection accepted");
      RemoteClient = Serverx.available();
    }
  }
}


void loop()
{
  //Serial.print(".");
  connectToWiFi(0);
  if (flagonline == 1)
  {
    looppublisher();
  }

  unsigned long timerbkp = millis();
  if (timerbkp - previousMillistimerbkp >= 3600000) //cada hora
  {
      previousMillistimerbkp = timerbkp;
      timebackup = 1;
  }
  //antes
  // if (millis() > timer)
  // {
  //   timer = 2*3600000 + millis(); //cada 2 horas
  //   timebackup = 1;
  // }


  unsigned long timerbkpOTA = millis();
  if (timerbkpOTA - previousMillistimerOTA >= 180000) {
      previousMillistimerOTA = timerbkpOTA;
      OTAcheck();
  }
//antes
  // if (millis() > timerOTA)
  // {
  //   timerOTA = 180000 + millis(); //cada 3 min
  //   OTAcheck();
  // }

  checkserial();
  sensors();  

  delay(100);
  Portal.handleClient();
 // CheckForConnections();
} //end loop


//millis overflow/rollover: https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover

//problema1: publicar tarda mas de 8 segundos por mensaje...


//esptool.py  --port /dev/cu.SLAB_USBtoUART erase_flash


//falta: watchdog
//falta hacer backup de version estable
//falta cuando el internet esta malo no hace bien los backups offline y no se recupera.

