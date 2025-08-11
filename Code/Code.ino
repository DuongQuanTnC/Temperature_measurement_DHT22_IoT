#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char *mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char *accessToken = "bRSp0xyi0Ippog3FfM1K";  // Thay đổi YOUR_ACCESS_TOKEN

const char* telemetryTopic = "v1/devices/me/telemetry";
const char* attributeTopic = "v1/devices/me/attributes";
const char* requestAttr = "v1/devices/me/attributes/request/1";
const char* responseAttr = "v1/devices/me/attributes/response/+";

#define DHTPIN 4        // Pin sensor DHT22
#define DHTTYPE DHT22   // Tipo sensor
#define LDR_PIN 34      // A1 - LDR (tín hiệu tương tự)
#define NTC_PIN 35      // A2 - NTC (tín hiệu tương tự)
#define BUTTON_PIN 18   // Nút nhấn (Digital Input)

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long now;

unsigned long lastAnalogRead = 0;
unsigned long lastAnalogReport = 0;
unsigned long lastDHTRead = 0;

int analogRawValue = 0;
int ldrFiltered = 0;
int ntcFiltered = 0;
unsigned long lastTelemetry = 0, lastAttrRead = 0;

float temp = 0, hum = 0;

// Trạng thái
String prevState = "";
int threshold = 25;
String state = "Off";

void sendStateAttribute() {
  StaticJsonDocument<128> doc;
  doc["state"] = state;
  String json;
  serializeJson(doc, json);
  client.publish(attributeTopic, json.c_str());
}

void requestThreshold() {
  client.publish(requestAttr, "{\"sharedKeys\":\"Threshold\"}");
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error && doc["Threshold"]) {
    threshold = doc["Threshold"];
    Serial.print("Threshold received: ");
    Serial.println(threshold);
  }
}

float computeNTCTemperature(int ntc_raw) {
  float voltage = ntc_raw * 3.3 / 4095.0;

  float R_FIXED = 10000.0;
  float ntc_resistance = (voltage * R_FIXED) / (3.3 - voltage);

  float BETA = 3950;
  float T0 = 298.15; // K
  float R0 = 10000.0;

  float tempK = 1.0 / (1.0/T0 + (1.0/BETA) * log(ntc_resistance / R0));
  float tempC = tempK - 273.15;

  return tempC;
}


void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);
  pinMode(NTC_PIN, INPUT);


  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Kết nối với máy chủ thông qua giao thức MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback); 
  
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  now = millis();
 // ==== Đọc cảm biến analog (LDR, NTC) mỗi 500ms ====
  if (now - lastAnalogRead >= 500) {
    lastAnalogRead = now;

    int rawLDR = analogRead(LDR_PIN);
    int rawNTC = analogRead(NTC_PIN);

    float temC = computeNTCTemperature(rawNTC);
    
    // Lọc trung bình cộng
    ldrFiltered = (ldrFiltered * 3 + rawLDR) / 4;
    ntcFiltered = (ntcFiltered * 3 + rawNTC) / 4;
  }

  // ==== Xuất dữ liệu cảm biến analog mỗi 1 giây ====
  if (now - lastAnalogReport >= 1000) {
    lastAnalogReport = now;

    Serial.print("LDR Filtered: ");
    Serial.print(ldrFiltered);
    Serial.print(" | NTC Filtered: ");
    Serial.println(ntcFiltered);
  }

  // === Đọc DHT11 mỗi 2s ===
  if (now - lastDHTRead >= 2000) {
    lastDHTRead = now;

    temp = dht.readTemperature();
    hum = dht.readHumidity();

    if (!isnan(temp) && !isnan(hum)) {
      Serial.print("Temp: ");
      Serial.print(temp);
      Serial.print(" C, Humidity: ");
      Serial.print(hum);
      Serial.println(" %");
    } else {
      Serial.println("Failed to read from DHT11");
    }
  }

  // Gửi Telemetry mỗi 5s
  if (now - lastTelemetry >= 5000) {
    StaticJsonDocument<256> doc;
    doc["ldr"] = ldrFiltered;
    doc["ntc"] = ntcFiltered;
    doc["temp"] = temp;
    doc["humi"] = hum;
    String json;
    serializeJson(doc, json);

    client.publish(telemetryTopic, json.c_str());
    lastTelemetry = now;
  }

  // Gửi lại trạng thái nếu có thay đổi
  if (digitalRead(BUTTON_PIN) == LOW) {
    state = (state == "On") ? "Off" : "On";
    Serial.print("state: ");
    Serial.println(state);
    delay(300); // debounce
  }

  if (state != prevState) {
    sendStateAttribute();
    prevState = state;
  }

  // Đọc Threshold mỗi 60s
  if (now - lastAttrRead >= 60000) {
    requestThreshold();
    lastAttrRead = now;
  }
  
}
  

//hàm kiểm tra và thực hiện kết nối lại với máy chủ TB
void reconnect() {
  // kiểm tra kết nối với TB, nếu kết nối bị ngắt sẽ thực hiện kết nối lại với máy chủ
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32Client", accessToken, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//process resquest and response message from TB server
// void tb_callback(char* topic, byte* payload, unsigned int length){
//   Serial.println(F("On TB message"));

//   char* ctrl_json = new char[length + 1];
//   strncpy (ctrl_json, (char*)payload, length);
//   ctrl_json[length] = '\0';
      
//   //debug
//   //topic: "v1/devices/me/rpc/request/$request_id"  
//   Serial.print(F("Topic: ")); Serial.println(topic); Serial.print(F("Message: ")); Serial.println(ctrl_json);
                        
//   // DeserializationError error = deserializeJson(jsonBuffer, ctrl_json);
//   // if(error){
//   //   Serial.println(F("parseObject() failed"));
//   //   return;
//   // }                  
// }