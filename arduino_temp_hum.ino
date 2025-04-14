#include <Wire.h>
#include <Adafruit_AM2315.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <Ethernet.h>
#include <Dns.h>
#include <avr/wdt.h>

const char* ip_to_str(const uint8_t*);
byte mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };
byte serverip[] = {192, 168, 1, 13};
#define health_id "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
byte skip_network=0;
IPAddress healthcheckio_IP;
byte debug=0;
bool AM2315_detected=false;
Adafruit_AM2315 am2315;
EthernetClient client;
DNSClient dnClient;
unsigned long temp_hum_interval=10000;
unsigned long temp_hum_previousMillis;
unsigned long currentMillis = 0;
unsigned long interval_heartbeat = 900000;				// READING INTERVAL --> how often is healthchecks.io contacted for a heartbeat update? value = 15 minutes 
float average_temp[6];
float average_hum[6];
byte average_counter=0;
long previousMillis_heartbeat = 0;
float average_temperature;
float average_humidity;

// Set the static IP address to use if the DHCP fails to assign
byte ip[] = { 192, 168, 1, 253 }; 
byte myDns[] = { 192, 168, 1, 1 }; 
byte gateway[] = { 192, 168, 1, 1 }; 
byte subnet[] = { 255, 255, 255, 0 }; 

void setup() { 
	wdt_enable(WDTO_4S);
	Serial.begin(115200);
	
//********************************************************************
// Activate Temperature / Humidity Sensor
//********************************************************************
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
	}

//********************************************************************
// start the Ethernet connection:
//********************************************************************
	Serial.println(F("Initialize Ethernet with DHCP:"));
	if (Ethernet.begin(mac) == 0) {
		Serial.println(F("Failed to configure Ethernet using DHCP"));
		// Check for Ethernet hardware present
		if (Ethernet.hardwareStatus() == EthernetNoHardware) {
			Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
			while (true) {
				delay(1); // do nothing, no point running without Ethernet hardware
			}
		}
		if (Ethernet.linkStatus() == LinkOFF) {
			Serial.println(F("Ethernet cable is not connected."));
		}
		// try to configure using IP address instead of DHCP:
		Ethernet.begin(mac, ip, myDns, gateway, subnet);
		skip_network=1;
		// give the Ethernet shield a second to initialize:
		delay(1000);
		Serial.print(F("Static assigned IP "));
		Serial.println(Ethernet.localIP());
	} else {
		// give the Ethernet shield a second to initialize:
		delay(1000);
		Serial.print(F("DHCP assigned IP "));
		Serial.println(Ethernet.localIP());
	}
	if (Ethernet.hardwareStatus() == EthernetW5100) {
		Serial.println(F("W5100 Ethernet controller detected."));
	}else if (Ethernet.hardwareStatus() == EthernetW5200) {
		Serial.println(F("W5200 Ethernet controller detected."));
	}else if (Ethernet.hardwareStatus() == EthernetW5500) {
		Serial.println(F("W5500 Ethernet controller detected."));
	}
  
//********************************************************************
// start DNS subsystem
//********************************************************************  
	dnClient.begin(Ethernet.dnsServerIP());
	if(dnClient.getHostByName("hc-ping.com",healthcheckio_IP) == 1) {
		Serial.print(F("hc-ping.com = "));
		Serial.println(healthcheckio_IP);
		Serial.println("");
		Serial.println("");
	}else{ 
		Serial.println(F("DNS Lookup Failed"));
	}

  if (client.connect(healthcheckio_IP,80)) {
			Serial.println(F("Heartbeat Client Connected"));
			Serial.println("");
			client.print(F("GET /"));
			client.print(health_id);
			client.println(F(" HTTP/1.1"));
			client.println(F("Host: hc-ping.com"));
			client.println(F("Connection: close"));
			client.println();
			client.stop();
		} else{
			Serial.print(F("Heartbeat could not connect to server"));
		}
}

void loop(){
	
//********************************************************************
//maintain DHCP IP lease
//********************************************************************
	if(skip_network==0){
		switch (Ethernet.maintain()) {
		case 1:
			//renewed fail
			Serial.println(F("Error: DHCP renew Fail"));
			break;
		case 2:
			//renewed success
			Serial.println(F("DHCP Renewed Success"));
			//print your local IP address:
			Serial.print(F("My IP address: "));
			Serial.println(Ethernet.localIP());
			break;
		case 3:
			//rebind fail
			Serial.println(F("Error: DHCP Rebind Fail"));
			break;
		case 4:
			//rebind success
			Serial.println(F("DHCP Rebind Success"));
			//print your local IP address:
			Serial.print(F("My IP address: "));
			Serial.println(Ethernet.localIP());
			break;
		default:
			//nothing happened
			break;
		}
	}
    wdt_reset();	
	
	delay(2000);//DEBOUNCE 
	currentMillis = millis();
  
  
  
//********************************************************************
//Connect to health checks to ensure this arduino has not crashed
//********************************************************************
	if(currentMillis - previousMillis_heartbeat > interval_heartbeat) { // PERFORM ONLY ONCE PER INTERVAL
		previousMillis_heartbeat = currentMillis; 
		if(dnClient.getHostByName("hc-ping.com",healthcheckio_IP) == 1) {
			Serial.print(F("hc-ping.com = "));
			Serial.println(healthcheckio_IP);
		}else{
			Serial.print(F("DNS Lookup Failed"));
		}
		if(debug==1){
			Serial.println(F("Heartbeat Times Up"));
		}
		if (client.connect(healthcheckio_IP,80)) {
			Serial.println(F("Heartbeat Client Connected"));
			Serial.println("");
			client.print(F("GET /"));
			client.print(health_id);
			client.println(F(" HTTP/1.1"));
			client.println(F("Host: hc-ping.com"));
			client.println(F("Connection: close"));
			client.println();
			client.stop();
		} else{
			Serial.print(F("Heartbeat could not connect to server"));
		}
	}
  


//****************************************************************************
//Start process the temperature and humidity
//****************************************************************************
	if (AM2315_detected==true){
		if(currentMillis - temp_hum_previousMillis > temp_hum_interval) { // process once per interval
			if (average_counter <6){
				if (! am2315.readTemperatureAndHumidity(&average_temp[average_counter], &average_hum[average_counter])) {
					Serial.println(F("Failed to read data from AM2315"));
					return;
				}
				average_temp[average_counter] = (average_temp[average_counter] *1.8)+32.0;
				Serial.print(F("Hum: ")); Serial.println(average_hum[average_counter]);
				Serial.print(F("Temp: ")); Serial.println(average_temp[average_counter]);
				average_counter++;
				temp_hum_previousMillis = currentMillis; 
			}else{
				average_counter=0;
				average_temperature=(average_temp[0] + average_temp[1] + average_temp[2] + average_temp[3] + average_temp[4] + average_temp[5])/6.0;
				average_humidity=(average_hum[0] + average_hum[1] + average_hum[2] + average_hum[3] + average_hum[4] + average_hum[5])/6.0;
				temp_hum_previousMillis = currentMillis; 
				Serial.println(F("Logging 2nd floor average temperature"));
				Serial.print(F("Average Hum: ")); Serial.println(average_humidity);
				Serial.print(F("Average Temp: ")); Serial.println(average_temperature);
				if (client.connect(serverip,80)) {
					Serial.println(F("Client Connected updating 2nd floor temperature logs"));
					client.print(F("GET /admin/second_floor_add.php?temp="));
					client.print(average_temperature);
					client.print(F("&hum="));
					client.print(average_humidity);
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

