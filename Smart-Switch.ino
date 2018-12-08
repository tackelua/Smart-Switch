// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       Contactor3.ino
	Created:	12/6/2018 9:33:17 AM
	Author:     GITH\tacke

	To Do
	- Add Auto off when get timeout
*/


#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <Time.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <Button.h>
#include <FS.h>

#define VERSION     "0.1.7"


#define INPUT1       12
#define INPUT2       13
#define INPUT3       14
#define OUTPUT1      4
#define OUTPUT2      5
#define OUTPUT3      16
#define BUTTON       0

#define TERMINAL     V0
#define VPIN1        V1
#define VPIN2        V2
#define VPIN3        V3
#define VTIMEOUT1    V4
#define VTIMEOUT2    V5
#define VTIMEOUT3    V6
#define VMODESTART   V7
#define VWIFI        V8

String HARDWARE_ID = "GITH-SW";

const char* file_blynk_token = "/blynk.token";

String blynkToken = "d8297a5187c14fea8b04e92d8e4c2292";
String blynkServer = "gith.cf";
int blynkPort = 5222;//8442;
bool BlynkConnected = false;

WidgetTerminal Terminal(TERMINAL);
WidgetRTC rtc;

const char* mqtt_server = "mic.duytan.edu.vn";
const char* mqtt_user = "Mic@DTU2017";
const char* mqtt_password = "Mic@DTU2017!@#";
const uint16_t mqtt_port = 1883;
WiFiClient mqtt_espClient;
PubSubClient mqtt_client(mqtt_espClient);

String tp_req = HARDWARE_ID + "/req";
String tp_res = HARDWARE_ID + "/res";
String tp_stt = HARDWARE_ID + "/stt";

enum VMODESTART_e {
	VMODE_OFF = 1,
	VMODE_BUTTON,
	VMODE_APP
};
VMODESTART_e STARTUP_MODE = VMODE_OFF;
bool STARTUP_MODE_synced = false;
bool BLYNK_connected = false;
String STARTUP_MODE_toString() {

	String m;
	switch (STARTUP_MODE)
	{
	case VMODE_OFF:
		m = "VMODE_OFF";
		break;
	case VMODE_BUTTON:
		m = "VMODE_BUTTON";
		break;
	case VMODE_APP:
		m = "VMODE_APP";
		break;
	default:
		m = "UNKNOWN";
		break;
	}
	return m;
}

#define Sprint		Serial.print
#define Sprintln	Serial.println
#define Sprintf		Serial.printf
#define Sflush		Serial.flush
#define Stime		Stime
void Stime() {
	Sprint(F("["));
	Sprint(getTimeStr());
	Sprint(F("] "));
	Sflush();
}

#define Tprint		Terminal.print
#define Tprintln	Terminal.println
#define Tprintf		Terminal.printf
#define Tflush		Terminal.flush
void Ttime() {
	String s;
	s += (F("["));
	s += getTimeStr();
	s += (F("] "));
	Tprint(s);
	Tflush();
}
String s_time() {
	String s;
	s += (F("["));
	s += getTimeStr();
	s += (F("] "));
	return s;
}

#define Dprint(x)		{ LED_ON(); Sprint(x); Sflush(); Tprint(x); Tflush(); LED_OFF(); }
#define Dprintln(x)		{ LED_ON(); Sprintln(x); Sflush(); Tprintln(x); Tflush(); mqtt_publish(tp_res, x, false); LED_OFF(); }
#define Dprintf(...)	{ LED_ON(); Sprintf(__VA_ARGS__); Sflush(); Tprintf(__VA_ARGS__); Tflush(); LED_OFF();}
#define Dtime()			{ Stime(); Ttime(); }

//Button(uint8_t pin, uint8_t puEnable, uint8_t invert, uint32_t dbTime);
bool puEnable = false;
bool invert = true;
long dbTime = 25;
Button button(BUTTON, puEnable, invert, dbTime);
Button Button1(INPUT1, puEnable, invert, dbTime);
Button Button2(INPUT2, puEnable, invert, dbTime);
Button Button3(INPUT3, puEnable, invert, dbTime);

class SwitchClass {
private:
	int _pin;
public:
	SwitchClass(int pin) {
		_pin = pin;
		pinMode(_pin, OUTPUT);
	}
	bool status;
	int timeout = 0;
	unsigned long last_changed = 0;
	void turn(bool stt) {
		last_changed = millis();
		status = stt;
		digitalWrite(_pin, status);

		switch (_pin)
		{
		case OUTPUT1:
			Blynk.virtualWrite(VPIN1, status);
			break;
		case OUTPUT2:
			Blynk.virtualWrite(VPIN2, status);
			break;
		case OUTPUT3:
			Blynk.virtualWrite(VPIN3, status);
			break;
		default:
			break;
		}
	}
	void toggle() {
		turn(!status);
	}
};

SwitchClass Switch1(OUTPUT1);
SwitchClass Switch2(OUTPUT2);
SwitchClass Switch3(OUTPUT3);


String getTimeStr() {
	String strTime;
	strTime = hour() < 10 ? String("0") + hour() : hour();
	strTime += ":";
	strTime += minute() < 10 ? String("0") + minute() : minute();
	strTime += ":";
	strTime += second() < 10 ? String("0") + second() : second();
	return strTime;
}

void hardware_init() {
	pinMode(INPUT1, INPUT);
	pinMode(INPUT2, INPUT);
	pinMode(INPUT3, INPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
}

void LED_ON() {
	digitalWrite(LED_BUILTIN, LOW);
}
void LED_OFF() {
	digitalWrite(LED_BUILTIN, HIGH);
}

void wifi_manager(bool force_create_AP = false) {
	Sprintln(F("WiFi Manager begin"));
	LED_ON();
	Sprintln("Create AP: " + HARDWARE_ID);

	WiFiManager wifiManager;
	wifiManager.setTimeout(120);
	if (force_create_AP) {
		wifiManager.startConfigPortal(HARDWARE_ID.c_str());
	}
	else {
		wifiManager.autoConnect(HARDWARE_ID.c_str());
	}

	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		LED_OFF();
		Sprintln(F("\nconnected"));
		Sprintln(WiFi.localIP());
	}

	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);
	delay(1); yield();
}

void create_AP_portal() {
	wifi_manager(true);
}
void handle_button() {
	button.read();
	Button1.read();
	Button2.read();
	Button3.read();

	if (Button1.wasPressed() || Button1.wasReleased()) {
		Switch1.toggle();
		String d = s_time() + "HUB | OUTPUT1 = " + String(Switch1.status);
		Dprintln(d);
		delay(1); yield();
	}

	if (Button2.wasPressed() || Button2.wasReleased()) {
		Switch2.toggle();
		String d = s_time() + "HUB | OUTPUT2 = " + String(Switch2.status);
		Dprintln(d);
		delay(1); yield();
	}

	if (Button3.wasPressed() || Button3.wasReleased()) {
		Switch3.toggle();
		String d = s_time() + "HUB | OUTPUT3 = " + String(Switch3.status);
		Dprintln(d);
		delay(1); yield();
	}

	if (button.pressedFor(5000)) {
		Dprintln("Created AP");
		create_AP_portal();
	}
	delay(1); yield();
}

void restart(bool TerminalDebug) {
	Sprintln("ESP Restart"); Sflush();
	if (TerminalDebug) {
		Tprintln("ESP Restart"); Tflush();
	}
	delay(10);
	ESP.restart();
	delay(1000);
}
void WiFi_init() {
	LED_ON();//on
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);

	WiFi.printDiag(Serial);

	Sprintln(F("\nWiFi_init"));

	WiFi.reconnect();
	//wifi_manager();
	if (WiFi.waitForConnectResult() != WL_CONNECTED) {
		//smart_config();
		wifi_manager();
		if (!WiFi.isConnected()) {
			restart(false);
		}
	}

	LED_OFF();
	Sprint(F("IP: "));
	Sprintln(WiFi.localIP());
}

int wifi_quality() {
	int32_t dBm = WiFi.RSSI();
	if (dBm <= -100)
		return 0;
	else if (dBm >= -50)
		return 100;
	else
		return int(2 * (dBm + 100));
}
void load_blynk_token() {
	SPIFFS.begin();
	File  blynk_token;
	blynk_token = SPIFFS.open(file_blynk_token, "r");
	if (blynk_token) {
		String _token;
		if (blynk_token.available()) {
			_token = blynk_token.readString();
		}
		_token.trim();
		Sprint("\r\nBlynk Token : ");
		Sprintln(_token);
		blynkToken = _token;
	}
	else {
		Sprintln("\r\nBlynk Token not available");
	}
	blynk_token.close();
}
void save_blynk_token() {
	Sprint("\r\nSave Blynk Token : ");
	Sprintln(blynkToken);
	File  blynk_token;
	blynk_token = SPIFFS.open(file_blynk_token, "w");
	if (blynk_token) {
		blynk_token.print(blynkToken);
		blynk_token.close();
		Sprintln("Done");
	}
	else {
		Sprintln(F("Failed to open file for writing"));
	}
}

void Blynk_init() {
	Sprintln(F("\r\nBlynk Init"));
	Sprintln("Server:\t" + blynkServer);
	Sprintln("Port:\t" + String(blynkPort));
	Sprintln("Token:\t" + blynkToken);

	static IPAddress ipServer;
	if (ipServer.fromString(blynkServer)) {
		Blynk.config(blynkToken.c_str(), ipServer, blynkPort);
	}
	else {
		Blynk.config(blynkToken.c_str(), blynkServer.c_str(), blynkPort);
	}

	//bool s = false;
	//unsigned long t = millis();
	//while (Blynk.connect() != true) {
	//	if (millis() - t > 100) {
	//		t = millis();
	//		s = !s;
	//		if (s) {
	//			LED_ON();
	//		}
	//		else {
	//			LED_OFF();
	//		}
	//	}
	//	handle_serial();
	//	handle_button();
	//}
}


void update_firmware(String url = "") {
	String link;
	if (url.length() > 0) {
		link = url;
	}
	String d = "Update Firmware\r\n" + link;
	Dprintln(d);

	bool ret = ESPhttpUpdate.update(link);

	switch (ret) {
	case HTTP_UPDATE_FAILED:
		Dprintf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
		break;

	case HTTP_UPDATE_NO_UPDATES:
		Dprintln(F("HTTP_UPDATE_NO_UPDATES"));
		break;

	case HTTP_UPDATE_OK:
		Sprintln(F("HTTP_UPDATE_OK"));
		restart(true);
		break;
	}
}void set_wifi_from_command(String& cmd) {
	//set wifi <ssid>|<psk>
	cmd = cmd.substring(9);
	cmd.trim();
	int pos = cmd.indexOf("|");
	if (pos > 0) {
		String _ssid = cmd.substring(0, pos);
		String _pw = cmd.substring(pos + 1);
		WiFi.disconnect();
		delay(100);
		WiFi.begin(_ssid.c_str(), _pw.c_str());
		Sprint(F("Connecting to "));
		Sprintln(_ssid);
		Sprint(F("Password: "));
		Sprintln(_pw);
		if (WiFi.waitForConnectResult() == WL_CONNECTED) {
			Sprintln(F("Connected"));
		}
		Sprint(F("IP: "));
		Sprintln(WiFi.localIP());
	}
}
void res_wifi() {
	String w = "WiFi: " + WiFi.SSID() + " - " + String(wifi_quality());
	Dprintln(w);
	w = WiFi.SSID() + " - " + String(wifi_quality());
	Blynk.virtualWrite(VWIFI, w);
}
void res_version() {
	String v = "Version: " VERSION;
	Dprintln(v);
}
void handle_command(String cmd) {
	yield();
	cmd.trim();
	if (cmd.length() <= 0) {
		return;
	}

	if (cmd == "/rs" || cmd == "/restart") {
		restart(true);
		delay(100);
	}
	else if (cmd.startsWith("/uf")) {
		String url;
		url = cmd.substring(3);
		url.trim();
		if (!url.startsWith("http")) {
			url = "http://gith.cf/files/Smart-Switch.bin";
		}
		update_firmware(url);

	}
	else if (cmd == "/v" || cmd.startsWith("/version")) {
		res_version();
	}
	else if (cmd == "/b" || cmd == "/blynk") {
		String t = "Blynk Token: " + blynkToken;
		Dprintln(t);
	}
	else if (cmd.startsWith("/set token")) {
		String token;
		token = cmd.substring(10);
		token.trim();

		String d = "Token : " + token;
		Dprintln(d);
		blynkToken = token;
		save_blynk_token();
		delay(500);
		restart(false);

		//Blynk.config(blynkToken.c_str(), token.c_str(), blynkPort);
		//
		//bool s = false;
		//while (Blynk.connect() != true) {
		//	s = !s;
		//	if (s) {
		//		LED_ON();
		//	}
		//	else {
		//		LED_OFF();
		//	}
		//	delay(100);
		//}
		//blynkToken = token;
	}
	else if (cmd == "/id") {
		String i = "ID: " + HARDWARE_ID;
		Dprintln(i);
	}
	else if (cmd == "/reset") {
		String i = "WiFi reset";
		Dprintln(i);
		create_AP_portal();
	}
	else if (cmd == "/w" || cmd.startsWith("/wifi")) {
		res_wifi();
	}
	else if (cmd.startsWith("/set wifi")) {
		//set wifi <ssid>|<psk>
		set_wifi_from_command(cmd);
	}
	else {
		String e = "Command unknown";
		Dprintln(e);
	}
	delay(1); yield();
}

void handle_serial() {
	if (Serial.available()) {
		String cmd = Serial.readStringUntil('\n');
		cmd.trim();
		handle_command(cmd);
	}
	delay(1); yield();
}

String getID() {
	byte mac[6];
	WiFi.macAddress(mac);
	String id;
	for (int i = 3; i < 6; i++)
	{
		id += mac[i] < 16 ? "0" + String(mac[i], HEX) : String(mac[i], HEX);
	}
	id.toUpperCase();
	Sprintln(id);
	return id;
}

bool mqtt_publish(String topic, String payload, bool retain) {
	if (!mqtt_client.connected()) {
		return false;
	}
	LED_ON();

	bool ret = mqtt_client.publish(topic.c_str(), payload.c_str(), retain);
	LED_OFF();
	yield();
	return ret;
}
void mqtt_reconnect() {
	if (!mqtt_client.connected()) {
		Sprintln(("\r\nAttempting MQTT connection..."));
		String id = HARDWARE_ID + getID();
		if (mqtt_client.connect(id.c_str(), mqtt_user, mqtt_password, tp_stt.c_str(), MQTTQOS1, true, "offline")) {
			Sprintln(("Connected."));
			mqtt_publish(tp_stt, "online", true);
			mqtt_client.subscribe(tp_req.c_str());
		}
		else {
			Sprint(("failed, rc="));
			Sprintln(String(mqtt_client.state()));
			return;
		}
	}
}
void mqtt_callback(char* topic, uint8_t* payload, unsigned int length) {
	yield();
	String pl;
	LED_ON();
	for (uint i = 0; i < length; i++) {
		pl += (char)payload[i];
	}
	LED_OFF();
	Sprintln(pl);

	if (pl == "1on") {
		Switch1.turn(true);
		String d = s_time() + "MQTT | OUTPUT1 = " + String(Switch1.status);
		Dprintln(d);
		notify(d);
	}
	else if (pl == "1off") {
		Switch1.turn(false);
		String d = s_time() + "MQTT | OUTPUT1 = " + String(Switch1.status);
		Dprintln(d);
		notify(d);
	}
	else if (pl == "2on") {
		Switch2.turn(true);
		String d = s_time() + "MQTT | OUTPUT2 = " + String(Switch2.status);
		Dprintln(d);
		notify(d);
	}
	else if (pl == "2off") {
		Switch2.turn(false);
		String d = s_time() + "MQTT | OUTPUT2 = " + String(Switch2.status);
		Dprintln(d);
		notify(d);
	}
	else if (pl == "3on") {
		Switch3.turn(true);
		String d = s_time() + "MQTT | OUTPUT3 = " + String(Switch3.status);
		Dprintln(d);
		notify(d);
	}
	else if (pl == "3off") {
		Switch3.turn(false);
		String d = s_time() + "MQTT | OUTPUT3 = " + String(Switch3.status);
		Dprintln(d);
		notify(d);
	}
	else handle_command(pl);
	delay(1); yield();
}
void mqtt_init() {
	mqtt_client.setServer(mqtt_server, mqtt_port);
	mqtt_client.setCallback(mqtt_callback);
}
void mqtt_loop() {
	if (!WiFi.isConnected()) {
		return;
	}
	if (!mqtt_client.connected()) {
		mqtt_reconnect();
	}
	mqtt_client.loop();
	yield();
}

BLYNK_CONNECTED() {
	res_version();
	rtc.begin();

	String w = String(wifi_quality()) + "% - " + WiFi.SSID();
	Blynk.virtualWrite(VWIFI, w);

	Blynk.syncVirtual(VTIMEOUT1, VTIMEOUT2, VTIMEOUT3, VMODESTART);
	BLYNK_connected = true;
	delay(1); yield();
}

BLYNK_WRITE(TERMINAL) {
	String cmd = String(param.asString());
	cmd.trim();
	handle_command(cmd);
	delay(1); yield();
}

BLYNK_WRITE(VPIN1) {
	int val = param.asInt();
	Switch1.turn(val);
	String d = s_time() + "APP | OUTPUT1 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VPIN2) {
	int val = param.asInt();
	Switch2.turn(val);
	String d = s_time() + "APP | OUTPUT2 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VPIN3) {
	int val = param.asInt();
	Switch3.turn(val);
	String d = s_time() + "APP | OUTPUT3 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VTIMEOUT1) {
	int val = param.asInt();
	Switch1.timeout = val * 60000;
	String d = s_time() + "TIMEOUT1 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VTIMEOUT2) {
	int val = param.asInt();
	Switch2.timeout = val * 60000;
	String d = s_time() + "TIMEOUT2 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VTIMEOUT3) {
	int val = param.asInt();
	Switch3.timeout = val * 60000;
	String d = s_time() + "TIMEOUT3 = " + String(val);
	Dprintln(d);
	delay(1); yield();
}

BLYNK_WRITE(VMODESTART) {
	int val = param.asInt();
	switch (val)
	{
	case VMODE_OFF:
	case VMODE_BUTTON:
	case VMODE_APP:
		STARTUP_MODE = VMODESTART_e(val);
		break;
	default:
		STARTUP_MODE = VMODE_OFF;
		break;
	}
	String d = s_time() + "STARTUP_MODE = " + STARTUP_MODE_toString();
	Dprintln(d);
	STARTUP_MODE_synced = true;
	delay(1); yield();
}

BLYNK_APP_CONNECTED() {
	String x = "App Connected.";
	Sprintln(x);
	mqtt_publish(tp_res, x, false);
	BlynkConnected = true;
}

// This is called when Smartphone App is closed
BLYNK_APP_DISCONNECTED() {
	String x = "App Disconnected.";
	Sprintln(x);
	mqtt_publish(tp_res, x, false);
	BlynkConnected = false;
}

void startup_mode() {
	if (BLYNK_connected && STARTUP_MODE_synced) {
		BLYNK_connected = false;
		STARTUP_MODE_synced = false;

		switch (STARTUP_MODE)
		{
		case VMODE_OFF:
			Switch1.turn(false);
			Switch2.turn(false);
			Switch3.turn(false);
			break;
		case VMODE_BUTTON:
			Button1.read();
			if (Button1.isReleased()) {
				Switch1.turn(false);
			}
			else if (Button1.isPressed()) {
				Switch1.turn(true);
			}

			Button2.read();
			if (Button2.isReleased()) {
				Switch2.turn(false);
			}
			else if (Button2.isPressed()) {
				Switch2.turn(true);
			}

			Button3.read();
			if (Button3.isReleased()) {
				Switch3.turn(false);
			}
			else if (Button3.isPressed()) {
				Switch3.turn(true);
			}
			break;
		case VMODE_APP:
			Blynk.syncVirtual(VPIN1, VPIN2, VPIN3);
			delay(100);
			for (int i = 0; i < 50; i++)
			{
				Blynk.run();
			}
			break;
		default:
			break;
		}

		String d = STARTUP_MODE_toString();
		d = s_time() + "START | OUTPUT1 = " + String(Switch1.status);
		d += "\r\n";
		d += s_time() + "START | OUTPUT2 = " + String(Switch2.status);
		d += "\r\n";
		d += s_time() + "START | OUTPUT3 = " + String(Switch3.status);
		Dprintln(d);
	}
}

void notify(String n) {
	if (!BlynkConnected) {
		Blynk.notify(n);
	}
}
void auto_off_timeout() {
	if (Switch1.status && (Switch1.timeout > 0) && (millis() - Switch1.last_changed > Switch1.timeout)) {
		Switch1.turn(false);
		String d = s_time() + "AUTO | OUTPUT1 = " + String(Switch1.status);
		Dprintln(d);
		notify(d);
		delay(1); yield();
	}
	if (Switch2.status && (Switch2.timeout > 0) && (millis() - Switch2.last_changed > Switch2.timeout)) {
		Switch2.turn(false);
		String d = s_time() + "AUTO | OUTPUT2 = " + String(Switch2.status);
		Dprintln(d);
		notify(d);
		delay(1); yield();
	}
	if (Switch3.status && (Switch3.timeout > 0) && (millis() - Switch3.last_changed > Switch3.timeout)) {
		Switch3.turn(false);
		String d = s_time() + "AUTO | OUTPUT3 = " + String(Switch3.status);
		Dprintln(d);
		notify(d);
		delay(1); yield();
	}
}

void blink_led() {
	static bool s = false;
	static unsigned long t = millis();
	if (millis() - t > 1900) {
		t = millis();
		s = true;
		LED_ON();
	}
	if (s && (millis() - t > 100)) {
		t = millis();
		s = false;
		LED_OFF();
	}
}

void setup()
{
	hardware_init();
	load_blynk_token();
	WiFi_init();
	Blynk_init();
	mqtt_init();
	LED_OFF();
}

// Add the main program code into the continuous loop() function
void loop()
{
	Blynk.run();
	mqtt_loop();
	startup_mode();
	handle_serial();
	handle_button();
	blink_led();
	auto_off_timeout();
}
