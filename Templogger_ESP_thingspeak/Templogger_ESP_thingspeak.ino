#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
DHTesp dht;

// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS D2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress Thermometer;

#define SECRET_SSID "LEMard"		// replace MySSID with your WiFi network name
#define SECRET_PASS "lem260182"	// replace MyPassword with your WiFi password

#define SECRET_CH_ID 1780615			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "DJQ7D93EZNA5DSDJ"   // replace XYZ with your channel write API Key

#include <ESP8266WiFi.h>
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

String myStatus = "";



ADC_MODE(ADC_VCC);
void setup(){
  pinMode(D1, OUTPUT);
  pinMode(D3, OUTPUT);
  Serial.begin(9600);

 // WiFi.mode(WIFI_STA); 
  dht.setup(D5, DHTesp::DHT11); // Connect DHT sensor to GPIO 17 
}

void loop()
{

    digitalWrite(D1, HIGH);  
    float humidity;
    float temperature;

    Serial.println();
    Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");

  for(int i=0;i<10;i++){
     
    delay(dht.getMinimumSamplingPeriod());    
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();    
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(humidity, 1);
    Serial.print("\t\t");
    Serial.print(temperature, 1);
    Serial.print("\t\t");
    Serial.print(dht.toFahrenheit(temperature), 1);
    Serial.print("\t\t");
    Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
    Serial.print("\t\t");
    Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
    digitalWrite(D1, LOW);  
    digitalWrite(D1, HIGH);      
    if (dht.getStatusString()=="OK"){
      break;
    }
  };

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  ThingSpeak.setField(7, humidity);
  ThingSpeak.setField(6, temperature);

  digitalWrite(D1, HIGH);
  sensors.begin();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF"); 

  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");


  for(int i=0;i<sensors.getDeviceCount();i++){
    sensors.getAddress(Thermometer, i);  
    printData(Thermometer);
    float tempC = sensors.getTempC(Thermometer);
    ThingSpeak.setField(i+1, tempC);
  }

  digitalWrite(D1, LOW); 

  float voltaje=0.00f;
  voltaje = ESP.getVcc();
  Serial.print(voltaje/1024.00f);
  Serial.println(" V");
  ThingSpeak.setField(8, voltaje/1024.00f);
  
  // write to the ThingSpeak channel
  Wificonnectmulti();
  senddata();
  sleep();
    /*
  for(int i=0;i<3;i++){
    // Connect or reconnect to WiFi

    WiFi.mode(WIFI_STA);
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        delay(5000);     
      } 

      Serial.println("\nConnected.");
    }

    ThingSpeak.setStatus(myStatus);    
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(x == 200){
      Serial.println("Channel update successful.");
//      WiFi.disconnect(); 
      WiFi.mode(WIFI_OFF);
      i=3;
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  ///  delay(15000);

  }

  WiFi.mode(WIFI_OFF);
  digitalWrite(D1, LOW);
  delay(500);
  ESP.deepSleep(600e6);

 */  


}

void sleep(){
  Serial.println("Sleep...");
  WiFi.mode(WIFI_OFF);
  digitalWrite(D1, LOW); 
  ESP.deepSleep(600e6);  
}

void senddata(){
  if (WiFi.status() == WL_CONNECTED){
    for(int i=0;i<3;i++){
      ThingSpeak.setStatus(myStatus);    
  //    Serial.print("ThingSpeak status ");
  //    Serial.println(myStatus);
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if(x == 200){
        Serial.println("Channel update successful.");
        digitalWrite(D1, HIGH);
        delay(200);
        digitalWrite(D1, LOW);
        i=3;
        sleep();      
      }
      else{
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    }    
  }

}
 
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void Wificonnectmulti() {
  Wificonnectclose("LEMard","lem260182");
  Wificonnectclose("LEM","lem260182");
  Wificonnectopen();
}


bool Wificonnectclose(String Ssid,String pass){
  WiFi.disconnect();
  WiFi.begin(Ssid,pass);
  Serial.print("Connect to ");
  Serial.print(Ssid);  
  for(int i=0;i<10000;i=i+500){
    if (WiFi.status() == WL_CONNECTED){
//      Serial.print("WiFi.status()=");
//      Serial.println(WiFi.status());
      Serial.print("Connected, IP address: ");
              //  "Подключились, IP-адрес: "
      Serial.println(WiFi.localIP());
      senddata();
      return true;
      break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

}

bool Wificonnectopen(){
  
  String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t* bssid;
  int32_t channel;
  bool hidden;
  int scanResult;

  Serial.println(F("Starting WiFi scan..."));

  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (scanResult == 0) {
    Serial.println(F("No networks found"));
  } else if (scanResult > 0) {
    Serial.printf(PSTR("%d networks found:\n"), scanResult);

    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %s\n"),
                    i,
                    channel,
                    bssid[0], bssid[1], bssid[2],
                    bssid[3], bssid[4], bssid[5],
                    rssi,
                    (encryptionType == ENC_TYPE_NONE) ? ' ' : '*',
                    hidden ? 'H' : 'V',
                    ssid.c_str());
      delay(0);
      if (encryptionType == ENC_TYPE_NONE){
        Serial.print("Connect to ");
        Serial.print(ssid.c_str());
        WiFi.begin(ssid.c_str(),"");

        for(int i=0;i<10000;i=i+500){
          if (WiFi.status() == WL_CONNECTED){
          Serial.print("Connected, IP address: ");
                  //  "Подключились, IP-адрес: "
          Serial.println(WiFi.localIP());
          senddata();
          return true;
          break;
          }
          Serial.print(".");
          delay(500);
        }
        Serial.println("");


      }
    }
  } else {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
  }  
}
