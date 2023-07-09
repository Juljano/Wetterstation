#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <Adafruit_MLX90614.h>
#include  <Adafruit_BMP280.h>
#include "DHT.h" 
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#define DHT_TYPE DHT22 
#include <OneWire.h>
#include <DallasTemperature.h>
#define FIREBASE_HOST "https://wetterstation-9a964-default-rtdb.europe-west1.firebasedatabase.app/" 
#define FIREBASE_AUTH "AIzaSyCV2oxI6kTA2i34GOublb2xbL1syrrppx0"
#define WIFI_SSID "Fritz Routi" 
#define WIFI_PASSWORD "Elias01.12.2020" 
const char* host = "www.schrotthandel-moeller.de";
const int httpsPort = 443;
float currentVersion = 3.5;
float latestVersion;
double dewPoint;
FirebaseData fb;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Adafruit_BMP280 bmp; 
WiFiClientSecure client;
const int DHT_PIN = 14; 
#define ONE_WIRE_BUS D7

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {

 Serial.begin(115200);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA); 
  
 int retries = 0;
while (WiFi.status() != WL_CONNECTED)
{
  Serial.print(".");
  delay(300);
  retries++;
  if (retries > 10) {
    Serial.println("Connection failed");
    ESP.reset();
  }
}

  Serial.println("Verbunden!");                                                
  pinMode(LED_BUILTIN, OUTPUT);

   Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Enable auto reconnect the WiFi when connection lost
   Firebase.reconnectWiFi(true);



  /* Default settings from datasheet.  */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);  /* Standby time. */

  

   sensorState();

  checkForUpdate();


}

void loop() {

    sensors.requestTemperatures(); // Send the command to get temperatures


   digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);   

  //calculate the dewPoint
  double gamma = log(dht.readHumidity() / 100) + ((17.62 *  sensors.getTempCByIndex(0)) / (243.5 +  sensors.getTempCByIndex(0)));
  dewPoint = 243.5 * gamma / (17.62 - gamma);

  updateFirebase();
  startdeepSleep();
}


void updateFirebase() {
  
  Firebase.pushFloat(fb, "/DHT22/Temperature",dht.readTemperature());
  Firebase.pushFloat(fb, "/DHT22/Humidity", dht.readHumidity());
  Firebase.pushFloat(fb, "/BMP280/Pressure", bmp.readPressure()/100000);
  Firebase.pushFloat(fb, "/BMP280/Temperature", bmp.readTemperature()-1.0);
  Firebase.pushFloat(fb, "/BMP280/Altitude", bmp.readAltitude(1021.04));
  Firebase.pushFloat(fb, "/Others/dew Point", dewPoint);
  Firebase.pushFloat(fb, "/DS18b20/Temperature", sensors.getTempCByIndex(0));
  Firebase.pushFloat(fb, "/Others/Heat index", dht.computeHeatIndex(sensors.getTempCByIndex(0), dht.readHumidity(), false));

    Serial.println();

  Serial.println(mlx.readObjectTempC());
  Firebase.pushFloat(fb, "/MLX90614/Cloudtemperature",mlx.readObjectTempC());
  Firebase.pushFloat(fb, "/MLX90614/Temperature in C",mlx.readObjectTempC());
  Firebase.pushFloat(fb, "/MLX90614/Temperature in F",mlx.readObjectTempF());


}

void startdeepSleep(){


   Serial.println("Tiefschlaf-Modus");
   ESP.deepSleep(1800*1e6); //Für eine halbe Stunde geht der ESP schlafen
  
}

void sensorState() {
  sensors.begin();
  dht.begin();

  float tempC = sensors.getTempCByIndex(0);
  bool allSensorsWorking = true;

  if (tempC == DEVICE_DISCONNECTED_C) {
    Firebase.setString(fb, "/sensorStates/DS18b20", "Der DS18b20 funktioniert nicht!");
    allSensorsWorking = false;
  }

  if (!mlx.begin()) {
    Firebase.setString(fb, "/sensorStates/MLX90614", "Der MLX90614 funktioniert nicht!");
    allSensorsWorking = false;
  }

  if (!bmp.begin()) {
    Firebase.setString(fb, "/sensorStates/BMP", "Der BMP280 funktioniert nicht!");
    allSensorsWorking = false;
  }

  if (isnan(dht.readTemperature()) || isnan(dht.readHumidity())) {
    Firebase.setString(fb, "/sensorStates/DHT22", "Der DHT22 funktioniert nicht!");
    allSensorsWorking = false;
  }

  if (allSensorsWorking) {
    Firebase.setString(fb, "/sensorStates/DS18b20", "OK");
    Firebase.setString(fb, "/sensorStates/MLX90614", "OK");
    Firebase.setString(fb, "/sensorStates/BMP", "OK");
    Firebase.setString(fb, "/sensorStates/DHT22", "OK");
  }
}
