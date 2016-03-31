// Push DHT22 data from MRK1000 to Azure Event Hub

#include "DHT.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include <WiFi101.h>

#include <RTCZero.h>
#include "time.h"
#include "ssid.h"

RTCZero rtc;

#define DHTPIN 6
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Generate SAS key using https://github.com/sandrinodimattia/RedDog/releases
char hostname[] = "connectthedotsex-ns.servicebus.windows.net";
char authSAS[] = "SharedAccessSignature sr=https%3a%2f%2fconnectthedotsex-ns.servicebus.windows.net%2fehdevices%2fpublishers%2fd1%2fmessages&sig=OzFhN9z3ROSBqmaEz1em3DHIPsj7vcu3gY4BE60n%2fTo%3d&se=1460172816&skn=D1";

String hubName = "ehdevices";
String deviceName = "D1";
String uri = "/" + hubName + "/publishers/" + deviceName + "/messages";

int status = WL_IDLE_STATUS;
WiFiSSLClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("DHT22 Sensor for Microsoft Azure");

  // check for the presence of the shield :
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true); // stop
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    if (status != WL_CONNECTED) {
      // wait 10 seconds for connection:
      delay(10000);
    }
  }
  Serial.println("Connected to wifi");

  rtc.begin();
  setTimeUsingTimeServer(rtc);
}

void loop() {
  float temperature = dht.readTemperature(true); // true == Fahrenheit
  float humidity = dht.readHumidity();

  Serial.print(temperature);
  Serial.print(" *F ");
  Serial.print(humidity);
  Serial.println(" %");

  if (!(isnan(temperature) || isnan(humidity))) {
    sendEvent("Temperature", temperature, "F"); 
    sendEvent("Humidity", humidity, "%");     
  }

  delay(10000);
}

void sendEvent(String measurename, float value, String unitofmeasure) {

  String json = createJSON(measurename, value, unitofmeasure);
  httpPost(json);

  String response = "";
  char c;
  while (client.available()) {
    c = client.read();
    response.concat(c);
  }

  if (!response.equals("")) {
    if (response.startsWith("HTTP/1.1 201")) {
      Serial.println("Data sent to Azure"); 
    } else {
      Serial.println("Error posting " + json);
      Serial.println(response);
    }
  }

}

String createJSON(String measurename, float value, String unitofmeasure) {

  // ISO8601 date string
  char dateString[25];
  sprintf(dateString, "2%03d-%02d-%02dT%02d:%02d:%02d.000Z", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

  // JSON is based on Microsoft Connect the Dots example
  // https://github.com/Azure/connectthedots/blob/master/Introduction.md#device-basics
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["subject"] = "wthr";
  root["organization"] = "Foo Industries";
  root["displayname"] = "MKR1000";
  root["measurename"] = measurename;
  root["unitofmeasure"] = unitofmeasure;
  root["value"] = String(value);
  root["timecreated"] = dateString;
  root["guid"] = "123456";

  char json[200];
  root.printTo(json, sizeof(json));
  return String(json);
}

// Borrowed from https://github.com/mohankrr/Arduino/blob/master/MKR1000AzureReceiveFull/MKR1000AzureReceiveFull.ino#L152-L186
void httpPost(String content)
{
    // close any connection before send a new request.
    client.stop();
  
    if (client.connectSSL(hostname, 443)) {
      client.print("POST ");
      client.print(uri);
      client.println(" HTTP/1.1"); 
      client.print("Host: "); 
      client.println(hostname);
      client.print("Authorization: ");
      client.println(authSAS); 
      client.println("Connection: close");

      client.print("Content-Type: ");
      client.println("application/json");
      client.print("Content-Length: ");
      client.println(content.length());
      client.println();
      client.println(content);
      
      delay(200);
      
    } else {
      Serial.println("Connection failed");
    }
}

