// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       Contactor3.ino
	Created:	12/6/2018 9:33:17 AM
	Author:     GITH\tacke
*/


#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <Time.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <Button.h>
#include <FS.h>

#define VERSION     "0.1"


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

String HARDWARE_ID = "SWITCH_GITH";

const char* file_blynk_token = "/blynk.token";

String blynkToken = "d8297a5187c14fea8b04e92d8e4c2292";
String blynkServer = "gith.cf";
int blynkPort = 8442;

WidgetTerminal Terminal(TERMINAL);
WidgetRTC rtc;

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

#define Dprint(x)		{ LED_ON(); Sprint(x); Sflush(); Tprint(x); Tflush(); LED_OFF(); }
#define Dprintln(x)		{ LED_ON(); Sprintln(x); Sflush(); Tprintln(x); Tflush(); LED_OFF(); }  
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
	void turn(bool stt) {
		status = stt;
		digitalWrite(_pin, status);
	}
	void toggle() {
		status = !status;
		digitalWrite(_pin, status);
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
		Blynk.virtualWrite(VPIN1, Switch1.status);
		String d = "HUB | OUTPUT1 = " + String(Switch1.status);
		Dtime();
		Dprintln(d);
		delay(1); yield();
	}

	if (Button2.wasPressed() || Button2.wasReleased()) {
		Switch2.toggle();
		Blynk.virtualWrite(VPIN2, Switch2.status);
		String d = "HUB | OUTPUT2 = " + String(Switch2.status);
		Dtime();
		Dprintln(d);
		delay(1); yield();
	}

	if (Button3.wasPressed() || Button3.wasReleased()) {
		Switch3.toggle();
		Blynk.virtualWrite(VPIN3, Switch3.status);
		String d = "HUB | OUTPUT3 = " + String(Switch3.status);
		Dtime();
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
	Sprintln("Server\t" + blynkServer);
	Sprintln("Port\t" + blynkPort);
	Sprintln("Token\t" + blynkToken);

	static IPAddress ipServer;
	if (ipServer.fromString(blynkServer)) {
		Blynk.config(blynkToken.c_str(), ipServer, blynkPort);
	}
	else {
		Blynk.config(blynkToken.c_str(), blynkServer.c_str(), blynkPort);
	}

	bool s = false;
	unsigned long t = millis();
	while (Blynk.connect() != true) {
		if (millis() - t > 100) {
			t = millis();
			s = !s;
			if (s) {
				LED_ON();
			}
			else {
				LED_OFF();
			}
		}
		handle_serial();
		handle_button();
	}
	LED_OFF();
}


void update_firmware(String url = "") {
	//update firmware
	Sprintln(url);
	Dprintln(F("Update Firmware"));
	String link;
	if (url.length() > 0) {
		link = url;
	}
	Dprint("URL: ");
	Dprintln(link);
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
}
void handle_command(String cmd) {
	yield();
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
	else if (cmd == "/v" || cmd == "/version") {
		String v = "Version: " VERSION;
		Dprintln(v);
	}
	else if (cmd == "/b" || cmd == "/blynk") {
		String t = "Blynk Token: " + blynkToken;
		Dprintln(t);
	}
	else if (cmd == "/set token") {
		String token;
		token = cmd.substring(10);
		token.trim();

		String d = "Token : " + token;
		Dprintln(d);

		Blynk.config(blynkToken.c_str(), token.c_str(), blynkPort);

		bool s = false;
		while (Blynk.connect() != true) {
			s = !s;
			if (s) {
				LED_ON();
			}
			else {
				LED_OFF();
			}
			delay(100);
		}
		blynkToken = token;
		save_blynk_token();
	}
	else if (cmd == "/id") {
		String i = "ID: " + HARDWARE_ID;
		Dprintln(i);
	}
	else if (cmd == "/reset") {
		String i = "Factory reset";
		Dprintln(i);
		create_AP_portal();
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



BLYNK_CONNECTED() {
	// Synchronize time on connection
	//Blynk.syncAll();
	rtc.begin();
	Blynk.syncVirtual(VPIN1, VPIN2, VPIN3);
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
	String d = "APP | OUTPUT1 = " + String(val);
	Dtime();
	Dprintln(d);
	Switch1.turn(val);
	delay(1); yield();
}

BLYNK_WRITE(VPIN2) {
	int val = param.asInt();
	String d = "APP | OUTPUT2 = " + String(val);
	Dtime();
	Dprintln(d);
	Switch2.turn(val);
	delay(1); yield();
}

BLYNK_WRITE(VPIN3) {
	int val = param.asInt();
	String d = "APP | OUTPUT3 = " + String(val);
	Dtime();
	Dprintln(d);
	Switch3.turn(val);
	delay(1); yield();
}


void setup()
{
	hardware_init();
	//load_blynk_token();
	WiFi_init();
	Blynk_init();
}

// Add the main program code into the continuous loop() function
void loop()
{
	Blynk.run();
	handle_serial();
	handle_button();
}
