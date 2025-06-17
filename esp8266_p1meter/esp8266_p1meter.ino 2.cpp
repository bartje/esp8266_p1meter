# 1 "/var/folders/r9/63bqj8pn573gphb2b_l_yd_h0000gn/T/tmpebmou3q5"
#include <Arduino.h>
# 1 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
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
#include <DateTimeFunctions.h>
#include <ArduinoJson.h>


#include "settings.h"


Ticker ticker;


DateTimeFunctions dTF;


WiFiClient espClient;


PubSubClient mqtt_client(espClient);
void configModeCallback(WiFiManager *myWiFiManager);
void tick();
void send_mqtt_message(const char *topic, char *payload);
bool mqtt_reconnect();
void send_metric(String name, long metric);
void send_data_to_broker();
void send_data();
long epochUTC(long long date, bool summerTime);
unsigned int CRC16(unsigned int crc, unsigned char *buf, int len);
bool isNumber(char *res, int len);
int FindCharInArrayRev(char array[], char c, int len);
int FindCharInArray(char array[], char c, int len);
long getValue(char *buffer, int maxlen, char startchar, char endchar);
struct timestampData getDate(char *buffer, int maxlen, char startchar, char endchar);
struct timedValue getTimedValue(char *buffer, int maxlen, char firststartchar, char firstendchar, char laststartchar, char lastendchar);
bool decode_telegram(int len);
void read_p1_hardwareserial();
void processLine(int len);
String read_eeprom(int offset, int len);
void write_eeprom(int offset, int len, String value);
void save_wifi_config_callback ();
void setup_ota();
void setup_mdns();
void setup();
void loop();
#line 34 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());


    Serial.println(myWiFiManager->getConfigPortalSSID());


    ticker.attach(0.2, tick);
}






void tick()
{

    int state = digitalRead(LED_BUILTIN);
    digitalWrite(LED_BUILTIN, !state);
}






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


bool mqtt_reconnect()
{

    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES)
    {
        MQTT_RECONNECT_RETRIES++;
        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);


        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
        {
            Serial.println(F("MQTT connected!"));


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



}

void send_data_to_broker()
{
    if (FLIPHIGHLOWTARIF) {
        send_metric("consumption_low_tarif", CONSUMPTION_HIGH_TARIF.value);
        send_metric("consumption_high_tarif", CONSUMPTION_LOW_TARIF.value);
        send_metric("returndelivery_low_tarif", RETURNDELIVERY_HIGH_TARIF.value);
        send_metric("returndelivery_high_tarif", RETURNDELIVERY_LOW_TARIF.value);
        if (ACTUAL_TARIF.value == 1)
        {
            ACTUAL_TARIF.value = 2;
        } else {
            ACTUAL_TARIF.value = 1;
        }
    } else {
        send_metric("consumption_low_tarif", CONSUMPTION_LOW_TARIF.value);
        send_metric("consumption_high_tarif", CONSUMPTION_HIGH_TARIF.value);
        send_metric("returndelivery_low_tarif", RETURNDELIVERY_LOW_TARIF.value);
        send_metric("returndelivery_high_tarif", RETURNDELIVERY_HIGH_TARIF.value);
    }

    send_metric("actual_consumption", ACTUAL_CONSUMPTION.value);
    send_metric("actual_returndelivery", ACTUAL_RETURNDELIVERY.value);
 send_metric("timestamp", epochUTC(TIMESTAMP.timestamp, TIMESTAMP.zomeruur));
# 169 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
 send_metric("gas_meter_m3 timestamp", epochUTC(GAS_METER_M3.timestamp.timestamp, GAS_METER_M3.timestamp.zomeruur));
    send_metric("gas_meter_m3 waarde", GAS_METER_M3.value);


    send_metric("actual_tarif_group", ACTUAL_TARIF.value);




}

void send_data(){
# 206 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
 float P_tot = L1_INSTANT_POWER_USAGE.value + L2_INSTANT_POWER_USAGE.value + L3_INSTANT_POWER_USAGE.value - L1_INSTANT_POWER_PRODUCTION.value - L2_INSTANT_POWER_PRODUCTION.value - L3_INSTANT_POWER_PRODUCTION.value;
 float P_tot_pos = 0;
 float P_tot_neg = 0;
 if(P_tot > 0){
  P_tot_pos = abs(P_tot);
 } else {
  P_tot_neg = abs(P_tot);
 }

 JsonDocument energy;
 energy["time"] = epochUTC(TIMESTAMP.timestamp, TIMESTAMP.zomeruur);
 energy["E_tot_pos"] = CONSUMPTION_LOW_TARIF.value + CONSUMPTION_HIGH_TARIF.value;
 energy["E_tot_neg"] = RETURNDELIVERY_LOW_TARIF.value + RETURNDELIVERY_HIGH_TARIF.value;
 energy["E_tot"] = CONSUMPTION_LOW_TARIF.value + CONSUMPTION_HIGH_TARIF.value - (RETURNDELIVERY_LOW_TARIF.value + RETURNDELIVERY_HIGH_TARIF.value);
 energy["P_tot_pos"] = P_tot_pos;
 energy["P_tot_neg"] = P_tot_neg;
 energy["P_tot"] = P_tot;
 energy["VL1"] = L1_VOLTAGE.value;
 energy["VL2"] = L2_VOLTAGE.value;
 energy["VL3"] = L3_VOLTAGE.value;

}


long epochUTC(long long date, bool summerTime) {







 char buffer[13];
 lltoa(date,buffer,13,10);
 char YY[3] = {0};
   memcpy(&YY, &buffer[0], sizeof(YY)-1);
 char MM[3] = {0};
   memcpy(&MM, &buffer[2], sizeof(MM)-1);
 char DD[3] = {0};
   memcpy(&DD, &buffer[4], sizeof(DD)-1);
 char hh[3] = {0};
   memcpy(&hh, &buffer[6], sizeof(hh)-1);
 char mm[3] = {0};
   memcpy(&mm, &buffer[8], sizeof(mm)-1);
 char ss[3] = {0};
   memcpy(&ss, &buffer[10], sizeof(ss)-1);

 uint32_t epochtime = 0;
 epochtime = dTF.conDT2UT(atoi(DD),atoi(MM),2000 + atoi(YY),atoi(hh),atoi(mm),atoi(ss));
 int delta;
 if(summerTime){
  delta = 2*60*60;
 } else {
  delta = 1*60*60;
 }

 return long(epochtime-delta);
}





unsigned int CRC16(unsigned int crc, unsigned char *buf, int len)
{
 for (int pos = 0; pos < len; pos++)
    {
  crc ^= (unsigned int)buf[pos];

        for (int i = 8; i != 0; i--)
        {

            if ((crc & 0x0001) != 0)
            {

                crc >>= 1;
    crc ^= 0xA001;
   }

            else

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
    for (int i = 0; i <= len - 1; i++)
    {
        if (array[i] == c)
            return i;
    }
    return -1;
}

long getValue(char *buffer, int maxlen, char startchar, char endchar)
{


 int s = FindCharInArrayRev(buffer, startchar, maxlen - 1);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 1) - s - 1;



    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (endchar == '*')
        {
            if (isNumber(res, l))

                return (1000 * atof(res));
        }
        else if (endchar == ')')
        {
            if (isNumber(res, l))
                return atof(res);
        }
    }
    return 0;
}

struct timestampData getDate(char *buffer, int maxlen, char startchar, char endchar)
{



 int s = FindCharInArrayRev(buffer, startchar, maxlen - 1);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 1) - s - 1;
 int S = FindCharInArrayRev(buffer, 'S', maxlen - 1);
 int W = FindCharInArrayRev(buffer, 'W', maxlen - 1);




 bool ZOMER_UUR = false;
 if (S != -1) {
  ZOMER_UUR = true;
 }
 else if (W != -1){
  ZOMER_UUR = false;
 }
 struct timestampData timestampData_instance;
 timestampData_instance.zomeruur = ZOMER_UUR;


    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l-1))
    {

        if (isNumber(res, l))
   timestampData_instance.timestamp = atoll(res);
    }
    return timestampData_instance;
}

struct timedValue getTimedValue(char *buffer, int maxlen, char firststartchar, char firstendchar, char laststartchar, char lastendchar){



 int fs = FindCharInArray(buffer, firststartchar, maxlen - 1);
    int fl = FindCharInArray(buffer, firstendchar, maxlen - 1) - fs + 1;
 int ls = FindCharInArrayRev(buffer, laststartchar, maxlen - 1);
    int ll = FindCharInArrayRev(buffer, lastendchar, maxlen - 1) - ls + 1;




 struct timedValue timedValue_instance;
    char res[16];
    memset(res, 0, sizeof(res));



 if (strncpy(res, buffer + fs, fl)){




  timedValue_instance.timestamp = getDate(res, sizeof(res),firststartchar,firstendchar);
    }

 memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + ls, ll)){

        timedValue_instance.value = getValue(res, sizeof(res),laststartchar,lastendchar);
# 431 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    }
    return timedValue_instance;
}

bool decode_telegram(int len)
{


 int startChar = FindCharInArrayRev(telegram, '/', len);
    int endChar = FindCharInArrayRev(telegram, '!', len);
    bool validCRCFound = false;
# 454 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (startChar >= 0)
    {

        currentCRC = CRC16(0x0000,(unsigned char *) telegram+startChar, len-startChar);


    }
    else if (endChar >= 0)
    {

        currentCRC = CRC16(currentCRC,(unsigned char*)telegram+endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0;
        validCRCFound = (strtol(messageCRC, NULL, 16) == long(currentCRC));




        if (validCRCFound){
            Serial.print(currentCRC);
   Serial.println(F(" CRC Valid!"));
  }
        else{
            Serial.print(currentCRC);
   Serial.println(F(" CRC Invalid!"));
  }

        currentCRC = 0;
    }
    else
    {
        currentCRC = CRC16(currentCRC, (unsigned char*) telegram, len);


    }
# 500 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "0-0:1.0.0", strlen("0-0:1.0.0")) == 0)
    {
  TIMESTAMP = getDate(telegram, len, '(', ')');

    }
# 513 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    {

  CONSUMPTION_LOW_TARIF.value = getValue(telegram, len, '(', '*');


    }



    if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    {
        CONSUMPTION_HIGH_TARIF.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    {
        RETURNDELIVERY_LOW_TARIF.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    {
        RETURNDELIVERY_HIGH_TARIF.value = getValue(telegram, len, '(', '*');
    }







    if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
    {
        GAS_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');


    }




    if (strncmp(telegram, "0-1:24.2.3", strlen("0-1:24.2.3")) == 0)
    {
        GAS_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');
    }
# 571 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "0-2:24.2.1", strlen("0-2:24.2.1")) == 0)
    {
        WATER_METER_M3 = getTimedValue(telegram, len, '(', ')', '(', '*');
 }
# 584 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    {
        ACTUAL_CONSUMPTION.value = getValue(telegram, len, '(', '*');
    }


    if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    {
        ACTUAL_RETURNDELIVERY.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0)
    {
        L1_INSTANT_POWER_USAGE.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0)
    {
        L2_INSTANT_POWER_USAGE.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:61.7.0", strlen("1-0:61.7.0")) == 0)
    {
        L3_INSTANT_POWER_USAGE.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:22.7.0", strlen("1-0:22.7.0")) == 0)
    {
        L1_INSTANT_POWER_PRODUCTION.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:42.7.0", strlen("1-0:42.7.0")) == 0)
    {
        L2_INSTANT_POWER_PRODUCTION.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:62.7.0", strlen("1-0:62.7.0")) == 0)
    {
        L3_INSTANT_POWER_PRODUCTION.value = getValue(telegram, len, '(', '*');
    }
# 644 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0)
    {
        L1_INSTANT_POWER_CURRENT.value = getValue(telegram, len, '(', '*');
    }


    if (strncmp(telegram, "1-0:51.7.0", strlen("1-0:51.7.0")) == 0)
    {
        L2_INSTANT_POWER_CURRENT.value = getValue(telegram, len, '(', '*');
    }


    if (strncmp(telegram, "1-0:71.7.0", strlen("1-0:71.7.0")) == 0)
    {
        L3_INSTANT_POWER_CURRENT.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:32.7.0", strlen("1-0:32.7.0")) == 0)
    {
        L1_VOLTAGE.value = getValue(telegram, len, '(', '*');
    }


    if (strncmp(telegram, "1-0:52.7.0", strlen("1-0:52.7.0")) == 0)
    {
        L2_VOLTAGE.value = getValue(telegram, len, '(', '*');
    }


    if (strncmp(telegram, "1-0:72.7.0", strlen("1-0:72.7.0")) == 0)
    {
        L3_VOLTAGE.value = getValue(telegram, len, '(', '*');
    }
# 688 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
    {
        ACTUAL_TARIF.value = getValue(telegram, len, '(', ')');
    }



    if (strncmp(telegram, "0-0:96.7.21", strlen("0-0:96.7.21")) == 0)
    {
        SHORT_POWER_OUTAGES.value = getValue(telegram, len, '(', ')');
    }



    if (strncmp(telegram, "0-0:96.7.9", strlen("0-0:96.7.9")) == 0)
    {
        LONG_POWER_OUTAGES.value = getValue(telegram, len, '(', ')');
    }



    if (strncmp(telegram, "1-0:32.32.0", strlen("1-0:32.32.0")) == 0)
    {
        SHORT_POWER_DROPS.value = getValue(telegram, len, '(', ')');
    }



    if (strncmp(telegram, "1-0:32.36.0", strlen("1-0:32.36.0")) == 0)
    {
        SHORT_POWER_PEAKS.value = getValue(telegram, len, '(', ')');
    }
# 728 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
    if (strncmp(telegram, "1-0:1.4.0", strlen("1-0:1.4.0")) == 0)
    {
        QUARTER_VALUE.value = getValue(telegram, len, '(', '*');
    }



    if (strncmp(telegram, "1-0:1.6.0", strlen("1-0:1.6.0")) == 0)
    {
        QUARTER_PEAK_CURRENT_MONTH = getTimedValue(telegram, len, '(', ')','(','*');
    }


    return validCRCFound;

}

void read_p1_hardwareserial()
{
    if (Serial.available())
 {


  memset(telegram, 0, sizeof(telegram));



  int counter = 1;

  while (Serial.available())
        {


   counter++;
   tick();

   ESP.wdtDisable();
            int len = Serial.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);
            ESP.wdtEnable(1);






   processLine(len);
        }
    }
}

void processLine(int len) {



 telegram[len] = '\n';
    telegram[len + 1] = 0;
    yield();

    bool result = decode_telegram(len + 1);



    if (result) {
# 799 "/Users/bartje/Documents/Knutsel en co/github/esp8266_p1meter/esp8266_p1meter/esp8266_p1meter.ino"
        send_data_to_broker();
        LAST_UPDATE_SENT = millis();
    }

}





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





bool shouldSaveConfig = false;


void save_wifi_config_callback ()
{
    Serial.println(F("Should save config"));
    shouldSaveConfig = true;
}





void setup_ota()
{
    Serial.println(F("Arduino OTA activated."));


    ArduinoOTA.setPort(8266);


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





void setup_mdns()
{
    Serial.println(F("Starting MDNS responder service"));

    bool mdns_result = MDNS.begin(HOSTNAME);
    if (mdns_result)
    {
        MDNS.addService("http", "tcp", 80);
    }
}





void setup()
{

    EEPROM.begin(512);


    Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_FULL);
    Serial.println("");
    Serial.println("Swapping UART0 RX to inverted");
    Serial.flush();



    Serial.println("Serial port is ready to recieve.");


    pinMode(LED_BUILTIN, OUTPUT);


    ticker.attach(0.6, tick);


    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1")
    {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32);
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port", MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user", MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass", MQTT_PASS, 32);


    WiFiManager wifiManager;





    wifiManager.setAPCallback(configModeCallback);


    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);


    wifiManager.setSaveConfigCallback(save_wifi_config_callback);


    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);



    if (!wifiManager.autoConnect())
    {
        Serial.println(F("Failed to connect to WIFI and hit timeout"));


        ESP.reset();
        delay(WIFI_TIMEOUT);
    }


    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());


    if (shouldSaveConfig)
    {
        Serial.println(F("Saving WiFiManager config"));

        write_eeprom(0, 64, MQTT_HOST);
        write_eeprom(64, 6, MQTT_PORT);
        write_eeprom(70, 32, MQTT_USER);
        write_eeprom(102, 32, MQTT_PASS);
        write_eeprom(134, 1, "1");
        EEPROM.commit();
    }


    Serial.println(F("Connected to WIFI..."));


    ticker.detach();
    digitalWrite(LED_BUILTIN, LOW);


    setup_ota();


    setup_mdns();


    Serial.printf("MQTT connecting to: %s:%s\n", MQTT_HOST, MQTT_PORT);

    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));

}





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




  read_p1_hardwareserial();
    }
}