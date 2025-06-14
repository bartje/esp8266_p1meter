#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#include <time.h>
//#include <TimeLib.h>
#include <DateTimeFunctions.h>

// * Include settings
#include "settings.h"

// * Initiate led blinker library
Ticker ticker;

// * Set The Class Object Name
DateTimeFunctions dTF;

// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);

// **********************************
// * WIFI                           *
// **********************************

// * Gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // * If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // * Entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}

// **********************************
// * Ticker (System LED Blinker)    *
// **********************************

// * Blink on-board Led
void tick()
{
    // * Toggle state
    int state = digitalRead(LED_BUILTIN);    // * Get the current state of GPIO1 pin
    digitalWrite(LED_BUILTIN, !state);       // * Set pin to the opposite state
}

// **********************************
// * MQTT                           *
// **********************************

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

/* 
void send_data_to_broker(){
	
	if (FLIPHIGHLOWTARIF) {
        send_metric("consumption_low_tarif", CONSUMPTION_HIGH_TARIF);
        send_metric("consumption_high_tarif", CONSUMPTION_LOW_TARIF);
        send_metric("returndelivery_low_tarif", RETURNDELIVERY_HIGH_TARIF);
        send_metric("returndelivery_high_tarif", RETURNDELIVERY_LOW_TARIF);
        if (ACTUAL_TARIF == 1)
        {
            ACTUAL_TARIF = 2;
        } else {
            ACTUAL_TARIF = 1;
        }
    } else {
        send_metric("consumption_low_tarif", CONSUMPTION_LOW_TARIF);
        send_metric("consumption_high_tarif", CONSUMPTION_HIGH_TARIF);
        send_metric("returndelivery_low_tarif", RETURNDELIVERY_LOW_TARIF);
        send_metric("returndelivery_high_tarif", RETURNDELIVERY_HIGH_TARIF);
    }

    send_metric("actual_consumption", ACTUAL_CONSUMPTION);
    send_metric("actual_returndelivery", ACTUAL_RETURNDELIVERY);

    send_metric("l1_instant_power_usage", L1_INSTANT_POWER_USAGE);
    send_metric("l2_instant_power_usage", L2_INSTANT_POWER_USAGE);
    send_metric("l3_instant_power_usage", L3_INSTANT_POWER_USAGE);
    send_metric("l1_instant_power_current", L1_INSTANT_POWER_CURRENT);
    send_metric("l2_instant_power_current", L2_INSTANT_POWER_CURRENT);
    send_metric("l3_instant_power_current", L3_INSTANT_POWER_CURRENT);
    send_metric("l1_voltage", L1_VOLTAGE);
    send_metric("l2_voltage", L2_VOLTAGE);
    send_metric("l3_voltage", L3_VOLTAGE);
    
    send_metric("gas_meter_m3", GAS_METER_M3);
    send_metric("actual_consumption_gas_m3", ACTUAL_CONSUMPTION_GAS_M3);

    send_metric("actual_tarif_group", ACTUAL_TARIF);
    send_metric("short_power_outages", SHORT_POWER_OUTAGES);
    send_metric("long_power_outages", LONG_POWER_OUTAGES);
    send_metric("short_power_drops", SHORT_POWER_DROPS);
    send_metric("short_power_peaks", SHORT_POWER_PEAKS);
}
 */
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

struct timestampData getDate(char *buffer, int maxlen, char startchar, char endchar)
{
    // (170531201444S)
	int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;
	int S = FindCharInArrayRev(buffer, 'S', maxlen - 2) - s - 1;
	int W = FindCharInArrayRev(buffer, 'W', maxlen - 2) - s - 1;
	bool ZOMER = false;
	if (S != -1) {
		ZOMER = true;
		l = S;
	}
	else if (W != -1){
		ZOMER = false;
		l = W;
	}
	struct timestampData timestampData_instance;
	timestampData_instance.zomeruur = ZOMER;
	

    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (isNumber(res, l))
            timestampData_instance.timestamp = atof(res);
    }
    return timestampData_instance;
}


bool decode_telegram(int len)
{	
	int startChar = FindCharInArrayRev(telegram, '/', len);		// eerste character van de eerste verzending uit de informatietrein
    int endChar = FindCharInArrayRev(telegram, '!', len);		// eerste character van de laatste verzending uit de informatietrein
    bool validCRCFound = false;

/* 	Serial.print("  startChar: ");
	Serial.println(startChar);
	Serial.print("  endChar: ");
	Serial.println(endChar);
	Serial.print("  print telegram per char: "); */

/*     for (int cnt = 0; cnt < len; cnt++) {
        Serial.print(telegram[cnt]);
    }
    Serial.print("\n"); */

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

void read_p1_hardwareserial()
{
	if (log_telegrams){
		memset(complete_telegram, 0, sizeof(complete_telegram));
		//total_len = 0;
	}
	if (Serial.available())
	{
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
    }
}

void processLine(int len) {
    //Serial.print("ProcessLine length: ");
	//Serial.println(len);
	
	telegram[len] = '\n';
    telegram[len + 1] = 0;
    yield();		// ook iets te maken met de hardware watchdog.

    bool result = decode_telegram(len + 1);
	// result is enkel TRUE wanneer alle lijnen van een volledig telegram goed binnengekomen zijn CRC_VALID

    if (result) {
        /* 
		if (LAST_GAS_METER_M3 > 0) {
           if (GAS_METER_M3 > LAST_GAS_METER_M3) {
               ACTUAL_CONSUMPTION_GAS_M3 = GAS_METER_M3 - LAST_GAS_METER_M3;
           }
        } else {
           ACTUAL_CONSUMPTION_GAS_M3 = 0;
        }
        LAST_GAS_METER_M3 = GAS_METER_M3;
        send_data_to_broker();
         */
		send_data();
		LAST_UPDATE_SENT = millis();
    }

}

// **********************************
// * EEPROM helpers                 *
// **********************************

String read_eeprom(int offset, int len)
{
    Serial.print(F("read_eeprom()"));

    String res = "";
    for (int i = 0; i < len; ++i)
    {
        res += char(EEPROM.read(i + offset));
    }
    return res;
}

void write_eeprom(int offset, int len, String value)
{
    Serial.println(F("write_eeprom()"));
    for (int i = 0; i < len; ++i)
    {
        if ((unsigned)i < value.length())
        {
            EEPROM.write(i + offset, value[i]);
        }
        else
        {
            EEPROM.write(i + offset, 0);
        }
    }
}

// ******************************************
// * Callback for saving WIFI config        *
// ******************************************

bool shouldSaveConfig = false;

// * Callback notifying us of the need to save config
void save_wifi_config_callback ()
{
    Serial.println(F("Should save config"));
    shouldSaveConfig = true;
}

// **********************************
// * Setup OTA                      *
// **********************************

void setup_ota()
{
    Serial.println(F("Arduino OTA activated."));

    // * Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // * Set hostname for OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]()
    {
        Serial.println(F("Arduino OTA: Start"));
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println(F("Arduino OTA: End (Running reboot)"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Arduino OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Arduino OTA: Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Arduino OTA: Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Arduino OTA: Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Arduino OTA: Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("Arduino OTA: End Failed"));
    });

    ArduinoOTA.begin();
    Serial.println(F("Arduino OTA finished"));
}

// **********************************
// * Setup MDNS discovery service   *
// **********************************

void setup_mdns()
{
    Serial.println(F("Starting MDNS responder service"));

    bool mdns_result = MDNS.begin(HOSTNAME);
    if (mdns_result)
    {
        MDNS.addService("http", "tcp", 80);
    }
}

// **********************************
// * Setup Main                     *
// **********************************

void setup()
{
    // * Configure EEPROM
    EEPROM.begin(512);

    // Setup a hw serial connection for communication with the P1 meter and logging (not using inversion)
    Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_FULL);
    Serial.println("");
    Serial.println("Swapping UART0 RX to inverted");
    Serial.flush();

    // Invert the RX serialport by setting a register value, this way the TX might continue normally allowing the serial monitor to read println's
	//// testing
	/////////USC0(UART0) = USC0(UART0) | BIT(UCRXI);
    Serial.println("Serial port is ready to recieve.");

    // * Set led pin as output
    pinMode(LED_BUILTIN, OUTPUT);

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    // * Get MQTT Server settings
    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1")
    {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);   // * 0-63
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);    // * 64-69
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);  // * 70-101
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32); // * 102-133
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port",     MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user",     MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass",     MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    // wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(save_wifi_config_callback);

    // * Add all your parameters here
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect())
    {
        Serial.println(F("Failed to connect to WIFI and hit timeout"));

        // * Reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(WIFI_TIMEOUT);
    }

    // * Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // * Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        Serial.println(F("Saving WiFiManager config"));

        write_eeprom(0, 64, MQTT_HOST);   // * 0-63
        write_eeprom(64, 6, MQTT_PORT);   // * 64-69
        write_eeprom(70, 32, MQTT_USER);  // * 70-101
        write_eeprom(102, 32, MQTT_PASS); // * 102-133
        write_eeprom(134, 1, "1");        // * 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    Serial.println(F("Connected to WIFI..."));

    // * Keep LED on
    ticker.detach();
    digitalWrite(LED_BUILTIN, LOW);

    // * Configure OTA
    setup_ota();

    // * Startup MDNS Service
    setup_mdns();

    // * Setup MQTT
    Serial.printf("MQTT connecting to: %s:%s\n", MQTT_HOST, MQTT_PORT);

    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));

}

// **********************************
// * Loop                           *
// **********************************

void loop()
{
    ArduinoOTA.handle();
    long now = millis();
    if (!mqtt_client.connected())
    {
        if (now - LAST_RECONNECT_ATTEMPT > 5000)
        {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect())
            {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    }
    else
    {
        mqtt_client.loop();
    }
    
    if (now - LAST_UPDATE_SENT > UPDATE_INTERVAL) {
		//Serial.println("loop start");
		//Serial.println(now);
		//Serial.println(LAST_UPDATE_SENT);

		read_p1_hardwareserial();
    }
}
