#ifndef configs_H
#define configs_H

//google publish
 #include <HTTPClient.h>

String GOOGLE_SCRIPT_ID_UNIVERSAL = "AKfycbyRh8JmdSkUlhHSSYDKoXamBKhEeWvocPHMupTTCJVWwcHPRCq1NQMNJzI1ldZPcV7P";   //xocUniversalLog

//String GOOGLE_SCRIPT_ID = "AKfycbyPJOnFbw_qaIzhWTE9x-DocHO9zI_DgcSvat8mV7jQEvugwGrpPGuYPI7whYydoEaxLw";   //xoc100         ofi martin
//String GOOGLE_SCRIPT_ID = "AKfycbwXHoXiMxsIuWkYiYqx0_Ojf5tWEI3ADdIIIoOEWHv4J2qBxxk44VFTbGYoIltG7eMTSg";   //xoc100.2       ofi martin
//String GOOGLE_SCRIPT_ID = "AKfycbzbatJ23it4UXlkAWxygxuRZHQLH8xCn0YYpXdblshy3zovBH70zTwoDq8-PksskLeQlg";   //xoc110.1old    placa 1
//String GOOGLE_SCRIPT_ID = "AKfycbx1eTUim-IaVYIJKSrfKoBrz12SRGDIJ79_65JzjwnlAU-yB1uH427y63ZZMGu3uYU";      //xoc110.2       auditor32, antes ofi rafa 
//String GOOGLE_SCRIPT_ID = "AKfycbzUdArlq6LJqx0CaDESUdY6G3SllTf7RdS3QCxdNxHJUB9KExGBpmDJ8o7RmA_C3I6u";     //xoc110.3       nueva version con maxamps
//String GOOGLE_SCRIPT_ID = "AKfycby_S8heoGftmAkB2zkqIB8lV7z3Vhx9onEpBzakPphD_tDd0aP-ge_47zNTbsalU-3QCw";   //xoc111.2       casa rafa
//String GOOGLE_SCRIPT_ID = "AKfycbyi3zE-9paV9szCT9rrZtTLJalmb1eJileY8W6x5u2_sAbheyZyZYp3FClF7RoOUTmV";       //xoc112.3       XOC lab martin casa 42
//String GOOGLE_SCRIPT_ID = "AKfycbwCmgJNQL4QljM-QiLYVgjslGDyO_xFn21dm5mxD0oX3g8dib1Ls1bkIATYr5ZvkZZvPw";  //120 Mondragón
String GOOGLE_SCRIPT_ID = "AKfycbxjNaInfU0qQZXZETVB5phLk4nwPfVqQwHVLyBSh818LGwRnfxioHUB9_AA516A_cld"; //307 Cafe Mondragon
//String GOOGLE_SCRIPT_ID = "AKfycbxRBE2Mmx87n9x1vo6U9zJ9XNiudrM7cAIfer8aCzkzDXtAJtsqDJ8nFw2CCP8nEb1u"; //XOC 308. Falta por asignar.   
//String GOOGLE_SCRIPT_ID = "AKfycbwBMEqByPHIbSH_1rUYLl_unrL0XUao_0W9euQxywX-VWAhRLXQEwQcqK5LYLu9HkJN"; //XOC 309. Falta por asignar. Es la que tiene antena. 
//String GOOGLE_SCRIPT_ID = "AKfycbzaAoIxhJj3c3gWWkOJq0qZAGklBZAFwMLfS3d3dpM8D6E-aWDQoukvTYWgRQ2YLCmq"; //Xoc 310. Falta por asignar. Es la que tiene antena. 
//String GOOGLE_SCRIPT_ID = "AKfycbzV-ZoNT_nn9bW35pp808d4bK-sdnc3xOlXuqwaH5o6R3_r1VoxX99-wBJXxjduQ13q";       //xoc113.3       
//String GOOGLE_SCRIPT_ID = "AKfycbymt_d6malAw6fYKGCrgz9yuHgOj0Mrp8IRLj4LOye8zkQK0wk2T2-EJ8r2Emay_aCrNA";      //xoc114.3    
//String GOOGLE_SCRIPT_ID = "AKfycbwOsPqZyUdSsBWSiXjOMi9RHfynT5iGb9ajQ7uK-UWdtCs7ASETRP71mr2xUFitGMdn";         //xoc115.3    //panaderia wifi: IZZI-0BA4    pass: 6TsqH2amZx7FgrLFab  ANTES wifi: In-syc   pass: 1100303914
//String GOOGLE_SCRIPT_ID = "AKfycbzmQrColmTFXfAwn-CVxo92ouUHRruxqU-RHa8nMpL3XtTI3oMA9RrHoEWqv1qoTAQ";         //xoc116.3     //panaderia wifi: IZZI-0BA4    pass: 6TsqH2amZx7FgrLFab
//String GOOGLE_SCRIPT_ID = "AKfycbyVfh8g6TBGLESJM-_lsU-dCL7bU5cHINSxrm0KnynVstVfZIA4e8fDGkQE1D_kwaI0";          //xoc117.3     //panaderia wifi: IZZI-0BA4    pass: 6TsqH2amZx7FgrLFab

 
//>>>>>>>>>   remember to check:   <<<<<<<<<<<<<<<<<<<<<<<<
//1.sheet, 2.id, 3.ver, 4.wifi, 5.calib

const byte XOC_unique_id = 121;      //ID. Mi propia aportación. Para XOC120 es 120. XOC 307 es 121. XOX 308 es 122. XOC 309 es 123.  XOC 310 es 124. 
const float XOCver = 300;            //308      //version, debe ser igual a la que se exporta p.ej firmware4.6.bin en https://s3.console.aws.amazon.com/s3/buckets/otabucketm?region=us-east-1&tab=objects   services@m3tr.com    aguapasapormicasaaws3!

int caso = 0; //caso se usa para que la google spreadsheet mande adiferentes sheets, por el momento solo hay caso=0

const int resolution=440;//1040
const int reportacadaonline=60;    //180         //multiplos de 10 seg, 90=15min      antes 120
const int reportacadaoffline=60;    //360       //multiplos de 10 seg                 antes 360
int reportacada=60;             //1             120      //hace un corte de mediciones cada multiplo de 10 segundos.  ej: 18=180seg=3min  (ver la subrutina empaquetador)

const byte variabilidad=0;       //1              0      //1=se envia cuando hay variaciones y "reportacada" debe ser 1, 0=se envia cada "reportacada"
float deltai = 3.0;             //minima variacioon en corriente de cualquier clamp para trigerear reporte a la nube [amperes]
const int maxwait=20;//360
int initialmessages=3;                            //para publicar inmediatamente sin esperar a 'reportacada', envia la cantidad de mensajes de 'initialmessages' con delay de 10seg entre mensajes

////////xoc variables/////////
const int midpoint = 2046;  
const float Vcalib = 0.087;              //factor multiplicador para calibrar voltaje (falta ver si es lineal)    
const int atenuacion = 5;           // para reducir ruido cuando las corrientes son muy bajas (menores a 0.5A en clamps de 50A o sea 1% de la capacidad de la clamp), cualquier valor abajo de x se toma como cero.
const float pfcalib = 0;                 //correccion de pf por motivo de capacitancia e inductancia en los sensores

float icalib[]={
    404.64,//43.1@50A   99.13? Para 50A 94.94. 404.64 para transductor de 100A.  101.31 para 50A
    404.64, 
    404.64, 
    404.64,
    404.64, //Antes 216.74
    404.64, //transductor con etiqueta de 5. 
    404.64,
    404.64, 
    404.64,
    404.64,
    404.64,
    404.64,
};

//0 ignore, 1=fase1, 2=fase2, 3=fase3, 4=dont care just read           
//              1,2,3,4,5,6,7,8,9,10,11,12
 //byte iconfig[]={0,0,0,0,0,0,0,0,1,1, 1, 1}; //Para XOC 310
 //byte iconfig[]={1,1,1,1,0,1,0,0,0,0, 1, 1}; //Para XOC 120
 //byte iconfig[]={0,0,0,0,0,0,0,0,1,1, 1, 1}; //Para XOC 309
 //byte iconfig[]={1,1,1,1,1,1,1,1,1,1, 1, 1}; //Para XOC 308
 byte iconfig[]={1,1,1,1,1,1,1,1,1,1, 1, 1}; //Para XOC 307


const float XOCid = XOC_unique_id + XOCver/1000;  //p ej: 1.03;
//local AP url: http://172.217.28.1/_ac/

//////////////////// Default Wifi credentials ///////////////////////////

//const char * WIFI_SSID = "FLYX";
//const char * WIFI_PASSWORD = "infinito";

  const char * WIFI_SSID = "UMx Dragon";
  const char * WIFI_PASSWORD = "Dragones.UMx";

//const char * WIFI_SSID = "INFINITUMCCB5";
//const char * WIFI_PASSWORD = "7hfVacXZv4";  


//const char * WIFI_SSID = "GARWILq";
//const char * WIFI_PASSWORD = "chipotle";

const char * WIFI_SSID_xoc = "xoc";
const char * WIFI_PASSWORD2 = "12345678";


const byte whitelistlength=20;
int whiteID[whitelistlength];





// MQTT information4
//#define DEVICE_NAME "M3TR"
// #define AWS_IOT_TOPIC "gprod_test"
// #define AWS_IOT_ENDPOINT "a3ed10vql0i9ty-ats.iot.us-east-1.amazonaws.com"
// #define AWS_MAX_RECONNECT_TRIES 5

// // LoRa params
// #define SCK 5   // GPIO5  SCK
// #define MISO 19 // GPIO19 MISO
// #define MOSI 27 // GPIO27 MOSI
// #define SS 18   // GPIO18 CS
// #define RST 14  // GPIO14 RESET

// #define BAND 915E6
// #define SPREADING_FACTOR 12
// #define BAND_WINDTH 500E3

#endif



//esptool.py  --port /dev/cu.SLAB_USBtoUART erase_flash
//esptool.py  --port /dev/cu.usbserial-0001 erase_flash



//versiones binarios
//000.xyz donde x es el entero, y es el decimal, z es un build para hacer hasta 9 pruebas de OTA sin cacmbiar x o y.
/*
v11x se corrige empaquetador a:  if (publishnow == 0 || flaginit==1) dode antes no se tomaba en cuante glafinit y mandaba cuenta de wh elevada cuando se reinicia el xoc
v12x se mejora updatecurrenttime para prevenir que mande hora actualizada
v2xx running on VScode workspace "xoc003" nueva payload que incluye maxamps1-12 para mostrar corriente pico en cada intervalo. La payload solo es compatible con google sheets "xocXXX.3"


*/


/*
task2:

for (byte i = 0; i < 12; i++)
    readcycle_new
    analysis_new(byte i)
        cleanup_new
        voltajes_new
        corrientes_new
        maxcurrent(a);
        powerfactor_new();
    addup_new
    delay(10);

empaquetador
delay(100)

*/

