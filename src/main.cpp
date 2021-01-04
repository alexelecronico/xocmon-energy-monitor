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

void rootPage() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<script type=\"text/javascript\">"
    "setTimeout(\"location.reload()\", 1000);"
    "</script>"
    "</head>"
    "<body>"
    "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Hello, world</h2>"
    "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
    "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
    "<p></p><p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";
  static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
  struct tm *tm;
  time_t  t;
  char    dateTime[26];

  t = time(NULL);
  tm = localtime(&t);
  sprintf(dateTime, "%04d/%02d/%02d(%s) %02d:%02d:%02d.",
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
    wd[tm->tm_wday],
    tm->tm_hour, tm->tm_min, tm->tm_sec);
  content.replace("{{DateTime}}", String(dateTime));
  Server.send(200, "text/html", content);
}

void startPage() {
  // Retrieve the value of AutoConnectElement with arg function of WebServer class.
  // Values are accessible with the element name.
  String  tz = Server.arg("timezone");

  for (uint8_t n = 0; n < sizeof(TZ) / sizeof(Timezone_t); n++) {
    String  tzName = String(TZ[n].zone);
    if (tz.equalsIgnoreCase(tzName)) {
      configTime(TZ[n].tzoff * 3600, 0, TZ[n].ntpServer);
      Serial.println("Time zone: " + tz);
      Serial.println("ntp server: " + String(TZ[n].ntpServer));
      break;
    }
  }

  // The /start page just constitutes timezone,
  // it redirects to the root page without the content response.
  Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
  Server.send(302, "text/plain", "");
  Server.client().flush();
  Server.client().stop();
}
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
#define ledgreen 32 //antes 32
#define ledred 33   //antes 33
#define bot 26
#define railEnable 25
#define enserialport 2

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
  } // cada  5 min con flujo y cada 0.5 horas sin flujo
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
    if (inChar == '(')
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
        if (lengthpayload > 31)
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

    if (bufserial[4] == '4') //nivo
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
      WiFi.begin(WIFI_SSID2, WIFI_PASSWORD2);
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
}

void readshort(){
    unsigned long timer1=millis();
    //long timer2=millis();
    int flagflow = 0;
    //flow=0;
    pulses=0;
   
    do{
        if(digitalRead(bot)==LOW){timer1=millis()+TiempoLectura;digitalWrite(ledgreen,LOW);digitalWrite(ledred,HIGH);}
        if(digitalRead(flowpin) == LOW){
          digitalWrite(flowled,LOW);
          if(flagflow == 0)
            {
              Serial.print((int)pulses); Serial.print(" ");     //imprime los pulsos recibidos
              pulses++;
              flagflow=1;
            }
        }
        else if(digitalRead(flowpin) == HIGH) {digitalWrite(flowled,HIGH);flagflow=0;}
      }  
    while (millis() - timer1 < TiempoLectura ); 
    

    
    //if(WiFi.ready()){  
       // if(Particle.connected && flagonline==0){
           // publishnow=0;
          //  Particle.process();
       // }
    //}
}

void flowread(){
  Serial.print("flowread..");
   
    flow=0;
    for(int i=0;i<multiplos;i++){
        if (publishnow==0){i=multiplos;Serial.println("i=multiplos!!!!! publica ya!");}
        readshort(); 
       // esp_task_wdt_reset();
        Serial.print(" ");Serial.print(i);Serial.print("/");Serial.print(multiplos);Serial.print(" ");
        
        //if(i==10 || i==60 || i==120 || i==180 || i==240 || i==300 || i==360 || i==420|| i==480 || i==540 || i==600 || i==660 || i==720 || i==780 || i==840){
            //Serial.println("wificheck() desde multiplos");
            //wificheck();
     
        //}
        
        if(sensormodel==1){
            if(                    pulses >= A[0][1]) {flow=flow+    A[0][2] * pulses / A[0][1]          ;}
            if(pulses >= A[1][1] && pulses < A[0][1]) {flow=flow+    map(pulses,A[1][1],A[0][1],A[1][2],A[0][2]);}
            if(pulses >= A[2][1] && pulses < A[1][1]) {flow=flow+    map(pulses,A[2][1],A[1][1],A[2][2],A[1][2]);}
            if(pulses >= A[3][1] && pulses < A[2][1]) {flow=flow+    map(pulses,A[3][1],A[2][1],A[3][2],A[2][2]);}
            if(pulses >= A[4][1] && pulses < A[3][1]) {flow=flow+    map(pulses,A[4][1],A[3][1],A[4][2],A[3][2]);}
            if(pulses >  2.0     && pulses < A[4][1]) {flow=flow+    A[4][2]  * pulses /A[4][1];}
        }
     // else if(sensormodel==2){
          
    // }

    Serial.print(flow); Serial.println(" ");
    }
     if(flow<2){flow=0;}
     flow=flow*calib;       //ajuste calibracion global
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

void publicador()
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
    flowread();
    empaquetador();
    buttoncheck();
    ////esp_task_wdt_reset();
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
String bin = "/firmware" + String(M3TR_unique_id) +"."+  String(x + 1) + ".bin";
Serial.print("Looking for version: ");
Serial.println(String(x + 1) +"on firmware: "+ bin);

//OTA.update("/G0_firmware_v1.3.bin");
OTA.update(bin);
}

void setup() /////////////////    SETUP    ///////////////////
  {
    //delay(200);
    Serial.begin(9600);
    //delay(300);
    Serial.print("\n\n");
    Serial.println("Bienvenido a M3TR! putas");
    //delay(100);
  
  
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

void loop()
{
  //Serial.print(".");
  connectToWiFi(0);
  if (flagonline == 1)
  {
    looppublisher();
  }
  
  if (millis() > timer)
  {
    timer = 2*3600000 + millis(); //cada 2 horas
    timebackup = 1;
  }

  if (millis() > timerOTA)
  {
    timerOTA = 180000 + millis(); //cada 3 min
    OTAcheck();
  }

  checkserial();
  sensors();  

  delay(100);
  Portal.handleClient();
} //end loop




//problema1: publicar tarda mas de 8 segundos por mensaje...


//esptool.py  --port /dev/cu.SLAB_USBtoUART erase_flash


//falta: watchdog
//falta hacer backup de version estable

