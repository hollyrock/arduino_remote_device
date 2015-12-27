/**************************************
  Remote Sense Device using XBee
  by hollyRock  30 May 2015
 **************************************/

#include <SoftwareSerial.h>
#include <XBee.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

// ********************************************************
//    XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
//    XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
// ********************************************************
SoftwareSerial altSerial(2, 3);
XBee xbee = XBee();
uint8_t payload[] = {0, 0}; // allocate two bytes for to hold a 10-bit analog reading

Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload)); // Coordinator Address: 0x1234
Rx16Response rx16 = Rx16Response();

TxStatusResponse txStatus = TxStatusResponse();
Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); 
AtCommandRequest atRequest = AtCommandRequest();
AtCommandResponse atResponse = AtCommandResponse();

  // AT Command for XBee on Arduino
  uint8_t ID[] = {'I','D'}; //Personal Area Network ID ...OK
  uint8_t MY[] = {'M','Y'}; //16-bit Address           ...OK
  uint8_t VR[] = {'V','R'}; //Firmware Version         ...OK
  uint8_t FR[] = {'F','R'}; //Software Reset           ...OK
  //uint8_t DB[] = {'D','B'}; //Received Signal Strength ...NG
  //uint8_t PP[] = {'P','P'}; //Peak Power               ...NG
  //uint8_t BD[] = {'B','D'}; //Interface Data Rate      ...NG
  //uint8_t VP[] = {'V','+'}; //Voltage Supply Monitoring...NG
  //uint8_t TP[] = {'T','P'}; //Module Temperature       ...NG

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
//  Function    : getParam
//  Desc.       : Get curent configuration from XBee
// ********************************************************
void getParam(){
 
  atRequest.setCommand(ID);
  sendATCommand();
  atRequest.setCommand(MY);
  sendATCommand();
  atRequest.setCommand(VR);
  sendATCommand();
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
  Serial.print(payload[0]);
  Serial.print(payload[1]);
  Serial.print("Heat Index: ");
  Serial.print(hi);
  Serial.println("*C");
  return 1;
}

// ********************************************************
//  Function    : sendATCommand
//  Desc.       : Send AT command to XBee for configuration
// ********************************************************
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

// ********************************************************
//  Function    : debugPrint
//  Desc.       : Print Frame content into Serial Monitor
// ********************************************************
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

// ********************************************************
//  Function    : cmdDescriptor
//  Desc.       : Command Descriptor for Raspberry PI 
// ********************************************************
void cmdDescriptor(uint8_t command){
/*
 * Arduino is controlled by applying command:
 * 0x00 (0)  : No Action
 * 0x02 (2)  : Return device parameter
 * 0x03 (3)  : set device parameter
 * 0x21 (33) : Return Temp data
 * 0x81 (129): for debug (hello!)
 * 0x82 (130): for debug (bye now)
 * 0x83 (131): for debug (bye now) 
 * 0xC1 (193): motion control command
 * 0xD1 (209): upload device program command
 * 0xFF (255): Reset
*/
  switch (data) {
    case 0:
      break;
    case 2:
      debugPrint(ioSample);
      break;
    case 3:
      break;
    case 33:
      getEnvDataDHT();
      break;
    case 129:
      Serial.println ("Hello!");
      break;
    case 130:
      Serial.println ("Bye now");
      break;
    case 131:
      flashLed(statusLed, 3, 10);
      break;
    case 193:
      break;
    case 209:
      break;
    case 255:
      atRequest.setCommand(FR);
      sendATCommand();
      break;
    default:
      Serial.println ("[DBG] Invalid command.*****");
    }  
}

// ********************************************************
//  Function    : setup
// ********************************************************
void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  
  Serial.begin(serial_speed);    // Open serial port for debug console
  altSerial.begin(serial_speed); // Open serial port for Xbee
  xbee.setSerial(altSerial);

  dht.begin();
  getParam();                     // Obtain self configration parameters
}

// ********************************************************
//  Function    : loop
// ********************************************************
void loop() {
  
  xbee.readPacket();
  
  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      xbee.getResponse().getRx16IoSampleResponse(ioSample); //Fulfill the frame data
      
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
        xbee.getResponse().getRx16Response(rx16);
        //option = rx16.getOption();
        data = rx16.getData(0);
        cmdDescriptor(data);
      }
      
      // ***** End procedure
      if (xbee.readPacket(5000)) { // after sending a tx request, expecting a status response (timeout:5000ms)
        Serial.println("[DBG] There is a response.*****");

        if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) { // should be a znet tx status
          xbee.getResponse().getZBTxStatusResponse(txStatus);
          
          if (txStatus.getStatus() == SUCCESS) { // get the delivery status, the fifth byte
            flashLed(statusLed, 5, 50);
            Serial.println("[DBG] Success.*****");
          } else {
            Serial.println("[DBG] Something wrong during end procedure.*****");
            flashLed(errorLed, 3, 500);
          }
        }     
      } else if (xbee.getResponse().isError()) {
        Serial.println("[DBG] I've recieved error response.*****");
      } else {
         // local XBee did not provide a timely TX Status Response.  Radio is not configured properly or connected
         flashLed(errorLed, 2, 50);
      }
    }
    delay(1000);
  }
  getEnvDataDHT();
  delay(1000);
}
