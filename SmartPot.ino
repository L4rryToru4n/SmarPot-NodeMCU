/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */

/* SMART POT PROJECT ~ PPLBIoT */
/* This project reference could be found in 
 *  https://www.instructables.com/Automatic-Gardening-System-With-NodeMCU-and-Blynk-/
 *  Works credited to ArduFarmBot 2 by mjrovai
*/

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>
#include <Wire.h>
BlynkTimer timer;

/* Automatic Control Parameters Definition */
#define SOIL_DRY 16
#define SOIL_WET 25
#define TEMP_COLD 25
#define TEMP_HOT 27
#define TIME_PUMP_ON 7

/* Timer */
#define READ_SOIL_HUM_TM 10L // definitions in seconds
#define READ_AIR_DATA_TM 2L
#define SEND_UP_DATA_TM 10L
#define AUTO_CTRL_TM 60L

/* NOTIFICATION TIME CONTROLS */
int TIME_COUNTER_AT = 0;
int TIME_COUNTER_AH = 0;
int TIME_COUNTER_ATH = 0;
int TIME_COUNTER_SM = 0;
int EMAIL_COUNTER_AT = 0;
int EMAIL_COUNTER_AH = 0;
int EMAIL_COUNTER_ATH = 0;
int EMAIL_COUNTER_SM = 0;
int THING_COUNTER = 0;

/* Relays */
#define PUMP_PIN D6 //GPIO12
boolean status_pump = 0;

/* Soil Moisture Sensor */
#define SMPIN A0
#define SMVCC D4
int sM = 0;
int soilMoisture = 0;

/* DHT 22*/
#define DHTPIN D7     // Digital pin connected to the DHT sensor - GPIO13 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.
long airH = 0;
long airT = 0;

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)


/* Network Settings */
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "Enter your own BLYNK auth token";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Your Wi-Fi SSID";
char pass[] = "Your Wi-Fi password";

//Inisialisasi untuk ThingSpeak
unsigned long APIKEY = APIKEY;
const char * myWriteAPIKey = "Enter your own ThingSpeak API Key";
const int fieldNumber_airTemp = 1; //field number yang akan di isi dengan data
const int fieldNumber_airHum = 2; //field number yang akan di isi dengan data
const int fieldNumber_soilM = 3; //field number yang akan di isi dengan data

WiFiClient client; 

WidgetLCD lcdDhtT(V1);
WidgetLCD lcdDhtH(V2);
WidgetLCD lcdSm(V3);
WidgetLED ledPump(V5);

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

void sendThingSpeakHumData(long humidity){
  ThingSpeak.writeField(NMOWLKD28NPXPBRZ, fieldNumber_airHum, humidity, myWriteAPIKey); 
  delay(15000); //Free user hanya bisa mengupdate setiap 15 detik
}

void sendThingSpeakTempData(long temperature){
  ThingSpeak.writeField(NMOWLKD28NPXPBRZ, fieldNumber_airTemp, temperature, myWriteAPIKey); 
  delay(15000); //Free user hanya bisa mengupdate setiap 15 detik
}

void sendThingSpeakSoilData(long moisture){
  ThingSpeak.writeField(NMOWLKD28NPXPBRZ, fieldNumber_soilM, moisture, myWriteAPIKey); 
  delay(15000); //Free user hanya bisa mengupdate setiap 15 detik
}

void notifyAirTemp()
{
  Blynk.notify("Air temperature is below 31.0°C");
  if(EMAIL_COUNTER_AT / 60 == 12)
  {
   String body = String("Air temperature was below 31.0°C");
   Blynk.email("larrydennis.ltoruan@gmail.com", "Subject: Air Temperature Sensor", body); 
  }
  EMAIL_COUNTER_AT++;
}

void notifyAirHum()
{
  Blynk.notify("Air humidity is below 66%");
  if(EMAIL_COUNTER_AH / 60 == 12)
  {
    String body = String("Air temperature was below 66%");
    Blynk.email("larrydennis.ltoruan@gmail.com", "Subject: Air Humidity Sensor", body);
  }
  EMAIL_COUNTER_AH++;
}

void notifyAirTempHum()
{
  Blynk.notify("Air temperature is below 31.0 °C & humidity is below 66%");
  if(EMAIL_COUNTER_ATH / 60 == 12)
  {
   String body = String("Air temperature was below 31.0 °C and air humidity was below 66%");
   Blynk.email("larrydennis.ltoruan@gmail.com", "Subject: Air & Humidity Temperature Sensor", body); 
  }
  EMAIL_COUNTER_ATH++;
}

void notifySoilMos()
{
  Blynk.notify("Soil moisture is below 17.5%");
  if(EMAIL_COUNTER_SM / 60 == 12)
  {
   String body = String("Soil moisture was below 17.5%");
   Blynk.email("larrydennis.ltoruan@gmail.com", "Subject: Soil Moisture Sensor", body); 
  }
  EMAIL_COUNTER_SM++;
}

void getSoilMoistureData()
{
  long sMoisture;
  sMoisture = sM;
  lcdSm.clear();
  lcdSm.print(0, 0, "Moisture    :  %");

  digitalWrite (SMVCC, HIGH);
  delay (500);
  int N = 3;
  for(int i = 0; i < N; i++) // read sensor "N" times and get the average
  {
    sM += analogRead(SMPIN);   
    delay(150);
  } 

  digitalWrite (SMVCC, LOW);
  sM = sM / N;
  sM = sM / 10;
  soilMoisture = sM;
  if(sM < 17.5)
  {
    Serial.print(F("Soil Moisture : "));
    Serial.print(sM);
    Serial.println(F("%"));
    lcdSm.print(5, 1, sM);
    if(TIME_COUNTER_SM / 60 == 5)
    {
      notifySoilMos();
      TIME_COUNTER_SM = 0;
    }
    TIME_COUNTER_SM++;
    sM = map(sMoisture, 600, 0, 0, 100);
  }
  else
  {
    TIME_COUNTER_SM = 0;
    Serial.print(F("Soil Moisture : "));
    Serial.print(sM);
    Serial.println(F("%"));
    lcdSm.print(5, 1, sM);
    sM = map(sMoisture, 600, 0, 0, 100); 
  }

  if(THING_COUNTER % 10 == 0)
  {
    sendThingSpeakSoilData(soilMoisture);
    THING_COUNTER = 0;
  }
  THING_COUNTER++;
}

void getDhtData()
{
  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  lcdDhtT.clear();
  lcdDhtH.clear();
  lcdDhtT.print(0, 0, "Temperature : °C");
  lcdDhtH.print(0, 0, "Humidity    :  %");

  // use: (position X: 0-15, 0-15, position Y:0-1, "Message you want to print")
  long temperature, humidity;
  airH = event.relative_humidity;
  airT = event.temperature;

  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else if(event.temperature < 31.00)
  {
    delay(2000);
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    delay(2000);
    lcdDhtT.print(5, 1, airT);
    if(TIME_COUNTER_AT / 60 == 5)
    {
      notifyAirTemp();
      TIME_COUNTER_AT = 0;
    }
    TIME_COUNTER_AT++;
  }
  else 
  {
    TIME_COUNTER_AT = 0;
    TIME_COUNTER_ATH = 0;
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    lcdDhtT.print(5, 1, airT);
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else if(event.relative_humidity < 17.50)
  {
    delay(2000);
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    delay(2000);
    lcdDhtH.print(5, 1, event.relative_humidity);
    if(TIME_COUNTER_AH / 60 == 5)
    {
      notifyAirHum();
      TIME_COUNTER_AH = 0;
    }
    TIME_COUNTER_AH++;
  }
  else {
    TIME_COUNTER_AH = 0;
    TIME_COUNTER_ATH = 0;
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    lcdDhtH.print(5, 1, event.relative_humidity);
  }

  if(event.temperature < 31.00 && event.relative_humidity < 66.00)
  {
    if(TIME_COUNTER_ATH / 60 == 5)
    {
      notifyAirTempHum(); 
      TIME_COUNTER_ATH = 0;
    } 
    TIME_COUNTER_ATH++;
  }

    if(THING_COUNTER % 10 == 0)
  {
    sendThingSpeakTempData(airT);
    sendThingSpeakHumData(airH);
    THING_COUNTER = 0;
  }
  THING_COUNTER++;
}

void sensorTimers()
{
  timer.setInterval(READ_AIR_DATA_TM*1000, getDhtData);
  timer.setInterval(READ_SOIL_HUM_TM*1000, getSoilMoistureData);
  timer.setInterval(AUTO_CTRL_TM*1000, autoControlPlantation);
}

void setup()
{
  // Debug console
  Serial.begin(9600);

  // Set pin modes
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(SMVCC, OUTPUT);
  
  // Initialize device.
  dht.begin();

  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1500;

  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(SMVCC, LOW);
  
//  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8080);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

   lcdDhtT.clear();
   lcdDhtH.clear();
   lcdDhtT.print(0, 0, "Temperature : °C");
   lcdDhtH.print(0, 0, "Humidity    :  %");
  
   lcdSm.clear();
   lcdSm.print(0, 0, "Moisture    :  %");
   // use: (position X: 0-15, position Y: 0-1, "Message you want to print")
   // It will cause a FLOOD Error, and connection will be dropped

   sensorTimers();
   ThingSpeak.begin(client);
}

void loop()
{
  timer.run();
  Blynk.run();
}

/////////////////////////// Read remote commands //////////////////////////// 

BLYNK_WRITE(V4)
{
  //Pump remote control
  int i=param.asInt();
  if (i==1) 
  {
    status_pump = !status_pump;
    pumpControl();
  }
}

////////////////// Receive Commands for Actuators Controls //////////////////

void pumpControl()
{
  if (status_pump == 1) 
  {
    ledPump.off();
    Blynk.notify("Warning ! PUMP [OFF]"); 
    digitalWrite(PUMP_PIN, HIGH);
  }
  else
  {
    ledPump.on();
    Blynk.notify("Warning ! PUMP [ON]"); 
    digitalWrite(PUMP_PIN, LOW);
  }
}

//////////// Automatic control pump based on sensors reading /////////////

void autoControlPlantation()
{ 
  if (soilMoisture < SOIL_DRY && airT > TEMP_HOT) 
  {
    Serial.println("Automatic control activated !");
    status_pump = 1;
    pumpControl();
    delay (TIME_PUMP_ON*1000);
    status_pump = 0;
    pumpControl();
  }
}
