/**
 * This program will use the Ethernet port on a WT32-ETH01 devkit and connect to a Fluvius Smart meter.
 * It will publish data via MQTT
 */
#include "wt32_p1meter.h"
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <DateTimeFunctions.h>

#include <WebServer.h>
#include <ElegantOTA.h>

#include "arduino-dsmr-2/fields.h"
#include "arduino-dsmr-2/packet_accumulator.h"
#include "arduino-dsmr-2/parser.h"
#include <iostream>

using namespace arduino_dsmr_2;
using namespace fields;


//oude code
/*
#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

*/


WebServer server(80);

void onOTAStart() {
	// Log when OTA has started
	Serial.println("OTA update started!");
	// <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
	// Log every 1 second
	if (millis() - ota_progress_millis > 1000) {
		ota_progress_millis = millis();
    	Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
	}
}

void onOTAEnd(bool success) {
	// Log when OTA has finished
	if (success) {
    	Serial.println("OTA update finished successfully!");
	} else {
    	Serial.println("There was an error during OTA update!");
	}
  // <Add your own code here>
}



// * Set The Class Object Name
DateTimeFunctions dTF;

// * om de info vanuit de P1 poort te verzamelen
//PacketAccumulator accumulator(/* bufferSize */ P1_MAXLINELENGTH, /* check_crc */ true);
PacketAccumulator accumulator(/* bufferSize */ P1_MAXLINELENGTH, /* check_crc */ false);

using MyData = ParsedData<
    /* String */ identification,
	/* String */ p1_version_be,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ energy_delivered_tariff1,
	/* FixedValue */ energy_delivered_tariff2,
	/* FixedValue */ energy_returned_tariff1,
	/* FixedValue */ energy_returned_tariff2,
	/* String */ electricity_tariff,
	/* FixedValue */ power_delivered,
	/* FixedValue */ power_returned,
	// /* uint32_t */ electricity_failures,
	// /* uint32_t */ electricity_long_failures,
	// /* String */ electricity_failure_log,
	// /* String */ message_short,
	// /* String */ message_long,
	/* FixedValue */ current_l1,
	/* FixedValue */ current_l2,
	/* FixedValue */ current_l3,
	/* FixedValue */ power_delivered_l1,
	/* FixedValue */ power_delivered_l2,
	/* FixedValue */ power_delivered_l3,
	/* FixedValue */ power_returned_l1,
	/* FixedValue */ power_returned_l2,
	/* FixedValue */ power_returned_l3,
	/* FixedValue */ active_energy_import_current_average_demand,
	/*TimestampedFixedValue*/ active_energy_import_maximum_demand_running_month,
	/*TimestampedFixedValue*/ gas_delivered_be,
	/*TimestampedFixedValue*/water_delivered>;
// * Initiate WIFI client
//WiFiClient espClient;

// * Initiate MQTT client
//PubSubClient mqtt_client(espClient);



// **********************************
// * MQTT                           *
// **********************************

/*
// * Send a message to a broker topic
void send_mqtt_message(const char *topic, char *payload)
{
    Serial.printf("MQTT Outgoing on %s: ", topic);
    Serial.println(payload);

    bool result = mqtt_client.publish(topic, payload, false);

    if (!result)
    {
        Serial.printf("MQTT publish to topic %s failed\n", topic);
    }
}

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect()
{
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES)
    {
        MQTT_RECONNECT_RETRIES++;
        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
        {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char *message = new char[16 + strlen(HOSTNAME) + 1];
            strcpy(message, "p1 meter alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            Serial.printf("MQTT root topic: %s\n", MQTT_ROOT_TOPIC);
        }
        else
        {
            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES)
    {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        return false;
    }

    return true;
}

void send_metric(String name, long metric)
{
    Serial.print(F("Sending metric to broker: "));
    Serial.print(name);
    Serial.print(F("="));
    Serial.println(metric);

    char output[10];
    ltoa(metric, output, sizeof(output));

    String topic = String(MQTT_ROOT_TOPIC) + "/" + name;
    send_mqtt_message(topic.c_str(), output);
}


void send_data(){
	// !! hoe er zeker van zijn dat je geen oude data opstuurt --
	
	// maak verschillende json met de relevante data in
	// meterstanden Elek
	// meterstanden Gas
	// meterstanden Water
	// historische data
	// https://arduinojson.org/v7/example/

	// originele SMA meter
	//power/home/energy (gemiddelde waarden 5 seconden)
	//	{"time": 1748202861, "E_tot_pos": 17851.0626, "E_tot_neg": 14327.9693, "E_tot": 3523.0933, "P_tot_pos": 337.2, "P_tot_neg": 0.0, "P_tot": 337.2, "Cosphi": 0.81, "VL1L3": 232.6, "VL2L3": 233.3}
	
	//power/home/energy/instant (instantane waarde per 5 seconden)
	//	{"time": 1748202861, "P_tot_inst": 336.4}

	// *****************
	// *     Elek      *
	// *****************

	// decode TIMESTAMP
		// TIMESTAMP.timestamp = 221028213843; 2022 10 28 // 21u 38m 43s
		// TIMESTAMP.zomeruur = true;
		// datetime
	
	float P_tot = L1_INSTANT_POWER_USAGE + L2_INSTANT_POWER_USAGE+ L3_INSTANT_POWER_USAGE - L1_INSTANT_POWER_PRODUCTION - L2_INSTANT_POWER_PRODUCTION - L3_INSTANT_POWER_PRODUCTION;
	float P_tot_pos = 0;
	float P_tot_neg = 0;
	if(P_tot > 0){
		P_tot_pos = abs(P_tot);
	} else {
		P_tot_neg = abs(P_tot);
	}

	JsonDocument energy;
	energy["time"] = epochUTC(TIMESTAMP.timestamp, TIMESTAMP.zomeruur);
	energy["E_tot_pos"] = CONSUMPTION_LOW_TARIF + CONSUMPTION_HIGH_TARIF;
	energy["E_tot_neg"] = RETURNDELIVERY_LOW_TARIF + RETURNDELIVERY_HIGH_TARIF;
	energy["E_tot"] = CONSUMPTION_LOW_TARIF + CONSUMPTION_HIGH_TARIF - (RETURNDELIVERY_LOW_TARIF + RETURNDELIVERY_HIGH_TARIF);
	energy["P_tot_pos"] = P_tot_pos;
	energy["P_tot_neg"] = P_tot_neg;
	energy["P_tot"] = P_tot;
	energy["VL1"] = L1_VOLTAGE;
	energy["VL2"] = L2_VOLTAGE;
	energy["VL3"] = L3_VOLTAGE;

}
*/
long epochUTC(long date, bool summerTime) {
	// Convert date time to unix time.  
		// uint32_t conDT2UT(const uint8_t _DAY, const uint8_t _MONTH, const uint16_t _YEAR, const uint8_t _HOUR, const uint8_t _MIN, const uint8_t _SEC);
		// Returns: 0 ... 4294967295

		// de tijd die we krijgen is in onze tijdzone met of zonder zomertijd
		// de convertfunctie verwacht UTC time
		// eerste converteren en dan de aanpassing doen
	char buffer [sizeof(long)*8+1];				//buffer: 221028213843
	ltoa(date,buffer,10);
	
	char YY[3] = {0};
  	memcpy(&YY, &buffer[0], sizeof(YY)-1);		// YY: 22
	char MM[3] = {0};
  	memcpy(&YY, &buffer[2], sizeof(MM)-1);		// MM: 10
	char DD[3] = {0};
  	memcpy(&YY, &buffer[4], sizeof(DD)-1);		// DD: 28
	char hh[3] = {0};
  	memcpy(&YY, &buffer[4], sizeof(hh)-1);		// hh: 21
	char mm[3] = {0};
  	memcpy(&YY, &buffer[4], sizeof(mm)-1);		// mm: 38
	char ss[3] = {0};
  	memcpy(&YY, &buffer[4], sizeof(ss)-1);		// ss: 43
	
	uint32_t epochtime = 0;
	epochtime = dTF.conDT2UT(atoi(DD),atoi(MM),2000 + atoi(YY),atoi(hh),atoi(mm),atoi(ss));
	int delta;
	if(summerTime){
		delta = 2*60*60;
	} else {
		delta = 1*60*60;
	}
	// nu nog de tijdzone in zomertijd in rekening brengen (seconden aftrekken of optellen.)
	return long(epochtime-delta);
}
 /*
// **********************************
// * P1                             *
// **********************************

unsigned int CRC16(unsigned int crc, unsigned char *buf, int len)
{
	for (int pos = 0; pos < len; pos++)
    {
		crc ^= (unsigned int)buf[pos];    // * XOR byte into least sig. byte of crc
                                          // * Loop over each bit
        for (int i = 8; i != 0; i--)
        {
            // * If the LSB is set
            if ((crc & 0x0001) != 0)
            {
                // * Shift right and XOR 0xA001
                crc >>= 1;
				crc ^= 0xA001;
			}
            // * Else LSB is not set
            else
                // * Just shift right
                crc >>= 1;
		}
	}
	return crc;
}

bool isNumber(char *res, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
            return false;
    }
    return true;
}

int FindCharInArrayRev(char array[], char c, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if (array[i] == c)
            return i;
    }
    return -1;
}

int FindCharInArray(char array[], char c, int len)
{
    for (int i = 0; i >= len - 1; i++)
    {
        if (array[i] == c)
            return i;
    }
    return -1;
}


float getValue(char *buffer, int maxlen, char startchar, char endchar)
{
    int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (endchar == '*')
        {
            if (isNumber(res, l))
                // * Lazy convert float to long
                //return (1000 * atof(res));
				// return a float
				return (atof(res));
        }
        else if (endchar == ')')
        {
            if (isNumber(res, l))
                return atof(res);
        }
    }
    return 0;
}

struct timedValue getTimedValue(char *buffer, int maxlen, char firststartchar, char firstendchar, char laststartchar, char lastendchar){
	// (221028213843S)(00.378*kW)
    int fs = FindCharInArrayRev(buffer, firststartchar, maxlen - 2);			// startpositie
    int fl = FindCharInArrayRev(buffer, firstendchar, maxlen - 2) - fs;		  	// lengte  (eindkarakter telt wel mee inhoud)
	int ls = FindCharInArrayRev(buffer, laststartchar, maxlen - 2);				// startpositie
    int ll = FindCharInArrayRev(buffer, lastendchar, maxlen - 2) - ls - 1;		// lengte
	
	struct timedValue timedValue_instance;
    char res[16];
    memset(res, 0, sizeof(res));

	// de datum eruit halen
	// hier gebruiken we de getDate function
	if (strncpy(res, buffer + fs, fl))		//strncpy kopiert de eerste 'l' karakters, te beginnen bij positie 's' (want eerste karakter moet wel mee) 
    // (221028213843S)
	{
		//timestampData tijdstip;
		//tijdstip = getDate(res, sizeof(res),'(',')');
		timedValue_instance.timestamp = getDate(res, sizeof(res),'(',')');
    }

	memset(res, 0, sizeof(res));
	// de value eruit halen
    if (strncpy(res, buffer + ls + 1, ll))		//strncpy kopiert de eerste 'l' karakters, te beginnen bij positie 's+1' (want eerste karakter moet niet mee) 
    {
        if (lastendchar == '*')
        {
            if (isNumber(res, ll))
                // * Lazy convert float to long
				//timedValue_instance.value = 1000 * atof(res);
				timedValue_instance.value = atof(res);
        }
        else if (lastendchar == ')')
        {
            if (isNumber(res, ll))
                timedValue_instance.value = atof(res);
        }
    }
    return timedValue_instance;
}
 */

struct timestampData getDate(const std::string& buffer){
    // 170531201444S	
	bool VALID = false;
	bool SUMMER = false;
	
	if (buffer[12] == 'S') {
		SUMMER = true;
		VALID = true;
	}
	else if (buffer[12] == 'W'){
		SUMMER = false;
		VALID = true;
	}
	
	struct timestampData timestampData_instance;
	timestampData_instance.valid_data = VALID;
	
	//indien er een datum wordt ingelezen --> naar epoch omzetten
	if(VALID){
		timestampData_instance.meterTimestamp = buffer;
		// Convert date time to unix time.  
		// uint32_t conDT2UT(const uint8_t _DAY, const uint8_t _MONTH, const uint16_t _YEAR, const uint8_t _HOUR, const uint8_t _MIN, const uint8_t _SEC);
		// Returns: 0 ... 4294967295

		// de tijd die we krijgen is in onze tijdzone met of zonder zomertijd
		// de convertfunctie verwacht UTC time
		// eerste converteren en dan de aanpassing doen
		
		char YY[3] = {0};
  		memcpy(&YY, &buffer[0], sizeof(YY)-1);		// YY: 22
		char MM[3] = {0};
		memcpy(&MM, &buffer[2], sizeof(MM)-1);		// MM: 10
		char DD[3] = {0};
		memcpy(&DD, &buffer[4], sizeof(DD)-1);		// DD: 28
		char hh[3] = {0};
		memcpy(&hh, &buffer[6], sizeof(hh)-1);		// hh: 21
		char mm[3] = {0};
		memcpy(&mm, &buffer[8], sizeof(mm)-1);		// mm: 38
		char ss[3] = {0};
		memcpy(&ss, &buffer[10], sizeof(ss)-1);		// ss: 43
		//Serial.print("during Epoch: ");
		//Serial.print(YY);
		//Serial.print(MM);
		//Serial.print(DD);
		//Serial.print(hh);
		//Serial.print(mm);
		//Serial.println(ss);
		uint32_t epochtime = 0;
		epochtime = dTF.conDT2UT(atoi(DD),atoi(MM),2000 + atoi(YY),atoi(hh),atoi(mm),atoi(ss));
		int delta;
		if(SUMMER){
			delta = 2*60*60;
		} else {
			delta = 1*60*60;
		}
		// nu nog de tijdzone in zomertijd in rekening brengen (seconden aftrekken of optellen.)
		timestampData_instance.epochTimestamp = long(epochtime-delta);
	}
    return timestampData_instance;
	
}
/*
bool decode_telegram(int len)
{	
	int startChar = FindCharInArrayRev(telegram, '/', len);		// eerste character van de eerste verzending uit de informatietrein
    int endChar = FindCharInArrayRev(telegram, '!', len);		// eerste character van de laatste verzending uit de informatietrein
    bool validCRCFound = false;


    if (startChar >= 0)
    {
        // * Start found. Reset CRC calculation
        currentCRC = CRC16(0x0000,(unsigned char *) telegram+startChar, len-startChar);
		//Serial.print("    first line; CRC: ");
		//Serial.println(currentCRC);
    }
    else if (endChar >= 0)
    {
        // * Add to crc calc
        currentCRC = CRC16(currentCRC,(unsigned char*)telegram+endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0;   // * Thanks to HarmOtten (issue 5)
        validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);
		
		// #########**************** test
		Serial.print("    last line; CRC: ");
		Serial.println(currentCRC);

        if (validCRCFound)
            Serial.println(F("CRC Valid!"));
        else
            Serial.println(F("CRC Invalid!"));

        currentCRC = 0;
    }
    else
    {
        currentCRC = CRC16(currentCRC, (unsigned char*) telegram, len);
		//Serial.print("    other line; CRC: ");
		//Serial.println(currentCRC);
    }
	
	// **********************************
	// * Timestamp Elek                 *
	// **********************************

    // 0-0:1.0.0(170531201444S)
    // 0-0:1.0.0 = timestamp
    if (strncmp(telegram, "0-0:1.0.0", strlen("0-0:1.0.0")) == 0)
    {
		TIMESTAMP = getDate(telegram, len, '(', ')');
    }

	// **********************************
	// * Meterstanden Elek              *
	// **********************************

    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    {
        
		CONSUMPTION_LOW_TARIF = getValue(telegram, len, '(', '*');
		//Serial.print("  CONSUMPTION_LOW_TARIF: ");
		//Serial.println(CONSUMPTION_LOW_TARIF);
    }

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    {
        CONSUMPTION_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }
	
    // 1-0:2.8.1(000560.157*kWh)
    // 1-0:2.8.1 = Elektra teruglevering laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    {
        RETURNDELIVERY_LOW_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.8.2(000560.157*kWh)
    // 1-0:2.8.2 = Elektra teruglevering hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    {
        RETURNDELIVERY_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }

	// **********************************
	// * Meterstanden Gas               *
	// **********************************

	// 0-1:24.2.1(150531200000S)(00811.923*m3)
    // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
    if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
    {
        GAS_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');
    }
	
	
    // 0-1:24.2.3(150531200000S)(00811.923*m3)
    // 0-1:24.2.3 = Gas on Belgian meters
    if (strncmp(telegram, "0-1:24.2.3", strlen("0-1:24.2.3")) == 0)
    {
        GAS_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');
    }
	// **********************************
	// * Meterstanden Water             *
	// **********************************

	// 0-2:24.2.1(221028213843S)(00004.332*m3)
    // 0-2:24.2.1 = water on Belgian meters
    if (strncmp(telegram, "0-2:24.2.1", strlen("0-2:24.2.1")) == 0)
    {
        WATER_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');
	}
	
	// **********************************
	// * Vermogen Elek                  *
	// **********************************

    // 1-0:1.7.0(00.424*kW) Actueel verbruik = som van verbruik per fase
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    {
        ACTUAL_CONSUMPTION = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.7.0(00.000*kW) Actuele teruglevering (-P) in 1 Watt resolution = som van teruglevering per fase
    if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    {
        ACTUAL_RETURNDELIVERY = getValue(telegram, len, '(', '*');
    }

    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering L1
    if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0)
    {
        L1_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:41.7.0(00.378*kW)
    // 1-0:41.7.0 = Instantaan vermogen Elektriciteit levering L2
    if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0)
    {
        L2_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:61.7.0(00.378*kW)
    // 1-0:61.7.0 = Instantaan vermogen Elektriciteit levering L3
    if (strncmp(telegram, "1-0:61.7.0", strlen("1-0:61.7.0")) == 0)
    {
        L3_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

	// 1-0:22.7.0(00.378*kW)
    // 1-0:22.7.0 = Instantaan vermogen Elektriciteit productie L1
    if (strncmp(telegram, "1-0:22.7.0", strlen("1-0:22.7.0")) == 0)
    {
        L1_INSTANT_POWER_PRODUCTION = getValue(telegram, len, '(', '*');
    }

    // 1-0:42.7.0(00.378*kW)
    // 1-0:42.7.0 = Instantaan vermogen Elektriciteit productie L2
    if (strncmp(telegram, "1-0:42.7.0", strlen("1-0:42.7.0")) == 0)
    {
        L2_INSTANT_POWER_PRODUCTION = getValue(telegram, len, '(', '*');
    }

    // 1-0:62.7.0(00.378*kW)
    // 1-0:62.7.0 = Instantaan vermogen Elektriciteit productie L3
    if (strncmp(telegram, "1-0:62.7.0", strlen("1-0:62.7.0")) == 0)
    {
        L3_INSTANT_POWER_PRODUCTION = getValue(telegram, len, '(', '*');
    }

	// **********************************
	// * Spanning en stroom Elek        *
	// **********************************

    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantane stroom Elektriciteit L1
    if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0)
    {
        L1_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    
	// 1-0:51.7.0(002*A)
    // 1-0:51.7.0 = Instantane stroom Elektriciteit L2
    if (strncmp(telegram, "1-0:51.7.0", strlen("1-0:51.7.0")) == 0)
    {
        L2_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    
	// 1-0:71.7.0(002*A)
    // 1-0:71.7.0 = Instantane stroom Elektriciteit L3
    if (strncmp(telegram, "1-0:71.7.0", strlen("1-0:71.7.0")) == 0)
    {
        L3_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }

    // 1-0:32.7.0(232.0*V)
    // 1-0:32.7.0 = Voltage L1
    if (strncmp(telegram, "1-0:32.7.0", strlen("1-0:32.7.0")) == 0)
    {
        L1_VOLTAGE = getValue(telegram, len, '(', '*');
    }
    
	// 1-0:52.7.0(232.0*V)
    // 1-0:52.7.0 = Voltage L2
    if (strncmp(telegram, "1-0:52.7.0", strlen("1-0:52.7.0")) == 0)
    {
        L2_VOLTAGE = getValue(telegram, len, '(', '*');
    }   
    
	// 1-0:72.7.0(232.0*V)
    // 1-0:72.7.0 = Voltage L3
    if (strncmp(telegram, "1-0:72.7.0", strlen("1-0:72.7.0")) == 0)
    {
        L3_VOLTAGE = getValue(telegram, len, '(', '*');
    }

	// **********************************
	// * Varia Elek                     *
	// **********************************

    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
    {
        ACTUAL_TARIF = getValue(telegram, len, '(', ')');
    }

    // 0-0:96.7.21(00003)
    // 0-0:96.7.21 = Aantal onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.21", strlen("0-0:96.7.21")) == 0)
    {
        SHORT_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }

    // 0-0:96.7.9(00001)
    // 0-0:96.7.9 = Aantal lange onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.9", strlen("0-0:96.7.9")) == 0)
    {
        LONG_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }

    // 1-0:32.32.0(00000)
    // 1-0:32.32.0 = Aantal korte spanningsdalingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.32.0", strlen("1-0:32.32.0")) == 0)
    {
        SHORT_POWER_DROPS = getValue(telegram, len, '(', ')');
    }

    // 1-0:32.36.0(00000)
    // 1-0:32.36.0 = Aantal korte spanningsstijgingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.36.0", strlen("1-0:32.36.0")) == 0)
    {
        SHORT_POWER_PEAKS = getValue(telegram, len, '(', ')');
    }

	// **********************************
	// * Kwartierwaarden Elek           *
	// **********************************

	// 1-0:1.4.0(00.378*kW)
    // 1-0:1.4.0 = kwartierwaarde
    if (strncmp(telegram, "1-0:1.4.0", strlen("1-0:1.4.0")) == 0)
    {
        QUARTER_VALUE = getValue(telegram, len, '(', '*');
    }

	// 1-0:1.6.0(221028213843S)(00.378*kW)
    // 1-0:1.6.0 = tijdstip en  max kwartierpiek deze maand
    if (strncmp(telegram, "1-0:1.6.0", strlen("1-0:1.6.0")) == 0)
    {
        QUARTER_PEAK_CURRENT_MONTH = getTimedValue(telegram, len, '(', ')','(','*');
    }

    return validCRCFound;
	///return true;
}

*/

void read_p1_hardwareserial(){
	MyData data;
	bool first = true;
	while (Serial2.available())	{
        if(first){
			Serial.print("start: ");
			Serial.println(millis());
			first=false;
		}
		
		// First step is to get the full message from the P1 port.
		// In this example, we go through the bytes from the message above.
		// In a real application, you need to read the bytes from the UART one byte at a time.
		//for (const auto& byte : data_from_p1_port) {
			// feed the byte to the accumulator
			auto res = accumulator.process_byte(Serial2.read());

			// During receiving, errors may occur, such as CRC mismatches.
			// You can optionally log these errors, or ignore them.
			if (res.error()) {
				Serial.printf("Error during receiving a packet: %s", to_string(*res.error()));
			}

			// When a full packet is received, the packet() method will return it.
			// The packet starts with '/' and ends with the '!'.
			// The CRC is not included.
			if (res.packet()) {
				// Parse the received packet.
				const auto packet = *res.packet();
				Serial.println(packet.data());
				// Specify `check_crc` as false, since the accumulator already checked the CRC and didn't include it in the packet
				P1Parser::parse(&data, packet.data(), packet.size(), /* unknown_error */ false, /* check_crc */ false);
				LAST_UPDATE_SENT = millis();
				
				// Now you can use the parsed data.
				//data.applyEach(Printer());
				
				// strings
				Serial.printf("Identification: %s\n", data.identification.c_str());
				Serial.printf("P1 version: %s\n", data.p1_version_be.c_str());
				Serial.printf("Timestamp: %s\n", data.timestamp.c_str());
				Serial.printf("Equipment ID: %s\n", data.equipment_id.c_str());
				
				// FixedValue
				Serial.printf("Energy delivered tariff 1: %.3f\n", static_cast<double>(data.energy_delivered_tariff1.val()));
				Serial.printf("Energy delivered tariff 2: %.3f\n", static_cast<double>(data.energy_delivered_tariff2.val()));
				Serial.printf("Energy returned tariff 1: %.3f\n", static_cast<double>(data.energy_returned_tariff1.val()));
				Serial.printf("Energy returned tariff 2: %.3f\n", static_cast<double>(data.energy_returned_tariff2.val()));
				Serial.printf("Power delivered: %.3f\n", static_cast<double>(data.power_delivered.val()));
				Serial.printf("Power returned: %.3f\n", static_cast<double>(data.power_returned.val()));

				Serial.printf("Power delivered L1: %.3f\n", static_cast<double>(data.power_delivered_l1.val()));
				Serial.printf("Power returned L1: %.3f\n", static_cast<double>(data.power_returned_l1.val()));
				Serial.printf("Power delivered L2: %.3f\n", static_cast<double>(data.power_delivered_l2.val()));
				Serial.printf("Power returned L3: %.3f\n", static_cast<double>(data.power_returned_l2.val()));
				Serial.printf("Power delivered L3: %.3f\n", static_cast<double>(data.power_delivered_l3.val()));
				Serial.printf("Power returned L3: %.3f\n", static_cast<double>(data.power_returned_l3.val()));

				Serial.printf("Current L1: %.3f\n", static_cast<double>(data.current_l1.val()));
				Serial.printf("Current L2: %.3f\n", static_cast<double>(data.current_l2.val()));
				Serial.printf("Current L3: %.3f\n", static_cast<double>(data.current_l3.val()));

				// TimestampedFixedValue
				Serial.printf("Gasverbruik: %.3f\n", static_cast<double>(data.gas_delivered_be.val()));
				Serial.printf("Gas timestamp: %s\n", data.gas_delivered_be.timestamp.c_str());
				Serial.printf("Waterverbruik: %.3f\n", static_cast<double>(data.water_delivered.val()));
				Serial.printf("Water timestamp: %s\n", data.water_delivered.timestamp.c_str());
				
				timestampData testtime = getDate(data.gas_delivered_be.timestamp);
				if(testtime.valid_data){
					Serial.print("gas epoch: ");
					Serial.println(testtime.epochTimestamp);
				}
				Serial.print("end: ");
				Serial.println(millis());

				//--> maak nu boodschappen voor mqtt.  opgelet inhoud controleren alvorens te versturen.
			}
		//}

		/*
		//Serial.println("Serial.available");
		
		memset(telegram, 0, sizeof(telegram));
		
		//Serial.print("telegram value: ");
		//Serial.println(telegram);
		int counter = 0;
        
		while (Serial.available())
        {
            //Serial.print("Serial.available loop: ");
			//Serial.println(counter);
			counter++;
			
			ESP.wdtDisable();			//watchdog disable, geen idee waarom
            int len = Serial.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);
            ESP.wdtEnable(1);
			
			//Serial.print("gelezen bytes: ");
			//Serial.println(len);
			//Serial.print("telegram value: ");
			//Serial.println(telegram);
            
			// voeg de telegram toe aan complete_telegram
			if (log_telegrams){
				//total_len += len;
				char message[len+2];
        		strncpy(message, telegram, len+1);
				//strcpy (complete_telegram,telegram);
				strcat(complete_telegram,message);
				//Serial.print("Temp_telegram value: ");
				//Serial.println(complete_telegram);
				//Serial.print("C: ");
				//Serial.println(counter);

			}
			
  		

			processLine(len);
        }
		if (log_telegrams){
			Serial.print("C: ");
			Serial.println(counter);
			Serial.print("Complete_telegram value: ");
			Serial.println(complete_telegram);
		}
		*/
    }
}

/*
void processLine(int len) {
    //Serial.print("ProcessLine length: ");
	//Serial.println(len);
	
	telegram[len] = '\n';
    telegram[len + 1] = 0;
    yield();		// ook iets te maken met de hardware watchdog.

    bool result = decode_telegram(len + 1);
	// result is enkel TRUE wanneer alle lijnen van een volledig telegram goed binnengekomen zijn CRC_VALID

    if (result) {
		send_data();
		LAST_UPDATE_SENT = millis();
    }

}

*/

// **********************************
// * Setup Main                     *
// **********************************

void setup()
{
    delay(1000);
	// Serial via USB
	Serial.begin(115200);
	while(!Serial)
		delay(100);
	
	Serial.println( "\n" );
	Serial.println( "Function setup() is beginning." );
	Serial.printf( "Softwareversion %d\n", VERSION );
	if(!NO_NETWORK){
		WiFi.onEvent( NetworkEvent );
		Serial.println( "Network callbacks configured." );
		ETH.begin();
		// This delay give the Ethernet hardware time to initialize.
		delay(300);
		Serial.print("Connecting to Ethernet");
		while(!eth_connected){
			if (ethernetConnectionAttempts>maxEthernetConnectionAttempts){
				ESP.restart();
			} else {
				ethernetConnectionAttempts++;
				Serial.print(".");
				delay(500);
			}
		}
	
		//reset the ethernetConnectionAttempts
		ethernetConnectionAttempts = 0;

		Serial.println("OK");
		Serial.print("Connecting to MQTT broker");
		while( eth_connected && !mqttClient.connected() ){
			mqttConnect(BROKER_ADDRESS, BROKER_PORT);
			Serial.print(".");
			delay(500);
		}
		Serial.println("OK");
		//printTelemetry();
		// Ethernetverbinding is OK en MQTT is verbonden
		
		// NTP server instellen voor epoch time
		configTime(0, 0, ntpServer);

		// Over The Air updates lanceren
		ElegantOTA.setAuth(User, Password);
		ElegantOTA.begin(&server);    // Start ElegantOTA
		// ElegantOTA callbacks
		ElegantOTA.onStart(onOTAStart);
		ElegantOTA.onProgress(onOTAProgress);
		ElegantOTA.onEnd(onOTAEnd);
		server.begin();
		Serial.println("HTTP server started");
	}
	

    // Setup a hw serial connection for communication with the P1 meter and logging (not using inversion)
    //Serial1.begin(BAUD_RATE, SERIAL_8N1, SERIAL_FULL);
	Serial2.begin(BAUD_RATE, SERIAL_8N1, RXD2, TXD2, true);		// true --> invert the signals
    Serial.println("init Serial2");
    Serial.println("Swapping UART0 RX to inverted");
    Serial.flush();

    // Invert the RX serialport by setting a register value, this way the TX might continue normally allowing the serial monitor to read println's
	//// testing
	//USC0(UART0) = USC0(UART0) | BIT(UCRXI);
    Serial.println("Serial2 port is ready to recieve.");


}

// **********************************
// * Loop                           *
// **********************************

void loop()
{
	
	if(!NO_NETWORK){
		//OTA
		server.handleClient();
		ElegantOTA.loop();

		if( eth_connected && !mqttClient.connected() ){
			mqttConnect( BROKER_ADDRESS, BROKER_PORT );
		}
		else{
			mqttClient.loop();
		}
	}
    // test
	// lees serial2 en schrijf naar Serial
	long now = millis();

	
	if (now - LAST_UPDATE_SENT > UPDATE_INTERVAL) {
		read_p1_hardwareserial();
	}
	

	/*
	if(Serial2.available()){
		Serial.println("boodschap op Serial2");

		memset(telegram, 0, sizeof(telegram));
        
		while (Serial2.available())
        {	
			//ESP.wdtDisable();			//watchdog disable, geen idee waarom
            int len = Serial2.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);
            //ESP.wdtEnable(1);
			//Serial.print("gelezen bytes: ");
			//Serial.println(len);
			//Serial.print("telegram value: ");
			Serial.println(telegram);
		}
	}
	*/
	
}
