#include <Wire.h>
#include <Adafruit_AM2315.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
//#include <Ethernet2.h>//needed for server_logger
#include <Ethernet.h>
#include <EEPROM.h>
const char* ip_to_str(const uint8_t*);
//byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 };    //SERV_Logger
//byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };    //PIT_Logger
//byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x03 };    //Utility_Mon
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x04 };    //2nd_floor_temp_hum
byte serverip[] = {192, 168, 1, 13};
EthernetClient client;


//temp and humidty sensor variables
bool AM2315_detected=false;
Adafruit_AM2315 am2315;
unsigned long temp_hum_interval=10000;
unsigned long temp_hum_previousMillis;
unsigned long currentMillis = 0;
float average_temp[6];
float average_hum[6];
byte average_counter=0;

byte localip[4];        //the local system IP, either local or DHCP generated depending on user preference
byte subnetmask[4];
byte gateway[4];
byte dnsServerIp[4];      //system variable for the DNS server to use to resolve the NTP server URL. 
#define LOCALIPADDREEPROMADDRPART1 30
#define LOCALIPADDREEPROMADDRPART2 31
#define LOCALIPADDREEPROMADDRPART3 32
#define LOCALIPADDREEPROMADDRPART4 33
#define SUBNETMASKEEPROMADDRPART1 34
#define SUBNETMASKEEPROMADDRPART2 35
#define SUBNETMASKEEPROMADDRPART3 36
#define SUBNETMASKEEPROMADDRPART4 37
#define GATEWAYEEPROMADDRPART1 38
#define GATEWAYEEPROMADDRPART2 39
#define GATEWAYEEPROMADDRPART3 40
#define GATEWAYEEPROMADDRPART4 41
#define DNSSERVEREEPROMADDRPART1 42
#define DNSSERVEREEPROMADDRPART2 43
#define DNSSERVEREEPROMADDRPART3 44
#define DNSSERVEREEPROMADDRPART4 45

void setup() { 
  Serial.begin(115200);
if (! am2315.begin()) {
     Serial.println(F("AM2315 Sensor not found, check wiring & pullups!"));
    AM2315_detected=false;
    Serial.print(F("AM2315_detected set to "));
    Serial.println(AM2315_detected);
}else{
    Serial.println(F("AM2315 Sensor Detected!"));
    AM2315_detected=true;
    Serial.print(F("AM2315_detected set to "));
    Serial.println(AM2315_detected);
    Wire.setClock(31000L);//reset the TWI bus to a slower clock of 31,000 Hz to allow better data transmission over a greater length of cable
    Serial.println(F("wire clock set to 31,000 Hz"));
    am2315.readHumidity();
    delay(100);
    am2315.readTemperature();
}

    localip[0] = EEPROM.read(LOCALIPADDREEPROMADDRPART1);
    localip[1] = EEPROM.read(LOCALIPADDREEPROMADDRPART2);
    localip[2] = EEPROM.read(LOCALIPADDREEPROMADDRPART3);
    localip[3] = EEPROM.read(LOCALIPADDREEPROMADDRPART4);
    subnetmask[0] = EEPROM.read(SUBNETMASKEEPROMADDRPART1);
    subnetmask[1] = EEPROM.read(SUBNETMASKEEPROMADDRPART2);
    subnetmask[2] = EEPROM.read(SUBNETMASKEEPROMADDRPART3);
    subnetmask[3] = EEPROM.read(SUBNETMASKEEPROMADDRPART4);
    gateway[0] = EEPROM.read(GATEWAYEEPROMADDRPART1);
    gateway[1] = EEPROM.read(GATEWAYEEPROMADDRPART2);
    gateway[2] = EEPROM.read(GATEWAYEEPROMADDRPART3);
    gateway[3] = EEPROM.read(GATEWAYEEPROMADDRPART4);
    dnsServerIp[0] = EEPROM.read(DNSSERVEREEPROMADDRPART1);
    dnsServerIp[1] = EEPROM.read(DNSSERVEREEPROMADDRPART2);
    dnsServerIp[2] = EEPROM.read(DNSSERVEREEPROMADDRPART3);
    dnsServerIp[3] = EEPROM.read(DNSSERVEREEPROMADDRPART4);
    
    Ethernet.begin(mac, localip, dnsServerIp, gateway, subnetmask); 
    Serial.println(F("System is using Static Ethernet Settings."));

    Serial.print(F("System IP address is "));
    Serial.println(ip_to_str(localip));
  
    Serial.print(F("Gateway address is "));
    Serial.println(ip_to_str(gateway));
  
    Serial.print(F("DNS IP address is "));
    Serial.println(ip_to_str(dnsServerIp));
    
    Serial.print(F("Subnet Mask is "));
    Serial.println(ip_to_str(subnetmask));

}

void loop(){
delay(2000);//DEBOUNCE 
  currentMillis = millis();


//****************************************************************************
//Start process the temperature and humidity
//****************************************************************************
  if (AM2315_detected==true){
     if(currentMillis - temp_hum_previousMillis > temp_hum_interval) { // process once per interval
     // float average_temp[6]=0;
    //float average_hum[6]=0;
    //byte average_counter=0;
    if (average_counter <6){
          average_hum[average_counter] = am2315.readHumidity();
          delay(100);
          average_temp[average_counter] = (am2315.readTemperature() *1.8)+32;
           Serial.print(F("Hum: ")); Serial.println(average_hum[average_counter]);
           Serial.print(F("Temp: ")); Serial.println(average_temp[average_counter]);
           average_counter++;
          temp_hum_previousMillis = currentMillis; 
      }else{
        average_counter=0;
        average_hum[average_counter] = am2315.readHumidity();
          delay(100);
          average_temp[average_counter] = (am2315.readTemperature() *1.8)+32;
           Serial.print(F("Hum: ")); Serial.println(average_hum[average_counter]);
           Serial.print(F("Temp: ")); Serial.println(average_temp[average_counter]);
           average_counter++;
          temp_hum_previousMillis = currentMillis; 
        Serial.println(F("Logging 2nd floor average temperature"));
        Serial.print(F("Average Hum: ")); Serial.println((average_hum[0] + average_hum[1] + average_hum[2] + average_hum[3] + average_hum[4] + average_hum[5])/6);
           Serial.print(F("Average Temp: ")); Serial.println((average_temp[0] + average_temp[1] + average_temp[2] + average_temp[3] + average_temp[4] + average_temp[5])/6);
          if (client.connect(serverip,80)) { // REPLACE WITH YOUR SERVER ADDRESS
              Serial.println(F("Client Connected updating 2nd floor temperature logs"));
              client.print(F("GET /admin/second_floor_add.php?temp="));
              client.print((average_temp[0] + average_temp[1] + average_temp[2] + average_temp[3] + average_temp[4] + average_temp[5])/6);
              client.print(F("&hum="));
              client.print((average_hum[0] + average_hum[1] + average_hum[2] + average_hum[3] + average_hum[4] + average_hum[5])/6);
              client.println( F(" HTTP/1.1"));
              client.println( F("Host: 192.168.1.13") );
              client.println( F("Content-Type: application/x-www-form-urlencoded") );
              client.println( F("Connection: close") );
              client.println();
              client.println();
              client.println( F("Connection: close") );
              client.println();
              client.println();
              client.println( F("Connection: close") );
              client.println();
              client.println();
              client.stop();
              client.stop();
          } else{
            Serial.println(F("could not connect to server"));
          }
      }
    }else{
    }
  }

}

// Just a utility function to nicely format an IP address.
const char* ip_to_str(const uint8_t* ipAddr)
{
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}

