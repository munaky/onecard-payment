#include <ESP8266WebServer.h>
#include <ArduinoHttpClient.h>
#include "LiquidCrystal_I2C_PLUS.h"
#include <WiFiClient.h>
#include <DNSServer.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "html_content.h"

#define RST_PIN D3
#define SDA_PIN D4

ESP8266WebServer server(80);
DNSServer dnsServer;
WiFiClient wifi;
MFRC522 rfid(SDA_PIN, RST_PIN);
LiquidCrystal_I2C_PLUS lcd(0x27, 16, 2);
DynamicJsonDocument doc(1024);

//Host
const char host[] = "payment.generasi4bogor.com";
const int port = 80;
const String mainUrl = "/api/rfid/get";
bool isConnected = false;
HttpClient client = HttpClient(wifi, host, port);

//
const String ruang = "OneCard";
const String ssid = ruang;
const String password = "";
const int wifiTimeout = 15;  //TimeOut For Wifi Connection (Seconds)
String token, targetSSID, targetPass;

void fit(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printFit(msg);
}

void info(String msg) {
  fit(msg);
  Serial.println(msg);
  delay(2000);
}

void startAP() {
  info("Starting Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  info("IP Address: " + WiFi.softAPIP().toString());
}

bool setNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(targetSSID, targetPass);

  for (int x = 0; WiFi.status() != WL_CONNECTED && x < wifiTimeout; x++) {
    Serial.print(".");
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    info("Koneksi Gagal");
    return false;
  }

  Serial.println();

  info("Koneksi Berhasil");

  return true;
}

String listNetwork(String networks = "") {
  int totalN = WiFi.scanNetworks();
  for (int x = 0; x < totalN; x++) {
    networks += "<h3><a href='javascript:void(0)' onclick='setSSID(this.innerHTML)'>" + WiFi.SSID(x) + "</a></h3>";
  }
  return networks;
}

void setConfig() {
  targetSSID = server.arg("ssid");
  targetPass = server.arg("password");

  Serial.println("Config is Ready\nSSID : " + targetSSID + "\nPassword : " + targetPass);

  if (validation() == false) {
    info("Validasi Gagal");
    info("Memulai Ulang");
    return startAP();
  }

  isConnected = true;

  info("Tempelkan Kartu RFID");
}

void mainWeb() {
  server.send(200, "text/html", mainPage(listNetwork()));
}

void setRoutes() {
  server.on("/", mainWeb);
  server.on("/setting", setConfig);
}

bool validation() {
  // WiFi Validation
  info("Menunggu Koneksi");

  if (setNetwork() == false) {
    return false;
  }

  info("Validasi Berhasil");
  return true;
}

String getId(String id = "") {
  for (byte x = 0; x < rfid.uid.size; x++) {
    id += rfid.uid.uidByte[x];
  }
  return id;
}

bool sendRequest() {
  info("Validasi...");

  String idKartu = getId();
  info(idKartu);
  String data = "card_id=" + idKartu + "&token=" + token;

  client.beginRequest();
  client.post(mainUrl);
  client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
  client.sendHeader("Content-Length", data.length());
  client.beginBody();
  client.print(data);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode != 200) {
    info("Request Gagal");
    Serial.println(statusCode);
    return false;
  }

  deserializeJson(doc, response);

  for (int x = 0; x < doc.size(); x++) {
    info(doc[x]);
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  info("Starting...");

  // Setup RFID Sensor
  SPI.begin();
  rfid.PCD_Init();

  // Setup Access Point
  startAP();

  // Setup Server
  setRoutes();
  server.begin();
}

void loop() {
  if (!isConnected) {
    server.handleClient();
    return;
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    info("Tempelkan Kartu RFID");
    return;
  }

  sendRequest();
}
