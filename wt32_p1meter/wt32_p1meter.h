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
#define NO_NETWORK false
#define ENABLE_LOG false
#define PRODUCTION true


// about the software version
const unsigned int VERSION				= 20251031;     // Versie van de software

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
#define UPDATE_INTERVAL 5  // in seconds
#define P1_MAXLINELENGTH 5050
// * Set to store received telegram
char telegram[P1_MAXLINELENGTH];
int counter		=	0;
float P_tot		=	0;
float P_tot_pos	=	0;
float P_tot_neg	=	0;
float VL1		=	0;
float VL2		=	0;
float VL3		=	0;
float IL1		=	0;
float IL2		=	0;
float IL3		=	0;


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
const char *mqtt_topic_elek					= "power/home/p1/energy";
const char *mqtt_topic_elek_inst			= "power/home/p1/energy/instant";
const char *mqtt_topic_gas					= "home/verbruik/p1/gas";
const char *mqtt_topic_water				= "home/verbruik/p1/water";

#endif  //WT32_ETH01_MQTT_WT32_ETH01_MQTT_H