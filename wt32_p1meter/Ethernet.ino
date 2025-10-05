/**
 * This file holds functions related to Ethernet networking.
 */


#include "Ethernet.h"


/**
 * @brief This is a callback function to handle network events.
 * @param event a pre-defined event to process.
 * Even though the event type is WiFiEvent_t, the same events work with the ETH.h library.
 */
void NetworkEvent( WiFiEvent_t event )
{
	networkCallbackCount++;
	switch( event )
	{
		case ARDUINO_EVENT_ETH_START:
			Serial.println( "ETH Started" );
			ETH.setHostname( HOSTNAME );
			break;
		case ARDUINO_EVENT_ETH_CONNECTED:
			Serial.println( "ETH Connected" );
			break;
		case ARDUINO_EVENT_ETH_GOT_IP:
			snprintf( macAddress, 18, "%s", ETH.macAddress().c_str() );
			snprintf( ipAddress, 16, "%d.%d.%d.%d", ETH.localIP()[0], ETH.localIP()[1], ETH.localIP()[2], ETH.localIP()[3] );
			if( ETH.fullDuplex() )
				snprintf( duplex, 12, "%s", "FULL_DUPLEX" );
			else
				snprintf( duplex, 12, "%s", "HALF_DUPLEX" );
			linkSpeed = ETH.linkSpeed();
			Serial.printf( "ETH MAC: %s, IPv4: %s, %s, %u Mbps\n", macAddress, ipAddress, duplex, linkSpeed );
			eth_connected = true;
			break;
		case ARDUINO_EVENT_ETH_DISCONNECTED:
			Serial.println( "ETH Disconnected" );
			eth_connected = false;
			break;
		case ARDUINO_EVENT_ETH_STOP:
			Serial.println( "ETH Stopped" );
			eth_connected = false;
			break;
		default:
			break;
	}
}  // End of the NetworkEvent() callback function.


/**
 * @brief mqttCallback() will process incoming messages on subscribed topics.
 * { "command": "publishTelemetry" }
 * { "command": "changeTelemetryInterval", "value": 10000 }
 * ToDo: Add more commands for the board to react to.
 */
void mqttCallback( char *topic, byte *payload, unsigned int length )
{
	mqttCallbackCount++;
	Serial.printf( "\nMessage arrived on Topic: '%s'\n", topic );
	Serial.printf( "Callback count: '%lu'\n", mqttCallbackCount );
}  // End of the mqttCallback() function.


/**
 * @brief lookupMQTTCode() will return the string for an integer state code.
 */
void lookupMQTTCode( int code, char *buffer )
{
	switch( code )
	{
		case -4:
			snprintf( buffer, 29, "%s", "Connection timeout" );
			break;
		case -3:
			snprintf( buffer, 29, "%s", "Connection lost" );
			break;
		case -2:
			snprintf( buffer, 29, "%s", "Connect failed" );
			break;
		case -1:
			snprintf( buffer, 29, "%s", "Disconnected" );
			break;
		case 0:
			snprintf( buffer, 29, "%s", "Connected" );
			break;
		case 1:
			snprintf( buffer, 29, "%s", "Bad protocol" );
			break;
		case 2:
			snprintf( buffer, 29, "%s", "Bad client ID" );
			break;
		case 3:
			snprintf( buffer, 29, "%s", "Unavailable" );
			break;
		case 4:
			snprintf( buffer, 29, "%s", "Bad credentials" );
			break;
		case 5:
			snprintf( buffer, 29, "%s", "Unauthorized" );
			break;
		default:
			snprintf( buffer, 29, "%s", "Unknown MQTT state code" );
	}
}  // End of the lookupMQTTCode() function.


/**
 * @brief mqttConnect() will connect to the MQTT broker.
 */
void mqttConnect( const char *broker, const int port )
{
	long time = millis();
	// Connect the first time.  Avoid subtraction overflow.  Connect after cool down.
	if( lastMqttConnectionTime == 0 || ( time > mqttCoolDownInterval && ( time - mqttCoolDownInterval ) > lastMqttConnectionTime ) )
	{
		//mqttConnectCount++;
		//digitalWrite( RX_LED, LED_OFF );
		//digitalWrite( TX_LED, LED_OFF );
		Serial.printf( "Connecting to broker at %s:%d, using client ID '%s'...\n", broker, port, HOSTNAME );
		Serial.printf( "MQTT connection count: %lu\n", mqttConnectCount );
		mqttClient.setServer( broker, port );
		mqttClient.setBufferSize(1024);
		//mqttClient.setCallback( mqttCallback );

		// Connect to the broker, using the MAC address for a MQTT client ID.
		//if( mqttClient.connect( HOSTNAME, mqtt_user, mqtt_pass ) )
		if( mqttClient.connect( HOSTNAME ) )
		{
			Serial.println( "Connected to MQTT Broker." );
			//mqttClient.subscribe( COMMAND_TOPIC );
			//digitalWrite( RX_LED, LED_ON );
			//digitalWrite( TX_LED, LED_ON );
		}
		else
		{
			int mqttStateCode = mqttClient.state();
			char buffer[29];
			lookupMQTTCode( mqttStateCode, buffer );
			Serial.printf( "MQTT state: %s\n", buffer );
			Serial.printf( "MQTT state code: %d\n", mqttStateCode );

			// This block increments the broker connection "cooldown" time by 10 seconds after every failed connection, and resets it once it is over 2 minutes.
			if( mqttCoolDownInterval > 60000 )
				mqttCoolDownInterval = 0;
			mqttCoolDownInterval += 15000;
			Serial.printf( "MQTT cooldown interval: %lu\n", mqttCoolDownInterval );
			//toggleLED();
		}

		lastMqttConnectionTime = millis();
	}
}  // End of the mqttConnect() function.
