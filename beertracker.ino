#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <list>
#include <algorithm>

#include "wifi_credentials.h"

#define SOUND_SENSOR_PIN 2
#define SHORT_BUF_SZ 100
#define LONG_BUF_SZ (10*SHORT_BUF_SZ)
std::list<int> short_buf;
std::list<int> long_buf;

#define DHT_PIN 4
DHT dht(DHT_PIN, DHT11);

int print_counter = 1;

double calc_mean(const std::list<int>& l) {
  int sum = std::accumulate(l.begin(), l.end(), 0);
  int N = l.size();
  return static_cast<double>(sum)/N;
}

bool bubble_detection(std::list<int>& sbuff, const std::list<int>& lbuff) {
  int bubble_threshold = 2;
  std::list<int>::iterator max_val = std::max_element(sbuff.begin(), sbuff.end());
  double long_mean = calc_mean(lbuff);
  bool bubble_test = (*max_val - long_mean) > bubble_threshold;
  return bubble_test;
}

void setup() {  
  Serial.begin(115200);

  pinMode(SOUND_SENSOR_PIN, INPUT);
  for(int i = 0; i < SHORT_BUF_SZ; i++){
    short_buf.push_back(0);
  }
  for(int i = 0; i < LONG_BUF_SZ; i++) {
    long_buf.push_back(0);
  }

  dht.begin();

  Serial.printf("Connecting to ssid: %s\n", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  
  delay(200);
  Serial.println("Setup done.");
}

void loop() {
  int sound_sensor_val = analogRead(A0);
  short_buf.pop_front();
  short_buf.push_back(sound_sensor_val);
  long_buf.pop_front();
  long_buf.push_back(sound_sensor_val);
  
  if(print_counter < SHORT_BUF_SZ) {
    print_counter++;
  } else {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if(isnan(humidity) || isnan(temperature)) {
      Serial.println("error reading temp/humid");
    } else {
      Serial.printf("temp: %.1f, hum: %.1f\n", temperature, humidity);
    }
    print_counter = 1;
    bool bubble = bubble_detection(short_buf, long_buf);
    if(bubble) {
      Serial.println("BUBBLE!");
    }
  }
  delay(5);
}
