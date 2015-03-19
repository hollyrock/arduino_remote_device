/**
 * Copyright (c) 2009 Andrew Rapp. All rights reserved.
 *
 * This file is part of XBee-Arduino.
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

//Define SoftwareSerial TX/RX pins
//Connect Arduino pin 8 to TX of usb-serial device
uint8_t ssRX = 8;
//Connect Arduino pin 9 to RX of usb-serial device
uint8_t ssTX = 9;

SoftwareSerial nss(ssRX, ssTX);
XBee xbee = XBee();

Rx16IoSampleResponse ioSample = Rx16IoSampleResponse();
// 64-bit response is same except API ID equals RX_64_IO_RESPONSE and returns a 64-bit address

void setup() {

    Serial.begin(57600); // for Debug
    xbee.setSerial(Serial);
    nss.begin(57600); // Start soft serial

}

void loop() {
    
    xbee.readPacket(); //Attempt to read a packet
    
    if (xbee.getResponse().isAvailable()) {
        Serial.println("I got something!");
        
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
            xbee.getResponse().getRx16IoSampleResponse(ioSample);
            //flashLed(statusLed, 3, 10);
            nss.print("Received I/O Sample from: ");
            nss.println(ioSample.getRemoteAddress16(), HEX);
            
            nss.print("Sample size is ");
            nss.println(ioSample.getSampleSize(), DEC);
            
            if (ioSample.containsAnalog()) {
                nss.println("Sample contains analog data");
            }
            
            if (ioSample.containsDigital()) {
                nss.println("Sample contains digital data");
            }
            
            for (int k = 0; k < ioSample.getSampleSize(); k++) {
                nss.print("Sample ");
                nss.print(k+1, DEC);
                nss.println(":");
                
                for (int i = 0; i <= 5; i++) {
                    if (ioSample.isAnalogEnabled(i)) {
                        nss.print("Analog (AI");
                        nss.print(i, DEC);
                        nss.print(") is ");
                        nss.println(ioSample.getAnalog(i, k));
                    }
                }
                
                for (int i = 0; i <= 8; i++) {
                    if (ioSample.isDigitalEnabled(i)) {
                        nss.print("Digital (DI");
                        nss.print(i, DEC);
                        nss.print(") is ");
                        nss.println(ioSample.isDigitalOn(i, k));
                    }
                }
            }
        }else{
            nss.print("Expected I/O Sample, but got ");
            nss.print(xbee.getResponse().getApiId(), HEX);
        }
    }else if (xbee.getResponse().isError()) {
        nss.print("Error reading packet. Error code: ");
        nss.println(xbee.getResponse().getErrorCode());
    }
}


// Flash LEDs
void flashLed(int pin, int times, int wait) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(wait);
        digitalWrite(pin, LOW);
        if (i + 1 < times) delay(wait);
    }
}
