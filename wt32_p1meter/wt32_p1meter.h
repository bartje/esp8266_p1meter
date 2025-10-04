/**
 * Created by Adam on 2025-09-18.
 */

#ifndef WT32_ETH01_MQTT_WT32_ETH01_MQTT_H
#define WT32_ETH01_MQTT_WT32_ETH01_MQTT_H

#include "Ethernet.h"
#include "PubSubClient.h"
#include <time.h>

#define RXD2 5
#define TXD2 17

// Debug
#define NO_NETWORK true

// about the software version
const unsigned int VERSION				= 20250921;     // Versie van de software

// * Baud rate for hardware serial2
#define BAUD_RATE 115200

// ELEGANT_OTA
const char *User							="bartje";
const char *Password						="bartje";

// ntp-server
const char* ntpServer						= "router.witje";			// NTP server to request epoch time

//*********/
// P1-port
//*********/
#define UPDATE_INTERVAL 9500  // 1 minute
#define P1_MAXLINELENGTH 5050
// * Set to store received telegram
char telegram[P1_MAXLINELENGTH];

// * Set to store the data values read
struct timestampData{
	bool valid_data;
	std::string meterTimestamp;
	long epochTimestamp;
};
timestampData TIMESTAMP;


//*********/
// MQTT
//*********/
long LAST_UPDATE_SENT = 0;


#endif  //WT32_ETH01_MQTT_WT32_ETH01_MQTT_H