#include "DHT.h"
#include <Wire.h> 
#include <ESP8266WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <Scheduler.h>

IPAddress server_addr(10,19,160,50);  // IP of the MySQL *server* here
char user[] = "namiki";              // MySQL user login username
char password[] = "namiki";        // MySQL user login password

// Sample query
char INSERT_SQL[] = "INSERT INTO `maipping_temp`.`device_data7` (`Datetime`, `Temperature`, `Humidity`) VALUES (NOW(), %s, %s)";
char query[256];

WiFiClient client; 
MySQL_Connection conn(&client);
MySQL_Cursor* cursor;

// Replace with your network details
const char* ssid = "NPT-CEO_2.4Ghz";
const char* pass = "2021020100";

// Web Server on port 80
WiFiServer server(80);

// DHT Sensor
#define DHTPIN 2     // what digital pin we're connected to

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

class WebserverTask : public Task {
protected:
    // runs over and over again
    void loop()  {
        // Listenning for new clients
        WiFiClient client = server.available();
      
        if (client) {
          Serial.println("New client");
          // bolean to locate when the http request ends
          boolean blank_line = true;
          while (client.connected()) {
            
            if (client.available()) {
              char c = client.read();
      
              if (c == '\n' && blank_line) {
                // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
                float h = dht.readHumidity();
                // Read temperature as Celsius (the default)
                float t = dht.readTemperature();
                // Read temperature as Fahrenheit (isFahrenheit = true)
                float f = dht.readTemperature(true);
                // Check if any reads failed and exit early (to try again).
                if (isnan(h) || isnan(t) || isnan(f)) {
                  Serial.println("Failed to read from DHT sensor!");
                  strcpy(celsiusTemp, "Failed");
                  strcpy(fahrenheitTemp, "Failed");
                  strcpy(humidityTemp, "Failed");
                }
                else {
                  // Computes temperature values in Celsius + Fahrenheit and Humidity
                  float hic = dht.computeHeatIndex(t, h, false);
                  dtostrf(hic, 6, 2, celsiusTemp);
                  float hif = dht.computeHeatIndex(f, h);
                  dtostrf(hif, 6, 2, fahrenheitTemp);
                  dtostrf(h, 6, 2, humidityTemp);
                  // You can delete the following Serial.print's, it's just for debugging purposes
                  Serial.print("Humidity: ");
                  Serial.print(h);
                  Serial.print(" %\t Temperature: ");
                  Serial.print(t);
                  Serial.print(" *C ");
                  Serial.print(f);
                  Serial.print(" *F\t Heat index: ");
                  Serial.print(hic);
                  Serial.print(" *C ");
                  Serial.print(hif);
                  Serial.print(" *F");
                  Serial.print("Humidity: ");
                  Serial.print(h);
                  Serial.print(" %\t Temperature: ");
                  Serial.print(t);
                  Serial.print(" *C ");
                  Serial.print(f);
                  Serial.print(" *F\t Heat index: ");
                  Serial.print(hic);
                  Serial.print(" *C ");
                  Serial.print(hif);
                  Serial.println(" *F");
                }
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                // your actual web page that displays temperature and humidity
                client.println("<!DOCTYPE HTML>");
                client.println("<html>");
                client.println("<head></head><body><h1>ESP8266 - Temperature and Humidity Myarduino.net</h1><h3>Temperature in Celsius: ");
                client.println(celsiusTemp);
                client.println("*C</h3><h3>Temperature in Fahrenheit: ");
                client.println(fahrenheitTemp);
                client.println("*F</h3><h3>Humidity: ");
                client.println(humidityTemp);
                client.println("%</h3><h3>");
                client.println("</body></html>");
                break;
              }
              if (c == '\n') {
                // when starts reading a new line
                blank_line = true;
              }
              else if (c != '\r') {
                // when finds a character on the current line
                blank_line = false;
              }
            }
          }
          
          // closing the client connection
          delay(1);
          client.stop();
          Serial.println("Client disconnected.");
        }
    }
} webserver_task;

class RecorddatabaseTask : public Task {
protected:
    void loop()  {
        if (conn.connected()) {
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          float dh = dht.readHumidity();
          // Read temperature as Celsius (the default)
          float dt = dht.readTemperature();
          // Read temperature as Fahrenheit (isFahrenheit = true)
          float df = dht.readTemperature(true);
          // Check if any reads failed and exit early (to try again).
          if (isnan(dh) || isnan(dt) || isnan(df)) {
            Serial.println("Failed to read from DHT sensor!");
            strcpy(celsiusTemp, "Failed");
            strcpy(fahrenheitTemp, "Failed");
            strcpy(humidityTemp, "Failed");
        } else {
          // Computes temperature values in Celsius + Fahrenheit and Humidity
          float hic = dht.computeHeatIndex(dt, dh, false);
          dtostrf(dt, 6, 2, celsiusTemp);
          float hif = dht.computeHeatIndex(df, dh);
          dtostrf(hif, 6, 2, fahrenheitTemp);
          dtostrf(dh, 6, 2, humidityTemp);
        
          // delay(1000);
          // Initiate the query class instance
          MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
          // Save
          sprintf(query, INSERT_SQL, celsiusTemp, humidityTemp);
          // Execute the query
          cur_mem->execute(query);
          // Note: since there are no results, we do not need to read any data
          // Deleting the cursor also frees up memory used
          delete cur_mem;    
          Serial.println(query);
          Serial.println("Data recorded.");
        }
      } else {
        Serial.println("Not Connect Database...");
        delay(10000);
        ESP.reset();
      }
      delay(5000);
    }
} recorddatabase_task;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  while (!Serial); // wait for serial port to connect. Needed for Leonardo only
    
  // put your setup code here, to run once:
  dht.begin();

  // Begin WiFi section
  Serial.printf("\nConnecting to %s", ssid);
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("." + i);
    i++;
    if (i == 20) ESP.reset();
  }
  
  Serial.println("");
  Serial.println("WiFi connected");

  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
  delay(10000);

  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
  Serial.print("Connecting to SQL...  ");
  if (conn.connect(server_addr, 3306, user, password))
    Serial.println("OK.");
  else
    Serial.println("FAILED.");
  
  // create MySQL cursor object
  cursor = new MySQL_Cursor(&conn);
  
  Scheduler.start(&webserver_task);
  Scheduler.start(&recorddatabase_task);

  Scheduler.begin();  
}

void loop() {}
