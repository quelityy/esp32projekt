#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid     = "";
const char* password = "";

#define TRIG1 5
#define ECHO1 18
#define TRIG2 19
#define ECHO2 23
#define PIN_DHT 4
#define PIN_BUZZER 25
#define PIN_PRZYCISK 13
#define LED_ZIELONA   26
#define LED_CZERWONA  27
#define LED_NIEBIESKA 14
#define LED_ZOLTA     12

Adafruit_SSD1306 ekran(128, 64, &Wire, -1);
DHT dht(PIN_DHT, DHT11);
WebServer serwer(80);

int   licznikOsob  = 0;
int   tryb         = 1;
float temperatura  = 0.0;
float wilgotnosc   = 0.0;

const int PROG_ODLEGLOSCI = 40;

long zmierzOdleglosc(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long czas = pulseIn(echo, HIGH, 20000);
  if (czas == 0) return 999;
  return czas * 0.034 / 2;
}

void aktualizujDiodyTrybu() {
  digitalWrite(LED_NIEBIESKA, tryb == 1 ? HIGH : LOW);
  digitalWrite(LED_ZOLTA,     tryb == 2 ? HIGH : LOW);
}

void rysujEkran() {
  ekran.clearDisplay();
  ekran.setTextColor(SSD1306_WHITE);
  if (tryb == 1) {
    ekran.setTextSize(1);
    ekran.setCursor(0, 0);
    ekran.println("== LICZNIK OSOB ==");
    ekran.setTextSize(3);
    ekran.setCursor(50, 20);
    ekran.println(licznikOsob);
    ekran.setTextSize(1);
    ekran.setCursor(0, 56);
    ekran.println("Auto-Zmiana: 5s");
  } else {
    ekran.setTextSize(1);
    ekran.setCursor(0, 0);
    ekran.println("== KLIMAT POKOJU ==");
    ekran.setTextSize(2);
    ekran.setCursor(0, 14);
    ekran.print("T: "); ekran.print(temperatura, 1); ekran.println("C");
    ekran.setCursor(0, 36);
    ekran.print("H: "); ekran.print(wilgotnosc, 1);  ekran.println("%");
    ekran.setTextSize(1);
    ekran.setCursor(0, 56);
    ekran.println("Auto-Zmiana: 5s");
  }
  ekran.display();
}

String generujStrone() {
  String s = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  s += "<meta http-equiv='refresh' content='5'>";
  s += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  s += "<title>ESP32 Monitor</title>";
  s += "<style>";
  s += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  s += "body { background: #0f172a; font-family: Arial, sans-serif; min-height: 100vh; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 2rem; }";
  s += ".label { color: #94a3b8; font-size: 13px; letter-spacing: 2px; text-transform: uppercase; margin-bottom: 0.5rem; }";
  s += "h1 { color: #f1f5f9; font-size: 22px; font-weight: 600; margin-bottom: 1.5rem; }";
  s += ".grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; width: 100%; max-width: 500px; }";
  s += ".card { background: #1e293b; border: 1px solid #334155; border-radius: 12px; padding: 1.25rem; text-align: center; }";
  s += ".card-label { color: #64748b; font-size: 12px; letter-spacing: 1px; text-transform: uppercase; margin-bottom: 8px; }";
  s += ".card-value { font-size: 2rem; font-weight: 700; }";
  s += ".card-unit { font-size: 13px; color: #64748b; margin-top: 4px; }";
  s += ".status { color: #22c55e; font-size: 12px; margin-top: 1.5rem; display: flex; align-items: center; gap: 6px; }";
  s += ".dot { width: 8px; height: 8px; background: #22c55e; border-radius: 50%; animation: pulse 2s infinite; }";
  s += "@keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }";
  s += "</style></head><body>";
  s += "<p class='label'>ESP32 Monitor</p>";
  s += "<h1>Panel sterowania</h1>";
  s += "<div class='grid'>";
  s += "<div class='card'><div class='card-label'>Osoby</div><div class='card-value' style='color:#60a5fa;'>" + String(licznikOsob) + "</div><div class='card-unit'>obecnych</div></div>";
  s += "<div class='card'><div class='card-label'>Temp.</div><div class='card-value' style='color:#f97316;'>" + String(temperatura, 1) + "</div><div class='card-unit'>&deg;C</div></div>";
  s += "<div class='card'><div class='card-label'>Wilg.</div><div class='card-value' style='color:#34d399;'>" + String(wilgotnosc, 1) + "</div><div class='card-unit'>%</div></div>";
  s += "</div>";
  s += "<div class='status'><div class='dot'></div> Odswiezanie co 5 sekund</div>";
  s += "</body></html>";
  return s;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(LED_ZIELONA, OUTPUT); pinMode(LED_CZERWONA, OUTPUT);
  pinMode(LED_NIEBIESKA, OUTPUT); pinMode(LED_ZOLTA, OUTPUT);

  if (!ekran.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nie znaleziony!");
    while (true);
  }

  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(100); }
  Serial.println("Polaczono z WiFi!");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());

  serwer.on("/", []() { serwer.send(200, "text/html", generujStrone()); });
  serwer.begin();

  aktualizujDiodyTrybu();
  rysujEkran();
}

void loop() {
  serwer.handleClient();

  // --- AUTOMATYCZNA ROTACJA EKRANU CO 5 SEKUND ---
  static unsigned long ostatniOled = 0;
  if (millis() - ostatniOled >= 5000) {
    ostatniOled = millis();
    tryb = (tryb == 1) ? 2 : 1;
    aktualizujDiodyTrybu();
    rysujEkran();
  }

  // --- CZUJNIK 1: WEJŚCIE ---
  static unsigned long blokada1 = 0;
  if (millis() - blokada1 >= 1000) {
    long d1 = zmierzOdleglosc(TRIG1, ECHO1);
    if (d1 > 2 && d1 < PROG_ODLEGLOSCI) {
      licznikOsob++;
      digitalWrite(LED_ZIELONA, HIGH);
      tone(PIN_BUZZER, 1200, 150);
      delay(150);
      digitalWrite(LED_ZIELONA, LOW);
      rysujEkran();
      Serial.print("Wykryto WEJSCIE. Aktualny stan: ");
      Serial.println(licznikOsob);
      blokada1 = millis();
    }
  }

  // --- CZUJNIK 2: WYJŚCIE ---
  static unsigned long blokada2 = 0;
  if (millis() - blokada2 >= 1000) {
    long d2 = zmierzOdleglosc(TRIG2, ECHO2);
    if (d2 > 2 && d2 < PROG_ODLEGLOSCI) {
      if (licznikOsob > 0) licznikOsob--;
      digitalWrite(LED_CZERWONA, HIGH);
      tone(PIN_BUZZER, 600, 150);
      delay(150);
      digitalWrite(LED_CZERWONA, LOW);
      rysujEkran();
      Serial.print("Wykryto WYJSCIE. Aktualny stan: ");
      Serial.println(licznikOsob);
      blokada2 = millis();
    }
  }

  // --- ODCZYT DHT11 CO 2 SEKUNDY ---
  static unsigned long ostatniDHT = 0;
  if (millis() - ostatniDHT >= 2000) {
    ostatniDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperatura = t;
      wilgotnosc  = h;
    }
    if (tryb == 2) rysujEkran();
  }
}