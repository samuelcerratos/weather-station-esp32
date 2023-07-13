#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SEALEVELPRESSURE_HPA (1013.25)

//Your Domain name with URL path or IP address with path
String serverName = "https://api-weather-station-zp3k.onrender.com/api/v1/measurements";

const int capacity = JSON_OBJECT_SIZE(7);
StaticJsonDocument<capacity> doc;

const char* ssid = "karagdum";
const char* password = "Mofletes69.";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME680 bme;
SCD4x scd4x;

void connectToWiFi() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Internet connection");
  display.display();

  WiFi.begin(ssid, password);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    display.setCursor(0, 16);
    display.print("Connecting to: ");
    display.setCursor(0, 30);
    display.print(ssid);
    display.setCursor(0, 40);
    for (int i = 0; i < dots; i++) {
      display.print(".");
    }
    display.display();
    dots = (dots + 1) % 4;
  }

  // Mostrar información de la conexión en la pantalla OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("SSID: ");
  display.setCursor(34, 0);
  display.println(ssid);
  display.setCursor(0, 16);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.setCursor(0, 32);
  display.print("MAC:");
  display.println(WiFi.macAddress());

  display.setCursor(23, 55);
  display.println("-- CONNECTED --");
  display.display();
}

bool sensorsConnection() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Checking sensors:");

  bool bmeStatus = bme.begin();
  bool scd4xStatus = scd4x.begin();

  // Mostrar estado del sensor BME680
  display.setCursor(0, 20);
  display.print("BME680: ");
  if (bmeStatus) {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);  // 320*C for 150 ms
    display.println("OK");
  } else {
    display.println("ERROR");
  }

  // Mostrar estado del sensor SCD4x
  display.setCursor(0, 30);
  display.print("SCD40: ");
  if (scd4xStatus) {
    display.println("OK");
  } else {
    display.println("ERROR");
  }

  display.display();
  delay(3000);

  // Retornar estado de conexión de ambos sensores
  return bmeStatus && scd4xStatus;
}


void setup() {
  Serial.begin(115200);

  // Inicializar la pantalla OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("-STARTING CONTROLLER-");
  display.display();
  delay(2000);

  // Verificar la conexión de los sensores
  if (!sensorsConnection()) {
    // Si falla la conexión de los sensores, detener el programa
    while (true) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Sensors connection");
      display.setCursor(0, 16);
      display.println("failed.");
      display.display();
      delay(2000);
    }
  }

  // Conectar a la red WiFi
  connectToWiFi();

  delay(2000);
}

void loop() {
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  Serial.print(F("Reading started at "));
  Serial.print(millis());
  Serial.print(F(" and will finish at "));
  Serial.println(endTime);

  Serial.println(F("You can do other work during BME680 measurement."));
  delay(50);  // This represents parallel work.
  // There's no need to delay() until millis() >= endTime: bme.endReading()
  // takes care of that. It's okay for parallel work to take longer than
  // BME680's measurement time.

  // Obtain measurement results from BME680. Note that this operation isn't
  // instantaneous even if milli() >= endTime due to I2C/SPI latency.
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  if (scd4x.readMeasurement() && bme.endReading()) {

    float humidity = (bme.humidity + scd4x.getHumidity()) / 2;
    float pressure = bme.pressure / 100.0;
    float co2 = scd4x.getCO2();
    float temperature = (scd4x.getTemperature() + bme.temperature) / 2;
    float iaq = bme.gas_resistance / 1000.0;
    float no2 = 0.32;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Temp:");
    display.setCursor(57, 0);
    display.print(temperature);
    display.println(" C");

    display.setCursor(0, 11);
    display.println("Humidity:");
    display.setCursor(57, 11);
    display.print(humidity);
    display.println(" %");

    display.setCursor(0, 22);
    display.println("Pressure:");
    display.setCursor(57, 22);
    display.print(pressure);
    display.println(" hPa");

    display.setCursor(0, 33);
    display.println("Gas:");
    display.setCursor(57, 33);
    display.print(iaq);
    display.println(" KOhms");

    display.setCursor(0, 44);
    display.print("CO2: ");
    display.setCursor(57, 44);
    display.print(co2);
    display.println(" ppm");

    display.setCursor(0, 55);
    display.print("NO2: ");
    display.setCursor(57, 55);
    display.print(no2);
    display.println(" ppm");

    display.display();

 if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");

      doc["deviceId"].set("WST_1");
      doc["temperature"].set(temperature);
      doc["humidity"].set(humidity);
      doc["pressure"].set(pressure);
      doc["co2"].set(co2);
      doc["iaq"].set(iaq);
      doc["no2"].set(no2);

      String json;
      serializeJson(doc, json);

      Serial.print(json);

      int httpResponseCode = http.POST(json);


      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);


      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
       connectToWiFi();
    }
    delay(60000);
  }
}
