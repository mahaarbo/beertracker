#include <ESP8266WiFi.h>
#include <AdafruitIO_WiFi.h>
#include <DHT.h>
#include <list>
#include <algorithm>

#include "aio_config.h"

#define SOUND_SENSOR_PIN 2
#define SHORT_BUF_SZ 100
#define LONG_BUF_SZ (10*SHORT_BUF_SZ)
std::list<int> short_buf;
std::list<int> long_buf;
int bubble_counter = 0;

#define DHT_PIN 4
DHT dht(DHT_PIN, DHT11);

AdafruitIO_WiFi aio(aio_user, aio_key, wifi_ssid, wifi_password);
AdafruitIO_Feed *aio_temperature = aio.feed("beer.temperature");
AdafruitIO_Feed *aio_humidity = aio.feed("beer.humidity");
AdafruitIO_Feed *aio_bubbles = aio.feed("beer.bubbles");

#define AIO_LOOP_DELAY_S 60
#define AIO_LOOP_DELAY_MS (AIO_LOOP_DELAY_S*1000)
#define SOUND_SENSOR_LOOP_DELAY_MS 5
#define SOUND_SENSOR_LOOP_MULTIPLIER (AIO_LOOP_DELAY_MS/SOUND_SENSOR_LOOP_DELAY_MS)
int loop_counter = 0;

float calc_mean(const std::list<int>& l) {
  int sum = std::accumulate(l.begin(), l.end(), 0);
  int N = l.size();
  return static_cast<float>(sum)/N;
}

bool bubble_detection(std::list<int>& sbuff, const std::list<int>& lbuff) {
  float bubble_threshold = 3.0;
  std::list<int>::iterator max_val = std::max_element(sbuff.begin(), sbuff.end());
  float long_mean = calc_mean(lbuff);
  Serial.printf("bubble diff: %.1f\n", static_cast<float>(*max_val) - long_mean);
  bool bubble_test = (static_cast<float>(*max_val) - long_mean) > bubble_threshold;
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

  aio.connect();
  while(aio.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println(aio.statusText());
  
  delay(200);
  Serial.println("Setup done.");
}

void loop() {
  int sound_sensor_val = analogRead(A0);
  short_buf.pop_front();
  short_buf.push_back(sound_sensor_val);
  long_buf.pop_front();
  long_buf.push_back(sound_sensor_val);

  if(loop_counter % SHORT_BUF_SZ == 0) {
    bool bubble = bubble_detection(short_buf, long_buf);
    if(bubble) {
      Serial.println("BUBBLE!!!!");
      bubble_counter++;
    }
  }
  
  if(loop_counter < SOUND_SENSOR_LOOP_MULTIPLIER) {
    loop_counter++;
  } else {
    aio.run();
    Serial.println(aio.statusText());
    loop_counter = 0;
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if(isnan(humidity) || isnan(temperature)) {
      Serial.println("error reading temp/humid");
    } else {
      aio_temperature->save(temperature);
      aio_humidity->save(humidity);
      float bubble_rate = static_cast<float>(bubble_counter)/(60.0/AIO_LOOP_DELAY_S);
      aio_bubbles->save(bubble_rate);
      bubble_counter = 0;
      Serial.printf("temp: %.1f, hum: %.1f, bubble rate: %.1f\n", temperature, humidity, bubble_rate);
    }
  }
  delay(SOUND_SENSOR_LOOP_DELAY_MS);
}
