/*
  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  Remote Sense Device using XBee
  
  Created 30 May 2015
  by hollyRock
  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
 
#include <XBee.h>
#include <SoftwareSerial.h>

/*
    XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
    XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
*/

SoftwareSerial altSerial(2, 3);
XBee xbee = XBee();

uint8_t payload[] = {0, 0}; // allocate two bytes for to hold a 10-bit analog reading
// AT Command for retreving XBee configurations on Arduino
//uint8_t arryCmd[] = {'M', 'Y'};
uint8_t ID[] = {'I','D'}; //Personal Area Network ID ...OK
uint8_t MY[] = {'M','Y'}; //16-bit Address           ...OK
uint8_t DB[] = {'D','B'}; //Received Signal Strength ...NG
uint8_t PP[] = {'P','P'}; //Peak Power               ...NG
uint8_t BD[] = {'B','D'}; //Interface Data Rate      ...NG
uint8_t VP[] = {'V','+'}; //Voltage Supply Monitoring
uint8_t TP[] = {'T','P'}; //Module Temperature
uint8_t VR[] = {'V','R'}; //Firmware Version         ...OK
uint8_t FR[] = {'F','R'}; //Software Reset           ...OK

Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload)); // Coordinator Address: 0x1234
TxStatusResponse txStatus = TxStatusResponse();

Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); 
Rx16Response rx16 = Rx16Response();

AtCommandRequest atRequest = AtCommandRequest();
AtCommandResponse atResponse = AtCommandResponse();

/*
    Configuration Parameters
*/
unsigned long start = millis();
int anaSense = 5;
int pin5 = 0;
int statusLed = 11;
int errorLed = 12;
int serial_speed = 19200;
//uint8_t option = 0;
uint8_t data = 0;

void setup() {
  
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  
  Serial.begin(serial_speed);     // Debug
  altSerial.begin(serial_speed);  // Xbee
  xbee.setSerial(altSerial);
  
  debugPrint(ioSample);       // Print frame content to serial monitor
  getParam();                 // Obtain self configration parameters

}

void flashLed(int pin, int times, int wait) {
    
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      if (i + 1 < times) delay(wait);
    }
}

void getParam(){
  atRequest.setCommand(ID);
  sendATCommand();
  atRequest.setCommand(MY);
  sendATCommand();
//  atRequest.setCommand(DB);
//  sendATCommand();
//  atRequest.setCommand(PP);
//  sendATCommand();
//  atRequest.setCommand(BD);
//  sendATCommand();
//  atRequest.setCommand(VP);
//  sendATCommand();
//  atRequest.setCommand(TP);
//  sendATCommand();
  atRequest.setCommand(VR);
  sendATCommand();
//  atRequest.setCommand(FR);
//  sendATCommand();
}

int sendATCommand(){

  int err = 0;
  xbee.send(atRequest); //send the command
      
  if (xbee.readPacket(5000)) {
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
      if (atResponse.isOk()) {
        Serial.print("Command [");
        Serial.write(atResponse.getCommand()[0]);
        Serial.write(atResponse.getCommand()[1]);
        Serial.println("] was successful.");
        if (atResponse.getValueLength() > 0) {
          Serial.print("Return Value length: ");
          Serial.println(atResponse.getValueLength(), DEC);
          Serial.print("Value: ");
          for (int i=0; i<atResponse.getValueLength(); i++) {
            Serial.print(atResponse.getValue()[i], HEX);
          }
          Serial.println();
        }
      }
      else {
        Serial.print("Command [");
        Serial.write(atResponse.getCommand()[0]);
        Serial.write(atResponse.getCommand()[1]);
        Serial.print("] was failed with error code: ");
        Serial.println(atResponse.getStatus(), HEX);
        err++;
      }
    }
  }
  return err;
}

void debugPrint(Rx16IoSampleResponse sample){
  
  Serial.println("DEBUG==========");
  Serial.print("ID         :");
  Serial.println(sample.getRemoteAddress16(), HEX);
  Serial.print("Sample Size:");
  Serial.println(sample.getRemoteAddress16(), DEC);
  Serial.print("Data Length:");
  Serial.println(sample.getDataLength(), DEC);
  Serial.print("Frame Data : ");
  for (int i=0; i<sample.getDataLength(); i++){
      Serial.println(sample.getData()[i], HEX);
  }
  Serial.println();
  Serial.println("==============================");
}

void loop() {

  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      xbee.getResponse().getRx16IoSampleResponse(ioSample); //Fulfill the frame data
      
      //Do somthing with received frame
      flashLed(statusLed, 3, 10); // Flash LED
      
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
        xbee.getResponse().getRx16Response(rx16);
        //option = rx16.getOption();
        data = rx16.getData(0);
        Serial.print("Data captured by rx16.getData: ");
        Serial.println(data, HEX);
      }
      
      Serial.print("So, I will send some charactors to XBee having addr: ");
      Serial.println(ioSample.getRemoteAddress16(), HEX);
      
      if (millis() - start > 15000){
        pin5 = analogRead(anaSense);
        payload[0] = pin5 >> 8 & 0xff;
        payload[1] = pin5 & 0xff;
        xbee.send(tx);
      }

      if (xbee.readPacket(5000)) { // after sending a tx request, expecting a status response
        
        Serial.println("Wow! I've got a response!");

        if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) { // should be a znet tx status
          
          xbee.getResponse().getZBTxStatusResponse(txStatus);
          
          if (txStatus.getStatus() == SUCCESS) { // get the delivery status, the fifth byte
          
            flashLed(statusLed, 5, 50);
            Serial.println("Success! Congraturations!");

          } else {

            Serial.println("a,oh we're in trouble..");
            flashLed(errorLed, 3, 500);
          }
        }     
      } else if (xbee.getResponse().isError()) {
        Serial.println("Wow! I've got an Error response!");

      } else {
         // local XBee did not provide a timely TX Status Response.  Radio is not configured properly or connected
         flashLed(errorLed, 2, 50);
      }
    }
    delay(1000);
  }
}
