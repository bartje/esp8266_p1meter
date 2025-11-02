/**
 * This program will use the Ethernet port on a WT32-ETH01 devkit and connect to a Fluvius Smart meter.
 * It will publish data via MQTT
 */
#include "wt32_p1meter.h"
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <DateTimeFunctions.h>
#include <cmath>

#include <WebServer.h>
#include <ElegantOTA.h>

#include "arduino-dsmr-2/fields.h"
#include "arduino-dsmr-2/packet_accumulator.h"
#include "arduino-dsmr-2/parser.h"
#include <iostream>

using namespace arduino_dsmr_2;
using namespace fields;

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

// CRC check aanzetten in productie
PacketAccumulator accumulator(/* bufferSize */ P1_MAXLINELENGTH, /* check_crc */ true);

//	PacketAccumulator accumulator(/* bufferSize */ P1_MAXLINELENGTH, /* check_crc */ false);

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
	/* FixedValue */ voltage_l1,
	/* FixedValue */ voltage_l2,
	/* FixedValue */ voltage_l3,
	// /* FixedValue */ power_delivered_l1,
	// /* FixedValue */ power_delivered_l2,
	// /* FixedValue */ power_delivered_l3,
	// /* FixedValue */ power_returned_l1,
	// /* FixedValue */ power_returned_l2,
	// /* FixedValue */ power_returned_l3,
	/* FixedValue */ active_energy_import_current_average_demand,
	/*TimestampedFixedValue*/ active_energy_import_maximum_demand_running_month,
	/*TimestampedFixedValue*/ gas_delivered_be,
	/*TimestampedFixedValue*/ water_delivered>;


	// * Initiate WIFI client
//WiFiClient espClient;

// * Initiate MQTT client
//PubSubClient mqtt_client(espClient);

// epochtime krijgen
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


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

*/

float round_prec(double n, int prec)
{
    return std::round(n * pow(10, prec)) / pow(10, prec);
}

void send_data(JsonDocument jsonData, const char *topic, boolean retain = false){ // topic; payload
	
	String payload;
	serializeJson(jsonData, payload);
	if (!NO_NETWORK){
		mqttClient.publish(topic, payload.c_str(), retain);
	}
	if (ENABLE_LOG) {
		Serial.println("*****");
		Serial.print("sending to topic: ");
		Serial.println(topic);
		Serial.print("data: ");
		Serial.println(payload.c_str());
		Serial.println("*****");
	}
}


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

void read_p1_hardwareserial(){
	MyData data;
	//bool first = true;
	while (Serial2.available())	{
        // if(first){
		// 	Serial.print("start: ");
		// 	Serial.println(millis());
		// 	first=false;
		// }
		
		// First step is to get the full message from the P1 port.
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
			if(ENABLE_LOG){
				Serial.println(packet.data());
			}
			// Specify `check_crc` as false, since the accumulator already checked the CRC and didn't include it in the packet
			P1Parser::parse(&data, packet.data(), packet.size(), /* unknown_error */ false, /* check_crc */ false);
			//LAST_UPDATE_SENT = millis();
			
			timestampData elekTimestamp = getDate(data.timestamp);
			if (elekTimestamp.valid_data){
				counter +=1;
				P_tot += (data.power_delivered.val() - data.power_returned.val());
				VL1 += data.voltage_l1.val();
				VL2 += data.voltage_l2.val();
				VL3 += data.voltage_l3.val();
				IL1 += data.current_l1.val();
				IL2	+= data.current_l2.val();
				IL3 += data.current_l3.val();

				if(ENABLE_LOG){
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

					//Serial.printf("Power delivered L1: %.3f\n", static_cast<double>(data.power_delivered_l1.val()));
					//Serial.printf("Power returned L1: %.3f\n", static_cast<double>(data.power_returned_l1.val()));
					//Serial.printf("Power delivered L2: %.3f\n", static_cast<double>(data.power_delivered_l2.val()));
					//Serial.printf("Power returned L3: %.3f\n", static_cast<double>(data.power_returned_l2.val()));
					//Serial.printf("Power delivered L3: %.3f\n", static_cast<double>(data.power_delivered_l3.val()));
					//Serial.printf("Power returned L3: %.3f\n", static_cast<double>(data.power_returned_l3.val()));

					Serial.printf("Current L1: %.3f\n", static_cast<double>(data.current_l1.val()));
					Serial.printf("Current L2: %.3f\n", static_cast<double>(data.current_l2.val()));
					Serial.printf("Current L3: %.3f\n", static_cast<double>(data.current_l3.val()));

					// TimestampedFixedValue
					Serial.printf("Gasverbruik: %.3f\n", static_cast<double>(data.gas_delivered_be.val()));
					Serial.printf("Gas timestamp: %s\n", data.gas_delivered_be.timestamp.c_str());
					Serial.printf("Waterverbruik: %.3f\n", static_cast<double>(data.water_delivered.val()));
					Serial.printf("Water timestamp: %s\n", data.water_delivered.timestamp.c_str());
				}
				
			}
			if (elekTimestamp.valid_data && elekTimestamp.epochTimestamp % UPDATE_INTERVAL == 0){
				timestampData gasTimestamp = getDate(data.gas_delivered_be.timestamp);
				timestampData waterTimestamp = getDate(data.water_delivered.timestamp);
				timestampData elekPeakMonthTimestamp = getDate(data.active_energy_import_maximum_demand_running_month.timestamp);
				
				// is niet meer nodig, want al gedaan ook voor de 5-vouden
				// counter +=1;
				// P_tot += (data.power_delivered.val() - data.power_returned.val());
				// VL1 += data.voltage_l1.val();
				// VL2 += data.voltage_l2.val();
				// VL3 += data.voltage_l3.val();
				// IL1 += data.current_l1.val();
				// IL2	+= data.current_l2.val();
				// IL3 += data.current_l3.val();
				// Serial.print("end: ");
				// Serial.println(millis());
				
				//--> maak nu boodschappen voor mqtt.  opgelet inhoud controleren alvorens te versturen.
				// elke 5 seconden
				// power/home/energy (gemiddelde waarden)
				//				{"time": 1759563805, "E_tot_pos": 18433.6452, "E_tot_neg": 15574.2192, "E_tot": 2859.426, 
				//					"P_tot_pos": 214.2, "P_tot_neg": 0.0, "P_tot": 214.2, "Cosphi": 0.48, "VL1L3": 231.8, "VL2L3": 233.5}
				//power/home/energy/instant (instantane waarde per 5 seconden)
				//				{"time": 1748202861, "P_tot_inst": 336.4}

				
				JsonDocument elek;
				JsonDocument elekInst;
				JsonDocument gas;
				JsonDocument water;
				// serializeJson(elek, Serial);

				elek["time"] = elekTimestamp.epochTimestamp;
				// energy is the latest counter
				elek["E_tot_pos"] = data.energy_delivered_tariff1.val()+ data.energy_delivered_tariff2.val();
				elek["E_tot_neg"] = data.energy_returned_tariff1.val() + data.energy_returned_tariff2.val();
				elek["E_tot"] = (data.energy_delivered_tariff1.val()+ data.energy_delivered_tariff2.val()) - (data.energy_returned_tariff1.val() + data.energy_returned_tariff2.val());
				
				// power delivered and power returned kan allebij positief zijn want verschillende fasen
				
				float P_tot_pos = 0;
				float P_tot_neg = 0;
				if(P_tot > 0){
					P_tot_pos = abs(P_tot);
				} else {
					P_tot_neg = abs(P_tot);
				}
				elek["P_tot_pos"] = 1000 * round_prec(P_tot_pos/counter,3);
				elek["P_tot_neg"] = 1000 * round_prec(P_tot_neg/counter,3);
				elek["P_tot"] = 1000 * round_prec(P_tot/counter,3);
				
				elek["VL1"] = round_prec(VL1/counter,1);
				elek["VL2"] = round_prec(VL2/counter,1);
				elek["VL3"] = round_prec(VL3/counter,1);
				elek["IL1"] = round_prec(IL1/counter,2);
				elek["IL2"] = round_prec(IL2/counter,2);
				elek["IL3"] = round_prec(IL3/counter,2);
				
				elek["currentPeak"] = data.active_energy_import_current_average_demand.val();
				if(elekPeakMonthTimestamp.valid_data){
					JsonObject capac = elek["capac"].to<JsonObject>();
					capac["timeMonthPeak"] = elekPeakMonthTimestamp.epochTimestamp;
					capac["monthPeak"] = data.active_energy_import_maximum_demand_running_month.val();
				}
				send_data(elek,mqtt_topic_elek,false);
				
				//reset all tempvalues
				counter = 0;
				P_tot = 0;
				VL1 = 0;
				VL2 = 0;
				VL3 = 0;
				IL1 = 0;
				IL2	= 0;
				IL3 = 0;

				float P_tot_inst = data.power_delivered.val() - data.power_returned.val();
				elekInst["time"] = elekTimestamp.epochTimestamp;
				elekInst["P_tot_inst"] = 1000 * P_tot_inst;
				send_data(elekInst,mqtt_topic_elek_inst,false);
				
				if (gasTimestamp.valid_data){
					gas["time"] = gasTimestamp.epochTimestamp;
					gas["gas"] = data.gas_delivered_be.val();
					gas["E_gas"] = round_prec(data.gas_delivered_be.val() * factor_gas,3);
					send_data(gas,mqtt_topic_gas,false);
				}
				if (waterTimestamp.valid_data){
					water["time"] = waterTimestamp.epochTimestamp;
					water["water"] = data.water_delivered.val();
					send_data(water,mqtt_topic_water,false);
				}


			}
			
		}
    }
}


void publishDiscovery(String sensor_name, String device_class, String state_class, String unit, String icon, String mqtt_topic){
	
	String unique_id = "P1_reader_device_1_" + sensor_name;
	JsonDocument discover;
	discover["name"] = sensor_name;
	discover["state_topic"] = mqtt_topic;
	discover["unit_of_measurement"] = unit;
	discover["icon"] = icon;
	discover["device_class"] = device_class;
	discover["state_class"] = state_class;
	discover["unique_id"] = unique_id;
	discover["value_template"] = "{{ value_json." + sensor_name + " | float }}";
	
	
	//JsonObject device = discover.createNestedObject("device");
	JsonObject device = discover["device"].to<JsonObject>();
	device["identifiers"] = "P1_reader_device_1";
	device["name"] = "P1 reader";
	device["model"] = "P1 to WT32";
	device["manufacturer"] = "Witje@Fluvius";
	
	String mqtt_discoverHA = "homeassistant/sensor/" + unique_id + "/config";
	
	// retain op true zetten in productie
	if (PRODUCTION)
		send_data(discover,mqtt_discoverHA.c_str(), true);
	else
		send_data(discover,mqtt_discoverHA.c_str(), false);
}

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
    Serial.println("Swapping UART0 RX an TX to inverted");
    Serial.flush();

    Serial.println("Serial2 port is ready to recieve.");


	// declaring all sensors to HomeAssistant
	/*
	
	*/
	// void publishDiscovery(String sensor_name, String device_class, String state_class, String unit, String icon, String mqtt_topic)
	
	// Elektricity
	publishDiscovery("E_tot_pos", "energy", "total_increasing", "kWh", "mdi:transmission-tower-export", mqtt_topic_elek);
	publishDiscovery("E_tot_neg", "energy", "total_increasing", "kWh", "mdi:transmission-tower-import", mqtt_topic_elek);
	publishDiscovery("E_tot", "energy", "total", "kWh", "mdi:transmission-tower", mqtt_topic_elek);
	publishDiscovery("P_tot_pos", "power", "measurement", "W", "mdi:transmission-tower-export", mqtt_topic_elek);
	publishDiscovery("P_tot_pos", "power", "measurement", "W", "mdi:transmission-tower-import", mqtt_topic_elek);
	publishDiscovery("P_tot", "power", "measurement", "W", "mdi:transmission-tower", mqtt_topic_elek);
	publishDiscovery("P_tot_inst", "power", "measurement", "W", "mdi:transmission-tower", mqtt_topic_elek_inst);
	publishDiscovery("VL1", "voltage", "measurement", "V", "mdi:sine-wave", mqtt_topic_elek);
	publishDiscovery("VL2", "voltage", "measurement", "V", "mdi:sine-wave", mqtt_topic_elek);
	publishDiscovery("VL3", "voltage", "measurement", "V", "mdi:sine-wave", mqtt_topic_elek);
	publishDiscovery("IL1", "current", "measurement", "A", "mdi:current-ac", mqtt_topic_elek);
	publishDiscovery("IL2", "current", "measurement", "A", "mdi:current-ac", mqtt_topic_elek);
	publishDiscovery("IL3", "current", "measurement", "A", "mdi:current-ac", mqtt_topic_elek);
	publishDiscovery("currentPeak", "power", "measurement", "kW", "mdi:transmission-tower", mqtt_topic_elek);
	
	// Gas
	publishDiscovery("gas", "gas", "total_increasing", "m³", "mdi:gas-cylinder", mqtt_topic_gas);
	publishDiscovery("E_gas", "energy", "total_increasing", "kWh", "mdi:gas-burner", mqtt_topic_gas);

	// Water
	publishDiscovery("water", "water", "total_increasing", "m³", "mdi:water", mqtt_topic_water);
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
    
	read_p1_hardwareserial();
	
	
	// lees serial2 en schrijf naar Serial
	/*
	long now = millis();
	if (now - LAST_UPDATE_SENT > UPDATE_INTERVAL) {
		read_p1_hardwareserial();
	}
	*/

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