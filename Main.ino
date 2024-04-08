#include <FirebaseESP8266.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include "DHT.h"
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "NTP.h"
#include <WiFiNINA.h> 
#include "Arduino.h"
#define DHT_TYPE DHT22
#define FIREBASE_AUTH "AIzaSyCV2oxI6kTA2i34GOublb2xbL1syrrppx0"
#define FIREBASE_HOST "https://wetterstation-9a964-default-rtdb.europe-west1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define USER_EMAIL "test@janosch.de"
#define USER_PASSWORD "29+=112008Joy&"
#define WIFI_SSID "Fritz Routi"
#define WIFI_PASSWORD "Elias01.12.2020"
const char *host = "www.schrotthandel-moeller.de";
const int httpsPort = 443;
float currentVersion = 5.0;
float latestVersion;
double dewPoint;
FirebaseData fb;
WiFiClientSecure client;
FirebaseAuth auth;
FirebaseConfig config; 

Adafruit_BMP280 bmp;
Adafruit_BME280 bme;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

const int DHT_PIN = 12; // DHT-Sensor
#define ONE_WIRE_BUS 13  // Dallas Temperature

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DHT dht(DHT_PIN, DHT_TYPE);

#define TCAADDR 0x70


void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

void setup()
{
  Serial.begin(115200);         
  Serial.println('\n');
  Wire.begin();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID); 
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { 
    
    delay(1500);
    Serial.println(".");

    if(i > 10){

      ESP.restart();
    }
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());  
  Serial.println("Verbunden!");                                                

  config.api_key = FIREBASE_AUTH;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = FIREBASE_HOST; 

  Firebase.begin(&config, &auth);
  //Enable auto reconnect the WiFi when connection lost
  Firebase.reconnectWiFi(true);
 
  ntp.ruleDST("CEST", Last, Sun, Mar, 2, 120); // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
  ntp.ruleSTD("CET", Last, Sun, Oct, 3, 60); // last sunday in october 3:00, timezone +60min (+1 GMT)
  ntp.begin();

   sensorState();

    /* Default settings from datasheet.  */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                 Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                 Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                 Adafruit_BMP280::STANDBY_MS_500);  /* Standby time. */

  //checkForUpdate();
}

void sensorState() {

  Wire.begin();
      
 
  bool allSensorsWorking = true;

  sensors.begin();
  float tempC = sensors.getTempCByIndex(0);


  if (tempC == DEVICE_DISCONNECTED_C) {
    Firebase.setString( fb,"/sensorStates/DS18b20", "Der DS18b20 funktioniert nicht!");
    Serial.println("Der DS18b20 funktioniert nicht!");
    allSensorsWorking = false;
  }
  
    tcaselect(1); // Select multiplexer channel 2 for 

  if (!mlx.begin()) {
    Firebase.setString(fb, "/sensorStates/MLX90614", "Der MLX90614 funktioniert nicht!");
    Serial.println("Der MLX funktioniert nicht!");

    allSensorsWorking = false;
  }

    tcaselect(2); // Select multiplexer channel 2 for 

  
  if (!bmp.begin()) {
    Firebase.setString(fb, "/sensorStates/BMP", "Der BMP280 funktioniert nicht!");
    Serial.println("Der BMP280 funktioniert nicht!");
    allSensorsWorking = false;
  }

    dht.begin();

  if (isnan(dht.readTemperature()) || isnan(dht.readHumidity())) {
    Firebase.setString(fb,"/sensorStates/DHT22", "Der DHT22 funktioniert nicht!");
    Serial.println("Der DHT funktioniert nicht!");

    allSensorsWorking = false;
  }

    
      tcaselect(0);

  if(!bme.begin(0x76)){

    Firebase.setString(fb, "/sensorStates/BME280", "Der BME280 funktioniert nicht!");
    Serial.println("Der BME280 funktioniert nicht!");

    allSensorsWorking = false;

  }

  if (allSensorsWorking) {
    Firebase.setString(fb, "/sensorStates/DS18b20", "OK");
    Firebase.setString(fb, "/sensorStates/MLX90614", "OK");
    Firebase.setString(fb, "/sensorStates/BMP280", "OK");
    Firebase.setString(fb, "/sensorStates/DHT22", "OK");
    Firebase.setString(fb, "/sensorStates/BME280", "OK");


  }
}

void loop() {

   sensors.requestTemperatures(); // Send the command to get temperatures

  double gamma = log(dht.readHumidity() / 100) + ((17.62 *  sensors.getTempCByIndex(0)) / (243.5 +  sensors.getTempCByIndex(0)));
  dewPoint = 243.5 * gamma / (17.62 - gamma);

   updateFirebase();
  
   //startdeepSleep();

}


void updateFirebase() {

  tcaselect(1);
  Serial.println(mlx.readObjectTempC());
  Serial.println(mlx.readAmbientTempC());


  float readObjectTemp = mlx.readObjectTempC();
  float readAmbientTemp = mlx.readAmbientTempC();



  Firebase.pushFloat(fb, "/MLX90614/Cloudtemperature",readObjectTemp);
  Firebase.pushFloat(fb, "/MLX90614/Temperature in C",readAmbientTemp);



  tcaselect(2);
  Firebase.pushFloat(fb, "/BMP280/Pressure", roundf(bmp.readPressure()/100000 * 100)/100);
  Firebase.pushFloat(fb,"/BMP280/Temperature", roundf((bmp.readTemperature()-1.0) * 100) / 100);
  Firebase.pushFloat(fb, "/BMP280/Altitude", roundf(bmp.readAltitude(1013.25)));


  tcaselect(0);
  Firebase.pushFloat(fb, "/BME280/Temperature", bme.readTemperature());
  Firebase.pushFloat(fb, "/BME280/Humidity", bme.readHumidity());
  Firebase.pushFloat(fb, "/BME280/Altitude", bme.readAltitude(1013.25));


  Firebase.pushFloat(fb, "/Others/dew Point", roundf(dewPoint * 100) / 100);
  Firebase.pushFloat(fb, "/DS18b20/Temperature", roundf(sensors.getTempCByIndex(0) * 100) / 100);
  Firebase.pushFloat(fb, "/Others/Heat index", dht.computeHeatIndex(sensors.getTempCByIndex(0), dht.readHumidity(), false));
  Firebase.pushFloat(fb, "/DHT22/Temperature",roundf(dht.readTemperature() *100) / 100);
  Firebase.pushFloat(fb, "/DHT22/Humidity", dht.readHumidity());


  ntp.update();
  Firebase.pushString(fb, "/station/LastMeasurements/", String("Letzte Messung am ") + ntp.formattedTime("%d. %B %Y") + " um " + ntp.formattedTime("%T"));

  Serial.println();



}




void startdeepSleep(){

   digitalWrite(1, LOW);
   ESP.deepSleep(30 * 60 * 1000000); //Für 30 Minuten geht der ESP8266 schlafen
  
}
