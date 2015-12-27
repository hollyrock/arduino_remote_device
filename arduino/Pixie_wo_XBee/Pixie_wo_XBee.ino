/**************************************
  Remote Sense Device without XBee
  
 **************************************/
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

// ********************************************************
//    Configuration Parameters
// ********************************************************
unsigned long start = millis();

int statusLed = 11; //Pin# for debug LED
int errorLed = 12; //Pin# for debug LED
int serial_speed = 19200;
uint8_t data = 0;

// ********************************************************
//  Function    : flashLed
//   Description : Blink LED
// ********************************************************
void flashLed(int pin, int times, int wait) {
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      if (i + 1 < times) delay(wait);
    }
}


// ********************************************************
//  Function    : getEnvDataDHT
//  Desc.       : Get temperature and Humidity from DHT Device
// ********************************************************
int getEnvDataDHT(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    return -1;
  }
  
  float hi = dht.computeHeatIndex(t, h, false);

  //payload[0] = t >> 8 & 0xff;
  //payload[1] = t & 0xff;
  //xbee.send(tx);
  
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("*C");
  Serial.print("Debug: ");
  Serial.print("Heat Index: ");
  Serial.print(hi);
  Serial.println("*C");
  return 1;
}


// ********************************************************
//  Function    : setup
// ********************************************************
void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  
  Serial.begin(serial_speed);    // Open serial port for debug console
  dht.begin();
}

// ********************************************************
//  Function    : loop
// ********************************************************
void loop() {
  getEnvDataDHT();
  delay(1000);
}
