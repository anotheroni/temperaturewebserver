/*
 Arduino temperature and humidity web server
 
 Using a Arduino Ethernet, a HIH-4030 analog humidity sensor and a
 Sparcfun TMP102 sensor connected with TWI/I2C.

 TMP102:
   https://www.sparkfun.com/products/9418
   sensor code from a bildr article:
   http://bildr.org/2012/11/hih4030-arduino/
 
 Circuit:
   Arduino Ethernet
   TWI: A4 (SDA) and A5 (SCL)
   Analog: A0
 */

#include <SPI.h>  // Needed by Ethernet.h
#include <Ethernet.h>
#include <Wire.h>

// assign a MAC address for the ethernet controller
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x5C, 0x48};

// assign an IP address for the controller:
IPAddress ip(192,168,0,20);
IPAddress gateway(192,168,0,1);	
IPAddress subnet(255, 255, 255, 0);

// Initialize the Ethernet server library with the IP address and port you
// want to use (port 80 is default for HTTP)
EthernetServer server(80);

// TMP102 TWI Address
int tmp102Address = 0x48;
// HIH-4030 Humidity sensor
int HIH4030_Pin = A0; //analog pin 0

float temperature = 0.0;
float relativeHumidity = 0.0;
long lastReadingTime = 0;

// Called at startup
void setup() {
  // start TWI
  Serial.begin(9600);
  Wire.begin();

  // start the Ethernet connection and the server
  Ethernet.begin(mac, ip);
  server.begin();

  // give the sensor and Ethernet shield time to setup
  delay(1000);
}

// Main program loop
void loop() { 
  // check for a reading no more than once every 5 seconds
  if (millis() - lastReadingTime > 5000) {
    temperature = getTemperature();
    relativeHumidity = getHumidity(temperature);
    lastReadingTime = millis();
  }

  // listen for incoming Ethernet connections
  listenForEthernetClients();
}

// Read temperature from TMP102
float getTemperature() {
  Wire.requestFrom(tmp102Address, 2); 

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  // Value is a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4; 

  float celsius = TemperatureSum * 0.0625;
  return celsius;
}

// Read humidity
float getHumidity(float degreesCelsius) {
  // Caculate relative humidity
  float supplyVolt = 5.0;

  // Read the value from the sensor
  int HIH4030_Value = analogRead(HIH4030_Pin);
  // Convert to voltage value
  float voltage = HIH4030_Value / 1023. * supplyVolt;

  // Convert the voltage to a relative humidity
  // - the equation is derived from the HIH-4030/31 datasheet
  // - it is not calibrated to your individual sensor
  // Table 2 of the sheet shows the may deviate from this line
  float sensorRH = 161.0 * voltage / supplyVolt - 25.8;
  // temperature adjustment 
  float trueRH = sensorRH / (1.0546 - 0.0026 * degreesCelsius);

  return trueRH;
}

void listenForEthernetClients() {
  // Listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          // print the current readings, in HTML format:
          client.print("Temperature: ");
          client.print(temperature);
          client.print(" degrees C");
          client.println("<br />");
          client.print("Humidity: ");
          client.print(relativeHumidity);
          client.print(" %");
          client.println("<br />");
          break;
        }
        if (c == '\n') {
          // starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}
