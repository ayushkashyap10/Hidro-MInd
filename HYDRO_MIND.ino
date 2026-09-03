#define BLYNK_PRINT Serial

// 🔥 Blynk Configuration
#define BLYNK_TEMPLATE_ID "blynk_template_id"
#define BLYNK_TEMPLATE_NAME "template name"
#define BLYNK_AUTH_TOKEN "YOUR_REAL_TOKEN"
// 📚 Libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// 📶 WiFi Credentials
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";
String weatherApiKey = "YOUR_OPENWEATHER_API_KEY";
String city = "Jalandhar,IN";

// 🔧 Pin Definitions
#define DHTPIN 18
#define DHTTYPE DHT11

#define SOIL 34
#define RELAY 23
#define PIR 19
#define BUZZER 25
#define AUTO_LED 26
#define PIR_LED 27
#define WEATHER_LED 33
#define LOCATION_LED 32

// 🌡️ DHT Setup
DHT dht(DHTPIN, DHTTYPE);

// ⏱️ Blynk Timer
BlynkTimer timer;

// AutoMode
bool autoMode = true;

// pirMode
bool pirMode = true;

// weather mode
String weatherCondition = "";
bool weatherMode = true;

// location mode
bool locationMode = false;

//location variable
float latitude = 0.0;
float longitude = 0.0;

String currentLocation = "";
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 🔔 PIR variables
int motionCount = 0;

unsigned long lastBuzzTime = 0;
const unsigned long cooldown = 1500;

// 📱 Pump control from Blynk App
BLYNK_WRITE(V12)
{ 
  if(!autoMode){
    int value = param.asInt();

    if (value == 1)
    {
      digitalWrite(RELAY, LOW); // Pump ON
      Serial.println("Pump ON from App");
      showPopup("PUMP", "ON FROM APP");
    }
    else
    {
      digitalWrite(RELAY, HIGH); // Pump OFF
      Serial.println("Pump OFF from App");
      showPopup("PUMP", "OFF FROM APP");
    }
  }

}

BLYNK_WRITE(V10)
{
  autoMode = param.asInt();
  digitalWrite(AUTO_LED, autoMode);

  if (autoMode)
  {
    Serial.println("AUTO MODE");
    showPopup("AUTO MODE", "ENABLED");
  }
  else
  {
    Serial.println("MANUAL MODE");
    showPopup("MANUAL MODE", "ENABLED");
  }
  
}


BLYNK_WRITE(V8)
{
  pirMode = param.asInt();
  digitalWrite(PIR_LED, pirMode);
  if (pirMode)
  {
    Serial.println("PIR MODE ON");
    showPopup("PIR MODE", "ON");
  }
  else
  {
    Serial.println("PIR MODE OFF");
    showPopup("PIR MODE", "OFF");
  }
}

BLYNK_WRITE(V30)
{
  weatherMode = param.asInt();

  digitalWrite(WEATHER_LED, weatherMode);

  if(weatherMode)
  {
    Serial.println("WEATHER MODE ON");
    showPopup("WEATHER MODE", "ON");
  }
  else
  {
    Serial.println("WEATHER MODE OFF");
    showPopup("WEATHER MODE", "OFF");
  }
}

BLYNK_WRITE(V40)
{
  if(!locationMode)
  {
    city = param.asString();

    Serial.print("City Changed To: ");

    Serial.println(city);
    showPopup("CITY UPDATED", city);
    getWeather();
  }
}

BLYNK_WRITE(V60)
{
  locationMode = param.asInt();
  digitalWrite(LOCATION_LED, locationMode);

  if(locationMode)
  {
    Serial.println("GPS LOCATION MODE");
    showPopup("GPS MODE", "ACTIVE");
    Blynk.virtualWrite(V40, "");
  }
  else
  {
    Serial.println("MANUAL CITY MODE");
    showPopup("CITY MODE", "ACTIVE");
  }
  if(!locationMode)
  {
    getWeather();
  }
}

BLYNK_WRITE(V50)
{
  latitude = param.asFloat();
}

BLYNK_WRITE(V51)
{
  longitude = param.asFloat();
  Serial.print("Latitude: ");
  Serial.println(latitude);

  Serial.print("Longitude: ");
  Serial.println(longitude);

  if(locationMode &&
    latitude != 0.0 &&
    longitude != 0.0)
  {
    getWeather();
  }
}

void getWeather()
{
  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient http;

  String url;
  if(locationMode)
  {
    url =
    "https://api.openweathermap.org/data/2.5/weather?lat=" +
    String(latitude, 6) +
    "&lon=" +
    String(longitude, 6) +
    "&appid=" +
    weatherApiKey +
    "&units=metric";
  }
  else
  {
    url =
    "https://api.openweathermap.org/data/2.5/weather?q=" +
    city +
    "&appid=" +
    weatherApiKey +
    "&units=metric";
  }

  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    String payload = http.getString();

    Serial.println("Weather Data:");

    DynamicJsonDocument doc(2048);

    deserializeJson(doc, payload);
    if(doc.isNull())
    {
      Serial.println("JSON Parse Failed!");

      return;
    }

    float outsideTemp =
    doc["main"]["temp"];

    int outsideHumidity =
    doc["main"]["humidity"];

    weatherCondition =
    doc["weather"][0]["main"].as<String>();

    currentLocation =
    doc["name"].as<String>();

    Serial.print("Outside Temp: ");
    Serial.println(outsideTemp);

    Serial.print("Outside Humidity: ");
    Serial.println(outsideHumidity);

    Serial.print("Weather: ");
    Serial.println(weatherCondition);

    Serial.print("Location: ");
    Serial.println(currentLocation);

    // Send weather to Blynk
    Blynk.virtualWrite(V20, outsideTemp);
    Blynk.virtualWrite(V21, outsideHumidity);
    Blynk.virtualWrite(V22, weatherCondition);
    Blynk.virtualWrite(V41, currentLocation);
  }
  else
  {
    Serial.println("Weather API Failed!");
  }

  http.end();
}

// 📡 Send Sensor Data to Blynk
void sendSensor()
{
  // 🌡️ Read Sensors
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum))
  {
    Serial.println("DHT Read Failed!");

    return;
  }

  int soil = analogRead(SOIL);

  int motion = digitalRead(PIR);

  // 📲 Send to Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V3, soil);
  Blynk.virtualWrite(V5, motion);

  // 🔍 Serial Monitor
  Serial.print("Temp: ");
  Serial.print(temp);

  Serial.print(" °C  Humidity: ");
  Serial.print(hum);

  Serial.print(" %  Soil: ");
  Serial.print(soil);

  Serial.print("  Motion: ");
  Serial.println(motion);
  if (autoMode)
  {
    // 🌦️ WEATHER MODE ENABLED
    if(weatherMode)
    {
      if(weatherCondition == "Rain" ||
      weatherCondition == "Drizzle" ||
      weatherCondition == "Thunderstorm")
      {
        digitalWrite(RELAY, HIGH);

        Serial.println("Rain Expected -> Pump OFF");

        Blynk.virtualWrite(V12, 0);
      }
      else
      {
        if (soil > 3000)
        {
          digitalWrite(RELAY, LOW);

          Serial.println("Smart Weather: Pump ON");

          Blynk.virtualWrite(V12, 1);
        }
        else
        {
          digitalWrite(RELAY, HIGH);

          Serial.println("Smart Weather: Pump OFF");

          Blynk.virtualWrite(V12, 0);
        }
      }
    }

    // 🌱 NORMAL SOIL IRRIGATION
    else
    {
      if (soil > 3000)
        {
          digitalWrite(RELAY, LOW);

          Serial.println("Normal AUTO: Pump ON");

          Blynk.virtualWrite(V12, 1);
        }
        else
        {
          digitalWrite(RELAY, HIGH);

          Serial.println("Normal AUTO: Pump OFF");

          Blynk.virtualWrite(V12, 0);
        }
      }
  }

  // 🔔 PIR + Buzzer Logic
  if (pirMode)
  {
    if (motion == HIGH)
    {
      motionCount++;
    }

    else
    {
      motionCount = 0;
    }
  }
  else
  {
    motionCount = 0;
    digitalWrite(BUZZER, LOW);
  }

  // ✅ Motion confirmed
  if (motionCount >= 1 && millis() - lastBuzzTime > cooldown)
  {
    Serial.println("Motion Confirmed!");

    // 🔊 Buzzer Pattern
    for (int i = 0; i < 4; i++)
    {
      digitalWrite(BUZZER, HIGH);
      delay(150);

      digitalWrite(BUZZER, LOW);
      delay(150);
    }

    lastBuzzTime = millis();
  }
}

bool popupActive = false;

String popupLine1 = "";
String popupLine2 = "";

unsigned long popupStart = 0;

const unsigned long popupDuration = 2000;

void showPopup(String line1, String line2)
{
  popupLine1 = line1;
  popupLine2 = line2;

  popupActive = true;

  popupStart = millis();

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print(line1);

  lcd.setCursor(0,1);
  lcd.print(line2);
}

void updateLCD()
{
  if(popupActive)
  {
    if(millis() - popupStart < popupDuration)
    {
      return;
    }
    else
    {
      popupActive = false;
      lcd.clear();
    }
  }
  static int screen = 0;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int soil = analogRead(SOIL);

  lcd.setCursor(0,0);
  lcd.print("                ");

  lcd.setCursor(0,1);
  lcd.print("                ");

  // SCREEN 1
  if(screen == 0)
  {
    lcd.setCursor(0,0);
    lcd.print("Temp:");
    lcd.print(temp,1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0,1);
    lcd.print("Hum:");
    lcd.print(hum,0);
    lcd.print("%");
  }

  // SCREEN 2
  else if(screen == 1)
  {
    lcd.setCursor(0,0);
    lcd.print("Soil:");
    lcd.print(soil);

    lcd.setCursor(0,1);

    if(digitalRead(RELAY) == LOW)
    {
      lcd.print("Pump: ON");
    }
    else
    {
      lcd.print("Pump: OFF");
    }
  }

  // SCREEN 3
  else if(screen == 2)
  {
    lcd.setCursor(0,0);
    lcd.print("Weather:");

    lcd.setCursor(0,1);
    lcd.print(weatherCondition);
  }

  // SCREEN 4
  else if(screen == 3)
  {
    lcd.setCursor(0,0);

    if(autoMode)
    {
      lcd.print("AUTO MODE");
    }
    else
    {
      lcd.print("MANUAL MODE");
    }

    lcd.setCursor(0,1);

    if(weatherMode)
    {
      lcd.print("WEATHER ON");
    }
    else
    {
      lcd.print("WEATHER OFF");
    }
  }

  // SCREEN 5
  else if(screen == 4)
  {
    lcd.setCursor(0,0);

    if(pirMode)
    {
      lcd.print("PIR: ON");
    }
    else
    {
      lcd.print("PIR: OFF");
    }

    lcd.setCursor(0,1);

    if(locationMode)
    {
      lcd.print("GPS MODE");
    }
    else
    {
      lcd.print(city);
    }
  }

  // SCREEN 6
  else if(screen == 5)
  {
    lcd.setCursor(0,0);
    lcd.print("Location:");

    lcd.setCursor(0,1);
    lcd.print(currentLocation);
  }

  screen++;

  if(screen > 5)
  {
    screen = 0;
  }
}

void setup()
{
  // 🔍 Serial Monitor
  Serial.begin(115200);

  // 🌡️ DHT Start
  dht.begin();

  Wire.begin(21, 22);

  lcd.init();

  lcd.backlight();

  lcd.setCursor(3,0);
  lcd.print("HYDRO MIND");
  lcd.setCursor(0,1);
  lcd.print("SMART IRRIGATION");
  delay(2000);

  // 🔌 Pin Modes
  pinMode(RELAY, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(AUTO_LED, OUTPUT);
  pinMode(PIR_LED, OUTPUT);
  pinMode(WEATHER_LED, OUTPUT);
  pinMode(LOCATION_LED, OUTPUT);

  // OFF initially
  digitalWrite(RELAY, HIGH);
  digitalWrite(AUTO_LED, HIGH);
  digitalWrite(PIR_LED, HIGH);
  digitalWrite(WEATHER_LED, HIGH);
  digitalWrite(LOCATION_LED, LOW);

  // 📶 Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Blynk.syncAll();
  delay(1000);
  getWeather();
  // ⏱️ Timer
  timer.setInterval(2000L, sendSensor);
  timer.setInterval(600000L, getWeather);
  timer.setInterval(3000L, updateLCD);
  Serial.println("System Started!");
  lcd.setCursor(0,1);
  lcd.print("SYSTEM  STARTED!");
  delay(3000);
  lcd.clear();
}

void loop()
{
  Blynk.run();

  timer.run();
}
