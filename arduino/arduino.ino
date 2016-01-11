/**************************************
  Remote Sense Device using XBee
 **************************************/

#include <SoftwareSerial.h>
#include <XBee.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define PAYLOAD_SIZE sizeof(float)

// ************ Talk with Pi 
typedef union {
  float float_variable;
  uint8_t bytes_array[4];
} float_sensor_result;

uint8_t payload[PAYLOAD_SIZE] = {0};
uint8_t *data = NULL;
size_t data_len = 0;
uint8_t option =0;

// ************* Communicate AT Command to XBee
  uint8_t ID[] = {'I','D'}; //Personal Area Network ID ...OK
  uint8_t MY[] = {'M','Y'}; //16-bit Address           ...OK
  uint8_t VR[] = {'V','R'}; //Firmware Version         ...OK
  uint8_t FR[] = {'F','R'}; //Software Reset           ...OK

// ************* Arduino Settings
unsigned long start = millis();
int statusLed = 11; //Pin# for debug LED
int errorLed = 12; //Pin# for debug LED
int serial_speed = 19200;

SoftwareSerial altSerial(2, 3);
DHT dht(DHTPIN,DHTTYPE);

//    XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
//    XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
XBee xbee = XBee();
Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload)); // Coordinator Address: 0x1234
Rx16Response rx16 = Rx16Response();

TxStatusResponse txStatus = TxStatusResponse();
Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); 
AtCommandRequest atRequest = AtCommandRequest();
AtCommandResponse atResponse = AtCommandResponse();

// ************* Flash LED function
void flashLed(int pin, int times, int wait) {
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      if (i + 1 < times) delay(wait);
    }
}

// ************* Get temperature and Humidity from DHT Device
float getEnvDataDHT(int mode){
  float ret;
  // mode 1 = Temperature, 2 = Humidity, 3 = HeatIndex
  if(mode == 3) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float ret = dht.computeHeatIndex(t, h, false);
  }else if(mode == 2){
    float ret = dht.readHumidity();
  }else{
    float ret = dht.readTemperature();
  }
  
  if (isnan(ret)) {
    return -1;
  }

  return ret;

  // debug
  Serial.print("mode: ");
  Serial.println(mode);
  Serial.print("Returne: ");
  Serial.print(ret);
}

//  ************* Get curent configuration from XBee
void getParam(){
  atRequest.setCommand(ID);
  sendATCommand();
  atRequest.setCommand(MY);
  sendATCommand();
  atRequest.setCommand(VR);
  sendATCommand();
}

// *************** Send AT command to XBee for configuration
int sendATCommand(){
  int err = 0;
  xbee.send(atRequest); //send the command
      
  if (xbee.readPacket(5000)) {
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
      if (atResponse.isOk()) {
        Serial.print("[DBG] Command [");
        Serial.write(atResponse.getCommand()[0]);
        Serial.write(atResponse.getCommand()[1]);
        Serial.println("] was successful.*****");
        if (atResponse.getValueLength() > 0) {
          Serial.print("[DBG] Return Value length: ");
          Serial.println(atResponse.getValueLength(), DEC);
          Serial.print("Value: ");
          for (int i=0; i<atResponse.getValueLength(); i++) {
            Serial.print(atResponse.getValue()[i], HEX);
          }
          Serial.println();
          Serial.println("*************************");
        }
      }
      else {
        Serial.print("[DBG] Command [");
        Serial.write(atResponse.getCommand()[0]);
        Serial.write(atResponse.getCommand()[1]);
        Serial.print("] was failed with error code: ");
        Serial.println(atResponse.getStatus(), HEX);
        Serial.println("*************************");
        err++;
      }
    }
  }
  return err;
}

// ****************** Print Frame content into Serial Monitor
void debugPrint(Rx16IoSampleResponse sample){
  Serial.println("DEBUG========================");
  Serial.print("ID         :");
  Serial.println(sample.getRemoteAddress16(), HEX);
  Serial.print("Sample Size:");
  Serial.println(sample.getRemoteAddress16(), DEC);
  Serial.print("Data Length:");
  Serial.println(sample.getDataLength(), DEC);
  Serial.print("Frame Data : ");
  for (int i=0; i<sample.getDataLength(); i++){
      Serial.print(sample.getData()[i], HEX);
  }
  Serial.println();
  Serial.println("==============================");
}

// *************** Command Descriptor for Raspberry PI 
void cmdDescriptor(uint8_t command){
  switch (*data) {
    case 'DBG':
      debugPrint(ioSample);
      break;
    case 'TMP':
      //getEnvDataDHT();
      break;
    case 'HELO':
      Serial.println ("Hello!");
      break;
    case 'BYE':
      Serial.println ("Bye now");
      break;
    case 'sLED':
      flashLed(statusLed, 3, 10);
      break;
    case 'RST':
      atRequest.setCommand(FR);
      sendATCommand();
      break;
    default:
      Serial.println ("[Cmd Descriptor] Invalid command.*****");
    }  
}

void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  
  Serial.begin(serial_speed);    // Open serial port for debug console
  altSerial.begin(serial_speed); // Open serial port for Xbee
  xbee.setSerial(altSerial);

  dht.begin();
  getParam();                     // Obtain self configration parameters
}

void loop() {
  xbee.readPacket();
  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      xbee.getResponse().getRx16IoSampleResponse(ioSample); //Fulfill the frame data
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
        xbee.getResponse().getRx16Response(rx16);
        option = rx16.getOption();
        data = rx16.getData();
        data_len = rx16.getDataLength();
      }

      if (NULL != data) {
        Serial.println("Received");
        Serial.write(data, data_len);
        Serial.println("");
        if(strcmp((char*)data, "temp") == 0){
          //cmdDescriptor(data);
          Serial.println("Send temperature");
          float temp = getEnvDataDHT(1); // Mode 1 = temperature
          Serial.print("Temp: ");
          Serial.print(temp, sizeof(float));
          float_sensor_result res;
          res.float_variable = temp;
          memcpy(&payload, &res.bytes_array[0], PAYLOAD_SIZE);

          xbee.send(tx);
          flashLed(statusLed, 5, 50);
        }
      }
      else{
        Serial.println("Failed to get data");
        flashLed(errorLed, 3, 500);
      }
    }
  }
}
