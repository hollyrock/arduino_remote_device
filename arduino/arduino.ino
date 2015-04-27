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
    This example is for Series 1 XBee
    Sends a TX16 or TX64 request with the value of analogRead(pin5) and checks the status response for success
    
    Note: In my testing it took about 15 seconds for the XBee to start reporting success, so I've added a startup delay
    XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
    XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
*/

SoftwareSerial altSerial(2, 3);
XBee xbee = XBee();

Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); 

// allocate two bytes for to hold a 10-bit analog reading
uint8_t payload[] = {0, 0};
unsigned long start = millis();

// with Series 1 you can use either 16-bit or 64-bit addressing

// 16-bit addressing: Enter address of remote XBee, typically the coordinator
Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload));

// create reusable response objects for responses we expect to handle 
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();

// 64-bit addressing: This is the SH + SL address of remote XBee
//XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x4008b490);
// unless you have MY on the receiving radio set to FFFF, this will be received as a RX16 packet
//Tx64Request tx = Tx64Request(addr64, payload, sizeof(payload));

TxStatusResponse txStatus = TxStatusResponse();

int pin5 = 0;
int statusLed = 11;
int errorLed = 12;
int serial_speed = 19200;

//uint8_t option = 0;
uint8_t data = 0;

void flashLed(int pin, int times, int wait) {
    
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      if (i + 1 < times) delay(wait);
    }
}

void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  Serial.begin(serial_speed); // for Debug
  altSerial.begin(serial_speed);
  xbee.setSerial(altSerial);
}

void loop() {
  xbee.readPacket();
  if (xbee.getResponse().isAvailable()) {
      Serial.println("I got something..");
    
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) {

        //Get the packet  
        xbee.getResponse().getRx16IoSampleResponse(ioSample);
        
        Serial.println("Frame from valid ID ==========");
        Serial.print("ID: ");
        Serial.println(ioSample.getRemoteAddress16(), HEX);
        Serial.print("Sample Size:");
        Serial.println(ioSample.getRemoteAddress16(), DEC);
        Serial.print("Data Length:");
        Serial.println(ioSample.getDataLength(), DEC);
        Serial.print("Frame Data:");
        for (int i=0; i<ioSample.getDataLength(); i++){
          Serial.write(' ');
          if(iscntrl(ioSample.getData()[i]))
            Serial.write('.');
          else
            Serial.write(ioSample.getData()[i]);
          Serial.write(' ');
        }
        Serial.println();
        Serial.println("==============================");
          
        flashLed(statusLed, 3, 10);
        
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
              xbee.getResponse().getRx16Response(rx16);
              //option = rx16.getOption();
              data = rx16.getData(0);
              Serial.print("Data captured by rx16.getData: ");
              Serial.println(data,HEX);
              
          }else{
              xbee.getResponse().getRx64Response(rx64);
              //option = rx64.getOption();
              data = rx64.getData(0);
              Serial.print("Data captured by rx64.getData: ");
              Serial.println(data,HEX);
          }
      
          Serial.print("So, I will send some charactors to XBee having addr: ");
          Serial.println(ioSample.getRemoteAddress16(),HEX);
          
          
          if (millis() - start > 15000){
              pin5 = analogRead(5);
              payload[0] = pin5 >> 8 & 0xff;
              payload[1] = pin5 & 0xff;
              xbee.send(tx);
          }
      
          // after sending a tx request, we expect a status response
          // wait up to 5 seconds for the status response
          if (xbee.readPacket(5000)) {
              Serial.println("Wow! I've got a response!");
              // should be a znet tx status            	
    	      if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) {
    	          xbee.getResponse().getZBTxStatusResponse(txStatus);
                  
                  // get the delivery status, the fifth byte
                  if (txStatus.getStatus() == SUCCESS) {
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
