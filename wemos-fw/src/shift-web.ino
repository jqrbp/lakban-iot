#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#define HTTPPORT  8088
#define WSPORT 81

const char* ssid = "U+Net2DB0";
const char* password = "5000013954";
MDNSResponder mdns;

ESP8266WebServer server(HTTPPORT);
WebSocketsServer webSocket(WSPORT);

//16 => D2
//15 => D10
//14 => D5
//13 => D7
//12 => D6
//4 => D14
//5 => D15
//2 => LED
//

#define READSHIFTIN_PERIOD 20
const int led = 2;
const int pinrclk = 5;
const int pinrdata = 4;
const int pinpl = 14;
const int pinclk = 12;
const int pindata = 13;
byte datain = 0;

// byte ledCnt;
byte switchState, switchStateIn, switchState_old;
byte ledState, ledState_old;

String getBinString(int _val) {
  String strout;
  byte temp;
  for(int i = 7; i >=0; i--) {
    temp = _val & (1 << i);
    if(temp) strout+="1";
    else strout+="0";
  }

  return strout;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    int numval;
    String str;
    switch(type) {
        case WStype_DISCONNECTED:
            // Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            // Serial.Printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            // Serial.Printf("[%u] get Text: %s\n", num, payload);
            switch (payload[0]) {
              case '#':
                numval = (int)(payload[1] - 48);
                ledState = ledState_old ^ (1 << numval);
                shiftOut(pinrdata, pinrclk, MSBFIRST, ledState);
                // writeShiftOut(pinrclk, pinrdata, ledState);
                ledState_old = ledState;
                str = "*" + getBinString(ledState);
                // send message to client
                webSocket.sendTXT(num, str);

                // send data to all connected clients
                // webSocket.broadcastTXT("message here");
                break;

              case '?':
                str = "*" + getBinString(ledState);
                webSocket.sendTXT(num, str);
                break;
            }

            break;
    }

}

void printHtmlHome(String _wsaddr) {
  String styleStr = "<style> .button { border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 32px; margin: 8px 16px; cursor: pointer; } .inputtxt {font-size: 32px;} .blue {background-color: #008CBA;} .green {background-color: #4CAF50;} .red {background-color: #f44336;} .black {background-color: #555555;} </style>";

  String scriptHeadStr = "<script language=\"javascript\">function switchon(a){disableButton(),ws.send(a)}function send(){var a=document.getElementById(\"txtSend\").value;ws.send(a)}function disableButton(){for(i=0;i<8;i++){var a=document.getElementById(\"btn\"+i);a.classList.remove(\"black\"),a.classList.remove(\"red\"),a.innerHTML=\"Tunggu\",a.disabled=!0}}function enableButton(){for(i=0;i<8;i++){var a=document.getElementById(\"btn\"+i);a.innerHTML=\"Tombol \"+(i+1),a.disabled=!1}}function setButton(a){var b=a.length-1;for(i=0;i<b;i++){var c=document.getElementById(\"btn\"+(b-i-1));\"0\"==a[i+1]?(c.classList.remove(\"black\"),c.classList.add(\"red\")):\"1\"==a[i+1]&&(c.classList.remove(\"red\"),c.classList.add(\"black\"))}msgState=a}function connect(){connectToIp(document.getElementById(\"ipaddress\").value)}function connectToIp(a){\"WebSocket\"in window?(ws=new WebSocket(\"ws://\"+a),ws.onopen=function(){ws.send(\"?\"),alert(\"Websocket Tersambung\")},ws.onmessage=function(a){var b=a.data;document.getElementById(\"txtRecv\").value=b,\"*\"==b[0]&&(enableButton(),setButton(b))},ws.onclose=function(){alert(\"Sambungan sudah ditutup...\")}):alert(\"WebSocket TIDAK didukung oleh Browser Anda!\")}var ws,msgState=\"*22222222\";</script>";

  String scriptBodyStr = "<script language=\"javascript\">connectToIp(\"" + _wsaddr + "\");document.getElementById(\"ipaddress\").value=\"" + _wsaddr + "\";</script>";

  String bodyStr = "<input type=\"text\" name=\"ip\" class=\"inputtxt\" id=\"ipaddress\"><button type=\"button\" class=\"button green\" onclick=\"connect()\">Sambung</button><br> <input type=\"text\" name=\"txtSend\" class=\"inputtxt\" id=\"txtSend\"><button type=\"button\" class=\"button green\" onclick=\"send()\">Kirim</button><br> <button type=\"button\" class=\"button\" id=\"btn0\" onclick=\"switchon('#0')\">Tombol 1</button><button type=\"button\" class=\"button\" id=\"btn7\" onclick=\"switchon('#7')\">Tombol 8</button><br> <button type=\"button\" class=\"button\" id=\"btn1\" onclick=\"switchon('#1')\">Tombol 2</button><button type=\"button\" class=\"button\" id=\"btn6\" onclick=\"switchon('#6')\">Tombol 7</button><br> <button type=\"button\" class=\"button\" id=\"btn2\" onclick=\"switchon('#2')\">Tombol 3</button><button type=\"button\" class=\"button\" id=\"btn5\" onclick=\"switchon('#5')\">Tombol 6</button><br> <button type=\"button\" class=\"button\" id=\"btn3\" onclick=\"switchon('#3')\">Tombol 4</button><button type=\"button\" class=\"button\" id=\"btn4\" onclick=\"switchon('#4')\">Tombol 5</button><br> <input type=\"text\" name=\"txtRecv\" id=\"txtRecv\">";

  String htmlStr = "<!DOCTYPE HTML><html><head>" + styleStr + scriptHeadStr + "</head>" + "<body>" + bodyStr + scriptBodyStr + "</body></html>";

  digitalWrite(led, 1);
  server.send(200, "text/html", htmlStr);
  digitalWrite(led, 0);
}

void handleRoot() {
  String wsAddrStr = server.arg("wsaddr");

  if (wsAddrStr.length() == 0) {
    IPAddress ip = WiFi.localIP();
    wsAddrStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2])
                  + "." + String(ip[3]) + ":" + String(WSPORT);
  }

  printHtmlHome(wsAddrStr);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void initWebServer(void) {
  WiFi.begin(ssid, password);
  // Serial.Println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.Print(".");
  }
  // Serial.Println("");
  // Serial.Print("Connected to ");
  // Serial.Println(ssid);
  // Serial.Print("IP address: ");
  // Serial.Println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/id", [](){
    server.send(200, "text/plain", "wemos-shift-web");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.Println("HTTP server started");

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Add service to MDNS
  MDNS.addService("http", "tcp", HTTPPORT);
  MDNS.addService("ws", "tcp", WSPORT);
}

void initSwitchPin(void) {
  pinMode(pinrclk, OUTPUT);
  pinMode(pinrdata, OUTPUT);

  pinMode(pinpl, OUTPUT);
  pinMode(pinclk, OUTPUT);
  pinMode(pindata, INPUT);

  pinMode(led, OUTPUT);

  ledState = 0xff;
  ledState_old = 0xff;
  switchState = 0xff;
  switchState_old = 0xff;
  shiftOut(pinrdata, pinrclk, MSBFIRST, ledState);

  delay(100);
}

byte readShiftIn(int _pinpl, int _pinclk, int _pindata) {
  int temp;
  byte dataout = 0;
  digitalWrite(_pinpl, 0);
  delayMicroseconds(READSHIFTIN_PERIOD);
  digitalWrite(_pinpl, 1);

  for(int i=7;i>=0;i--)
  {
    digitalWrite(_pinclk, 0);
    delayMicroseconds(READSHIFTIN_PERIOD);
    temp = digitalRead(_pindata);
    if(temp) {
      dataout = dataout | (1 << i);
    }

    digitalWrite(_pinclk, 1);
  }

  return dataout;
}

// void writeShiftOut(int _pinclk, int _pindata, byte _val) {
//   for (int i = 7; i >=0 ; i--) {
//     digitalWrite(_pinclk, 0);
//
//     if(bitRead(_val,i)) digitalWrite(_pindata, 1);
//     else digitalWrite(_pindata, 0);
//
//     digitalWrite(_pinclk, 1);
//   }
// }

void check_switch(void) {
  int i;
  String bstr;
  switchStateIn = readShiftIn(pinpl, pinclk, pindata);
  switchState = ((switchStateIn >> 4)) |  (switchStateIn << 4);

  if(switchState_old != switchState) {
    for(i=0;i<8;i++) {
      if((switchState & (1 << i)) < (switchState_old & (1 << i))) {
        ledState = ledState_old ^ (1 << i);
        // shiftOut(pinrdata, pinrclk, MSBFIRST, 0);
        shiftOut(pinrdata, pinrclk, MSBFIRST, ledState);
        // delay(100);
        // shiftOut(pinrdata, pinrclk, MSBFIRST, ledState);
        // delay(100);
        ledState_old = ledState;
//        Serial.println(getBinString(ledState));
      }
    }
    bstr = "*" + getBinString(ledState);
    webSocket.broadcastTXT(bstr);
  }

  switchState_old = switchState;
}

void setupArduinoOTA(void) {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword("panjul1017");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    // String type;
    // if (ArduinoOTA.getCommand() == U_FLASH)
    //   type = "sketch";
    // else // U_SPIFFS
    //   type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating ");// + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setup(void){
  setupArduinoOTA();
  initSwitchPin();
  // Serial.begin(115200);
  initWebServer();
}

void loop(void){
  ArduinoOTA.handle();
  webSocket.loop();
  server.handleClient();
  check_switch();
  delay(50);
}
