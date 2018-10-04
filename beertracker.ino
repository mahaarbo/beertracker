#include <ESP8266WiFi.h> // board specific
#include <WiFiClient.h> // Arduino library for wificlients
#include <list>
#include <algorithm>

#define SOUND_SENSOR_PIN 2
#define SHORT_BUF_SZ 100
#define LONG_BUF_SZ (10*SHORT_BUF_SZ)
std::list<int> short_buf;
std::list<int> long_buf;

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
    print_counter = 1;
    std::list<int>::iterator max_val = std::max_element(short_buf.begin(), short_buf.end());
    double long_mean = calc_mean(long_buf);
    double short_mean = calc_mean(short_buf);
    bool bubble = bubble_detection(short_buf, long_buf);
    Serial.printf("max val: %d | lmean: %.1f | maxdiff: %.1f | shortdiff: %.1f\n", *max_val, long_mean, static_cast<double>(*max_val)-long_mean, short_mean-long_mean);
    if(bubble) {
      Serial.println("BUBBLE!");
    }
  }
  delay(5);
}
