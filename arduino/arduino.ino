/**
 * Copyright (c) 2009 Andrew Rapp. All rights reserved.
 *
 * This file is modified from part of XBee-Arduino.
 *
 * XBee-Arduino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XBee-Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XBee-Arduino.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <XBee.h>
#include <SoftwareSerial.h>

/*
    XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
    XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
*/

SoftwareSerial altSerial(2, 3);
XBee xbee = XBee();

Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); 

// allocate two bytes for to hold a 10-bit analog reading
uint8_t payload[] = {0, 0};
unsigned long start = millis();

// Coordinator Address: 0x1234
Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload));
Rx16Response rx16 = Rx16Response();
TxStatusResponse txStatus = TxStatusResponse();

/*
    Configuration Parameters
*/
int pin5 = 0;
int statusLed = 11;
int errorLed = 12;
int serial_speed = 19200;

// AT Command for retreving XBee configurations on Arduino
//cahr* arryCmd[] = {"ID", "MY", "DB", "PP", "BD", "V+", "TP", "VR", "FR"};
uint8_t arryCmd[] = {'M', 'Y'};

//uint8_t option = 0;
uint8_t data = 0;

void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  Serial.begin(serial_speed);     // Debug
  altSerial.begin(serial_speed);  // Xbee
  xbee.setSerial(altSerial);
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
  if (sendATCommand()==0)
    Serial.println("Get all parameters.");
  else
    Serial.println("Fail to get all parameters. Debug me.");
}

int sendATCommand(){

  int err = 0;
  AtCommandRequest atRequest = AtCommandRequest(arryCmd);
  AtCommandResponse atResponse = AtCommandResponse();
  xbee.send(atRequest); //send the command
      
  if (xbee.readPacket(5000)) {
    
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(atResponse);
      
      if (atResponse.isOk()) {
        Serial.print("Command [");
        Serial.write(atResponse.getCommand()[0]);
        Serial.write(atResponse.getCommand()[1]);
        Serial.println("] was successful!");
        
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
        Serial.print("Error code: ");
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
  Serial.print("Frame Data :");
  
  for (int i=0; i<sample.getDataLength(); i++){
      Serial.write(sample.getData()[i]);
  }
  Serial.println();
  Serial.println("==============================");
}

void loop() {

  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    
    Serial.println("I got something.."); //Debug
    
    if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      
      xbee.getResponse().getRx16IoSampleResponse(ioSample); //Get the frame data
      debugPrint(ioSample); //for debug
      getParam();
      flashLed(statusLed, 3, 10);
        
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
        
        xbee.getResponse().getRx16Response(rx16);
        //option = rx16.getOption();
        data = rx16.getData(0);
        Serial.print("Data captured by rx16.getData: ");
        Serial.println(data,HEX);
      }
      
      Serial.print("So, I will send some charactors to XBee having addr: ");
      Serial.println(ioSample.getRemoteAddress16(),DEC);
          
      if (millis() - start > 15000){
        
        pin5 = analogRead(5);
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
