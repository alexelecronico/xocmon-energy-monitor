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


const unsigned int BACKUPSIZE=500;
int o=0;
unsigned long currenttimearray[BACKUPSIZE];
float payloadbag[BACKUPSIZE][6];


int multiplos=30;            //(sin unidad) cuantos "TiempoLectura" leo continuamente antes de reportar a la nube
int reportacada=20;         //(milisegundos) x por multilplos por TiempoLectira. es el tiempo maximo de espera si la lectura es cero y no reporta 

const int TiempoLectura=1000;      //
const int multiplosonline=30;//30  
const int multiplosoffline=180;//180
const int reportacadaonline=240;//240 
const int reportacadaoffline=40;//40

float vbat=0;
#define lipocheck 36  //GPIO36 analog pin para medir voltaje de bateria 
#define flowpin 34   //digital pin para medir el sensor de flujo
#define flowled 2
#define ledgreen 33
#define ledred 32
#define bot 26
#define railEnable 25

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


void APmode(){

  //disableCore0WDT();
  Serial.println("AP mode");
  Serial.println("Creating portal and trying to connect...");

  Config.immediateStart = true;
  Config.autoReconnect = true;
  Config.hostName = "M3TR " + String(M3TRid);
  Config.portalTimeout = 60000;
  Config.apid = "M3TR " + String(M3TRid);
  //Config.apip = 192.168.0.1;
  Config.retainPortal = false;     //testito
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
    WiFi.mode(WIFI_STA);
    //WiFi.begin();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   // WiFi.setHostname("M3TR");

    //portal.handleClient();
  }
}// end AP mode


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

        if (flagonline==0){
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
    WiFi.disconnect();
    delay(100);
    Serial.println("no wifi ");
    digitalWrite(ledred, HIGH);
    digitalWrite(ledgreen, LOW);
    flagonline = 0;
    Serial.print("[INFO]: Start setup to internet connection ");
    //WiFi.mode(WIFI_STA);
    WiFi.setHostname("M3TR");
    WiFi.mode(WIFI_STA);

    WiFi.begin();
    //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   

    // Only try 30 times to connect to the WiFi
    int retries = 30;
    while (WiFi.status() != WL_CONNECTED && retries > 1)
    {
      Serial.print("wifi reconnecting...");

      //WiFi.setAutoReconnect(true);
      //WiFi.persistent(true);
      //WiFi.begin();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      //esp_task_wdt_reset();
      WiFi.reconnect();
      delay(1000);
      Serial.print(retries);
      retries--;
     // giveup--;
     // if(giveup==0){
      //  esp_restart();
     // }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      flagonline=0;
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
        Serial.print("ping: ");Serial.println(avg_time_ms);
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
}


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
}

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

void onlinecheck3() //checa que pueda subor datos a internet, si no entonces guarda en ram cada medicion hasta que pueda subir los datos pendientes
{
  Serial.print("onlinecheck3");
  if (flagonline==1)
  {
    // bool ret = Ping.ping("www.google.com", 10);
    //avg_time_ms = Ping.averageTime();
    if (avg_time_ms < 1000)
    {
      { //if internet, send backups until finished

        Serial.println("");
        Serial.println("INTERNET OK!!");
        digitalWrite(ledred, LOW);
        digitalWrite(ledgreen, HIGH); //green
        reportacada = reportacadaonline;
        multiplos = multiplosonline;

        counterstatus++;
        if (counterstatus > 9999)
        {
          counterstatus = 0;
        }
        intcounterstatus = counterstatus;
        counterstatus = intcounterstatus;

        //digitalWrite(ledred, LOW);
        //digitalWrite(ledgreen, LOW);
        //flagonline = 1;

        //  [0]          [1]       [2]         [3]       [4]     [5]               [6]
        //currenttime,  flowtemp, flowtotal,flowacum,   vbat,   counterstatus,    has data?

        // currentTime = currenttimearray[o];
        // flowtemp1 = payloadbag[o][0];
        // flowtotal = payloadbag[o][1];
        // flowacum = payloadbag[o][2];
        // vbat = payloadbag[o][3];
        // counterstatus = payloadbag[o][4];
      }
    }
  }

  else
  { //if no internet store backup
    //flagonline=0; 

    digitalWrite(ledred, HIGH);
    digitalWrite(ledgreen, LOW);
    Serial.println("");
    Serial.println("NO INTERNET!!");
    reportacada = reportacadaoffline;
    multiplos = multiplosoffline;
    updatecurrentTime();

    counterstatus++;
    if (counterstatus > 9999)
    {
      counterstatus = 0;
    }
    
    intcounterstatus = counterstatus;
    counterstatus = intcounterstatus + 0.1;

    Serial.print("Backup counter 'o': ");
    Serial.println(o);
    if (o > 100)
    {
      reportacada = 15; multiplos = 120;
    } // cada  5 min con flujo y cada 0.5 horas sin flujo
    if (o > 200)
    {
      reportacada = 12; multiplos = 600;
    } // cada 10 min con flujo y cada 2 horas sin flujo
    if (o > 300)
    {
      reportacada = 16; multiplos = 900;
    } // cada 15 min con flujo y cada 4 horas sin flujo

    if (o == BACKUPSIZE - 1)
    {
      o = 0;
    }

    else
    {

      //cleanup
      if(o==0){
        for(int i=0;i<BACKUPSIZE;i++)
        {
          payloadbag[o][5] = 0;
        }
      }

      //  [0]          [1]       [2]         [3]       [4]     [5]               [6]
      //currenttime,  flowtemp, flowtotal,flowacum,   vbat,   counterstatus,    has data?

      currenttimearray[o] = currentTime;
      payloadbag[o][0] = flowtemp1;
      payloadbag[o][1] = flowtotal;
      payloadbag[o][2] = flowacum;
      payloadbag[o][3] = vbat;
      payloadbag[o][4] = counterstatus;
      payloadbag[o][5] = 1;

      Serial.print("backup: ");
      Serial.print(o);
      Serial.println(" saved:  ");

      Serial.print("               currenttime: ");
      Serial.println(currentTime);
      Serial.print("               flowtemp1: ");
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
      o++;
    }
    // }
  } //end else
} //end onlinecheck3


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
    // flowtotal = 0; //reset flowshort (flowtotal) para que pueda ir contando nuevamente mientras google responde
  }
  else
  {
    flagonline = 0;
    counterstatus--;
    onlinecheck3();
  }

  flowtotal = 0;
}//end senddata

void publicador()
{
  Serial.println(" publicador");
  digitalWrite(ledred, HIGH);
  digitalWrite(ledgreen, LOW);
  updatecurrentTime(); //timestamp en formato epoch para enviar en el paquete
  onlinecheck3();


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


  if(flow>=0.1){publishnow=0;}

    
  Serial.print (" flowtotal: ");Serial.print(flowtotal); Serial.print (" flowacum: ");Serial.print(flowacum);
  long periodo = multiplos*1000;
  if((zeit-prezeit) >      periodo   *   reportacada * flaginit * publishnow)     { 

    if((flowtotal >= 0.0) || flagpubcount > maxwait) { flagpub=1; }   
    else {
      if(publishnow==0){ flagpub=1; }
      else {flagpub=0; flagpubcount++;}
    }
        
    if (flagpub == 1) {
      vbatcheck();
      //tempread();
      publicador(); flagpubcount=0; //antes estaba aqui flowtotal=0; pero lo movi adespues de sendData
    } 
    publishnow=1; 
    prezeit=zeit;
  }//end if
  
  Serial.println(" end empaquetador ");
}//end empaquetador





// void Task1code( void * pvParameters ){
//   Serial.print("Task1 running on core ");
//   Serial.println(xPortGetCoreID());
//   for(;;){
//     Serial.println("----------core 0-----------");
//     //connectToWiFi();
//     if (WiFi.status() != WL_CONNECTED)
//     {
//       Serial.println("[ERROR]: Could not connect to WiFi");
//       //APmode();
//       //delay(1000);
//     }
//     delay(1000);
//   } 
// }


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
    //////esp_task_wdt_reset();
  }
}


void looppublisher()
{
  if (o > 0)
  {

    if (flagsend == 1)
    {
      //carga datos actuales
      sendData("Timestamp_Device=" + String(currentTime) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtemp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacum) + "&vbat=" + String(vbat) + "&counterstatus=" + String(counterstatus));
      if (httpCode == 302 || httpCode == 200)
      {
       // flagsend = 0;   
      }
      flagsend = 0;
    }
    else
    {
      if (x < o)
      {
        digitalWrite(ledred, LOW);
        digitalWrite(ledgreen, LOW);


        payloadbag[x][5] = 0;     
        

        long currentTimebkp = currenttimearray[x];
        float flowtempbkp = payloadbag[x][0];
        flowtotal = payloadbag[x][1];
        float flowacumbkp = payloadbag[x][2];
        float vbatbkp = payloadbag[x][3];
        float counterstatusbkp = payloadbag[x][4];

        x++;
        sendData("Timestamp_Device=" + String(currentTimebkp) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtempbkp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacumbkp) + "&vbat=" + String(vbatbkp) + "&counterstatus=" + String(counterstatusbkp));
        if (httpCode == 302 || httpCode == 200)
        {
          //flowtotal = 0;          //reintenta
        //  payloadbag[x][5] = 0;
         // x++;
        }
        if (httpCode == -1)
        {
          x--;
          payloadbag[x][5] = 1;    
        //payloadbag[x][5] = 0;     //voy a probar que solo lo intente una vez aunque no reciba confirmacion...
        //x++;
        }
        digitalWrite(ledred, LOW);
        digitalWrite(ledgreen, HIGH);
      }
      if(x==o)
      {
        x=o=0;
      }
    }//end else
  }

  if (flagsend == 1)
  {
    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, LOW);
    //carga datos actuales
    sendData("Timestamp_Device=" + String(currentTime) + "&device_id=" + String(M3TRid) + "&temp=" + String(flowtemp) + "&flowshort=" + String(flowtotal) + "&flowacum=" + String(flowacum) + "&vbat=" + String(vbat) + "&counterstatus=" + String(counterstatus));
    if (httpCode == 302 || httpCode == 200)
    {
     // flagsend = 0;
    }
    flagsend = 0;
    if (httpCode == -1)
    {
      //flagsend = 1;
    }

    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, HIGH);
  }
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

void setup()                                                                      /////////////////    SETUP    ///////////////////
{

  Serial.begin(9600);
  delay(1000);
  Serial.print("\n\n");
  Serial.println("Bienvenido a M3TR!");

  EEPROM.begin(EEPROM_SIZE);

  delay(100);
  Serial.println("-----------SETUP-----------");
  pinMode(flowpin,INPUT_PULLUP);
  pinMode(flowled,OUTPUT);
  pinMode(railEnable,OUTPUT);
  digitalWrite(railEnable,HIGH);  //LOW=ON
  
  pinMode(ledred,OUTPUT);
  digitalWrite(ledred,LOW);
  pinMode(ledgreen,OUTPUT);
  digitalWrite(ledgreen,LOW);

  pinMode(bot,INPUT_PULLUP);
  pinMode(14,INPUT_PULLDOWN);

  digitalWrite(ledred,LOW);
  digitalWrite(ledgreen,HIGH);
  delay(300);
  digitalWrite(ledred,HIGH);
  digitalWrite(ledgreen,LOW);
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
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
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
  Config.immediateStart = true;
  Config.autoReconnect = true;
  //Config.hostName = "M3TR";
  Config.portalTimeout = 60000;
  Config.apid = "M3TR " + String(M3TRid);
  //Config.apip = 192.168.0.1;
  Config.retainPortal = false;     //testito


  
  Portal.config(Config);
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
  Server.on("/", rootPage);
  Server.on("/start", startPage);   // Set NTP server trigger handler

  //esp_task_wdt_init(20, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  

  Serial.println("wifi begin setup");
  //WiFi.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (int a = 30; a > 0; a--)
  {
    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, HIGH);
    delay(100);
    digitalWrite(ledred, LOW);
    digitalWrite(ledgreen, LOW);
    delay(100);
  }
  //esp_task_wdt_reset();

  connectToWiFi(1);
  //if (WiFi.status() != WL_CONNECTED){
  //  autoconnect_stuff();
  //}

  
  Serial.println("Configurando timeClient...");
  timeClient.begin();
  timeClient.setTimeOffset(0);
  timeClient.update();
  Serial.print(timeClient.getEpochTime());
  

railcheck();

//OTAcheck();


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

  // if (millis() > timerReset)
  // {
  //   timerReset = 4*3600000 + millis(); //4horas
  //   timebackup=1;
  //   updatecurrentTime();
  //   ESP.restart();
  // }

  //esp_task_wdt_reset();
  delay(300);
  Portal.handleClient();
} //end loop

//OTA

//problema1: publicar tarda mas de 8 segundos por mensaje...


//esptool.py  --port /dev/cu.SLAB_USBtoUART erase_flash
