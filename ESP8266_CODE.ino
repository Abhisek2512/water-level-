// ==== BLYNK CONFIGURATION ====
#define BLYNK_TEMPLATE_ID "🤡🤡🤡🤡🤡"
#define BLYNK_TEMPLATE_NAME "Water Tank Monitor"
#define BLYNK_AUTH_TOKEN  "🤡🤡🤡🤡🤡"
#define BLYNK_PRINT Serial

#define BLYNK_TIMEOUT_MS 5000
#define BLYNK_HEARTBEAT 15

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h>

// ==== WI-FI CREDENTIALS ====
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = " "🤡🤡🤡🤡🤡"";// wifi ssid
char pass[] = " "🤡🤡🤡🤡🤡"";  // Replace with your real password

// ==== PIN DEFINITIONS ====
#define TRIG_PIN 5    // D1 = GPIO5
#define ECHO_PIN 4    // D2 = GPIO4
#define RELAY_PIN 14  // D5 = GPIO14

// ==== TANK PARAMETERS ====
const float TANK_HEIGHT_FT = 6.6;
const float TANK_CAPACITY_LITERS = 5000.0;

BlynkTimer timer;
ESP8266WebServer server(80);  // Local web server

// ==== STATE VARIABLES ====
float g_percent_full = 0.0;
float g_volume_liters = 0.0;
bool g_relay_state = false;

int lowLevelCount = 0;
bool lowWaterAlertSent = false;

int highLevelCount = 0;
bool tankFullAlertSent = false;

// For tracking last Blynk values
float lastBlynkPercent = -1;
float lastBlynkVolume = -1;

// For averaging readings every 5 minutes
float sumPercent = 0.0;
float sumVolume = 0.0;
int readingCount = 0;

// For storing last 3 averaged percentages for alert logic
#define AVG_HISTORY_SIZE 3
float avgHistory[AVG_HISTORY_SIZE] = {0};
int avgIndex = 0;
bool avgHistoryFull = false;

// For sensor failure tracking
int sensorFailCount = 0;
const int MAX_SENSOR_FAIL = 5;

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF (active LOW)

  // Wi-Fi Connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Timeout after 15 seconds
    if (millis() - wifiStart > 15000) {
      Serial.println("\nWiFi connection timeout! Restarting...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Blynk Manual Connection
  Blynk.config(auth, "68.183.87.221", 8080);
  if (!Blynk.connect(5000)) {  // Try connecting with 5s timeout
    Serial.println("Blynk connection failed! Restarting...");
    ESP.restart();
  }
  Serial.println("Connected to Blynk!");

  // Web Server Endpoint
  server.on("/", []() {
    String html = "<html><head><meta http-equiv='refresh' content='2'/>";
    html += "<title>Water Tank Monitor</title></head><body>";
    html += "<h2>🚰 Water Tank Monitor</h2>";
    html += "<p><strong>Water Level:</strong> " + String(g_percent_full, 1) + "%</p>";
    html += "<p><strong>Volume:</strong> " + String(g_volume_liters, 0) + " liters</p>";
    html += "<p><strong>Relay:</strong> " + String(g_relay_state ? "ON" : "OFF") + "</p>";
    html += "<p>Updated: " + String(millis() / 1000) + "s</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  server.begin();
  Serial.println("Web server started at http://" + WiFi.localIP().toString());

  // Timers
  timer.setInterval(2000L, updateLocalData);     // every 2 seconds
  timer.setInterval(300000L, sendBlynkData);     // every 5 minutes (300,000 ms)
}

// ==== LOOP ====
void loop() {
  Blynk.run();
  timer.run();
  server.handleClient();

  checkWiFiConnection();
  checkBlynkConnection();
}

// ==== WiFi reconnect helper ====
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nWiFi reconnect failed. Restarting...");
      ESP.restart();
    } else {
      Serial.println("\nWiFi reconnected.");
    }
  }
}

// ==== Blynk reconnect helper ====
void checkBlynkConnection() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Attempting to reconnect...");
    if (!Blynk.connect(5000)) {  // try 5 seconds to reconnect
      Serial.println("Blynk reconnect failed. Restarting...");
      ESP.restart();
    }
    Serial.println("Blynk reconnected.");
  }
}

// ==== SENSOR FUNCTION with error handling ====
bool readTankLevel(float &percent_full, float &volume_liters) {
  long duration;
  float distance_cm, distance_ft;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    sensorFailCount++;
    Serial.println("Sensor read failed (timeout). Count: " + String(sensorFailCount));
    if(sensorFailCount >= MAX_SENSOR_FAIL) {
      Serial.println("Multiple sensor failures! Restarting...");
      ESP.restart();
    }
    return false;
  }

  sensorFailCount = 0; // reset on success

  distance_cm = (duration * 0.0343) / 2.0;
  distance_ft = distance_cm / 30.48;
  if (distance_ft > TANK_HEIGHT_FT) distance_ft = TANK_HEIGHT_FT;

  float water_height = TANK_HEIGHT_FT - distance_ft;
  percent_full = (water_height / TANK_HEIGHT_FT) * 100.0;
  volume_liters = (percent_full / 100.0) * TANK_CAPACITY_LITERS;
  return true;
}

// ==== 2-SECOND LOCAL UPDATE ====
void updateLocalData() {
  float percent, volume;
  if (!readTankLevel(percent, volume)) {
    Serial.println("Skipping local update due to sensor error.");
    return;
  }

  g_percent_full = percent;
  g_volume_liters = volume;

  // Accumulate readings for averaging
  sumPercent += percent;
  sumVolume += volume;
  readingCount++;

  // Relay control (with hysteresis)
  static bool relayState = false;
  if (!relayState && percent > 72.0) {
    relayState = true;
    digitalWrite(RELAY_PIN, LOW);  // ON (active LOW)
  } else if (relayState && percent < 68.0) {
    relayState = false;
    digitalWrite(RELAY_PIN, HIGH); // OFF
  }
  g_relay_state = relayState;

  Serial.print("Local Update - Water: ");
  Serial.print(percent, 1);
  Serial.print("% | ");
  Serial.print(volume, 0);
  Serial.println(" L");
}

// ==== 5-MINUTE BLYNK UPDATE ====
void sendBlynkData() {
  if (readingCount == 0) return;  // Prevent division by zero

  // Calculate averages
  float avgPercent = sumPercent / readingCount;
  float avgVolume = sumVolume / readingCount;

  // Reset accumulators for next period
  sumPercent = 0.0;
  sumVolume = 0.0;
  readingCount = 0;

  // Store average percent in rolling history
  avgHistory[avgIndex] = avgPercent;
  avgIndex = (avgIndex + 1) % AVG_HISTORY_SIZE;
  if (avgIndex == 0) avgHistoryFull = true;

  // Send to Blynk only if significant change
  if (abs(avgPercent - lastBlynkPercent) >= 1.0) {
    Blynk.virtualWrite(V0, avgPercent);
    lastBlynkPercent = avgPercent;
  }

  if (abs(avgVolume - lastBlynkVolume) >= 50.0) {
    Blynk.virtualWrite(V1, avgVolume);
    lastBlynkVolume = avgVolume;
  }

  // Alert logic: only if we have 3 averages
  if (avgHistoryFull) {
    bool allLow = true;
    bool allHigh = true;

    for (int i = 0; i < AVG_HISTORY_SIZE; i++) {
      if (avgHistory[i] >= 30.0) allLow = false;
      if (avgHistory[i] <= 80.0) allHigh = false;
    }

    if (allLow && !lowWaterAlertSent) {
      Blynk.logEvent("low_water", "⚠️ Water below 30% for 15 minutes");
      lowWaterAlertSent = true;
    } else if (!allLow && lowWaterAlertSent && avgPercent > 35.0) {
      lowWaterAlertSent = false;
    }

    if (allHigh && !tankFullAlertSent) {
      Blynk.logEvent("tank_full", "✅ Tank is full for 15 minutes");
      tankFullAlertSent = true;
    } else if (!allHigh && tankFullAlertSent && avgPercent < 78.0) {
      tankFullAlertSent = false;
    }
  }

  Serial.println("✅ Blynk data sent.");
}
