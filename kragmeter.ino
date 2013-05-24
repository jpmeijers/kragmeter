/*
Kragmeter upload
based on: AnalogInOutSerial Arduino example by Tom Igoe

          WebClient Arduino example by David A. Mellis

          Modifications to the above mentioned examples, 
          including all new funtions by JP Meijers - 20-10-2012
          
History:
2012-11-06 - Added i2c temperature sensor code


Prepaid electricity meter flash counter and uploader
Copyright (C) 2012  JP Meijers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

volatile int ledon = 0;
volatile int ledoff = 1;
volatile int ledcount = 0;
long lastUploadTime = 0;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  0x90, 0xA2, 0xDA, 0x00, 0x29, 0x4F };
IPAddress server(192,168,2,101); // my server
IPAddress upstreamserver(146,232,220,17); // upstream server

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;


void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  
  // start the serial library:
  Serial.begin(9600);
  Serial.println("System startup");
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // give the Ethernet shield a second to initialize:
  
  
  delay(1000);
  Serial.println("connecting...");
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 624;            // 10ms
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
  
}

ISR(TIMER1_COMPA_vect)
{
  //voer elke 10ms uit
  int sensorValue = analogRead(A15);
  
      
  if((sensorValue < 1000) && (ledoff == 1))
  {
    //Serial.print("On - ");
    //Serial.println(sensorValue);
    ledon = 1;
    ledoff = 0;
    ledcount++;
  }
  if((sensorValue > 1010) && (ledon == 1))
  {
    //Serial.print("Off - ");
    //Serial.println(sensorValue);
    ledon = 0;
    ledoff = 1;
  }
  
}


float readTemperature()
{
  Wire.requestFrom(0b1001011, 2);    // request 6 bytes from slave device #2

  byte MostSigByte = Wire.read();              // Read the first byte this is the MSB
  byte LeastSigByte = Wire.read();             // Now Read the second byte this is the LSB

  // Being a 12 bit integer use 2's compliment for negative temperature values
  int TempSum = (((MostSigByte << 8) | LeastSigByte) >> 3); 
  // From Datasheet the TMP75 has a quantisation value of 0.0625 degreesC per bit
  float temp = (TempSum*0.0625);
  
  return temp;
}


void loop()
{
  long currentTime = millis();
  int upload = 0;
  int seconds = 1;
  
  //int sensorValue = analogRead(A15);
  //Serial.println(sensorValue);
  
  if(currentTime - lastUploadTime > 60000)
  {
    upload=1;
    seconds = (currentTime - lastUploadTime)/1000;
  }
  if((currentTime < lastUploadTime) && (currentTime>60000)) //for overflows
  {
    upload=1;
    seconds = currentTime/1000;
  }
  if (upload==1)
  {
    int countToUpload = ledcount;
    ledcount = 0;
    upload = 0;
    // if you get a connection, report back via serial:
    
//upload to local server    
    if (client.connect(server, 80)) {
      Serial.println("connected");
      Serial.print("GET /uploadkrag.php?seconds=");
      Serial.print(seconds);
      Serial.print("&flashes=");
      Serial.print(countToUpload);
      Serial.print("&temperature=");
      Serial.print(readTemperature());
      Serial.println(" HTTP/1.0");
      
      // Make a HTTP request:
      client.print("GET /uploadkrag.php?seconds=");
      client.print(seconds);
      client.print("&flashes=");
      client.print(countToUpload);
      client.print("&temperature=");
      client.print(readTemperature());
      client.println(" HTTP/1.0");
      client.println();
          
      lastUploadTime = currentTime;
    } 
    else {
      // kf you didn't get a connection to the server:
      Serial.println("connection to local server failed");
      
      //sonder hierdie twee lyne sal hy oor en oor probeer tot hy suksesvol is en dan die gemiddelde van die hele tydperk upload
      //eerder net gemiddeld van die afgelope 60s
      //lastUploadTime = currentTime;
      //ledcount=0;
    }
      // if there are incoming bytes available 
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  
    client.stop();
  
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnected.");
      client.stop();
    }
    
  delay(100);
    
//upload to remote server    
    if (client.connect(upstreamserver, 80)) {
      Serial.println("connected");
      Serial.print("GET http://www.jpmeijers.com/powermeter/insertdata.php?seconds=");
      Serial.print(seconds);
      Serial.print("&flashes=");
      Serial.print(countToUpload);
      Serial.print("&temperature=");
      Serial.print(readTemperature());
      Serial.println(" HTTP/1.0");
      
      // Make a HTTP request:
      client.print("GET http://www.jpmeijers.com/powermeter/insertdata.php?seconds=");
      client.print(seconds);
      client.print("&flashes=");
      client.print(countToUpload);
      client.print("&temperature=");
      client.print(readTemperature());
      client.println(" HTTP/1.0");
      client.println();
          
      lastUploadTime = currentTime;
    } 
    else {
      // kf you didn't get a connection to the server:
      Serial.println("connection to remote server failed");
      
      //sonder hierdie twee lyne sal hy oor en oor probeer tot hy suksesvol is en dan die gemiddelde van die hele tydperk upload
      //eerder net gemiddeld van die afgelope 60s
      //lastUploadTime = currentTime;
      //ledcount=0;
    }
      // if there are incoming bytes available 
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  
    client.stop();
  
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnected.");
      client.stop();
    }
  }  
  
  delay(100);
}


