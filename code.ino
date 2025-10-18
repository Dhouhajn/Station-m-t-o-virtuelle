#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
/* ======== Paramètres utilisateur ======== */
const char *WIFI_SSID   = "Wokwi-GUEST";         // WiFi simulé par Wokwi
const char *WIFI_PASS   = "";                    // Pas de mot de passe pour Wokwi-GUEST
const char *TB_HOST     = "mqtt.eu.thingsboard.cloud";  // Serveur ThingsBoard (Europe)
const uint16_t TB_PORT  = 1883;                  // Port MQTT standard
const char *TB_TOKEN    = "zyw06qf9q386k55b5etu";  // Remplace par ton Access Token
/* ======================================= */

/* Initialisation LCD (adresse I2C = 0x27, 16 colonnes, 2 lignes) */
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient wifiClient;
PubSubClient tbClient(wifiClient);

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connexion au Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connecté !");
}

void connectTB() {
  while (!tbClient.connected()) {
    Serial.print("Connexion à ThingsBoard...");
    if (tbClient.connect("ESP32_StationMeteo", TB_TOKEN, nullptr)) {
      Serial.println(" Connecté !");
    } else {
      Serial.print(" Échec, rc=");
      Serial.print(tbClient.state());
      Serial.println(" Réessai dans 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  tbClient.setServer(TB_HOST, TB_PORT);
  dht.begin();
  // Initialisation du LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Station Meteo");
  lcd.setCursor(0, 1);
  lcd.print("Demarrage...");
  delay(2000);
}

void loop() {
  connectWiFi();
  connectTB();
  tbClient.loop();

  static uint32_t t0;
  if (millis() - t0 > 5000) {  // Envoi toutes les 5 secondes
    t0 = millis();
    
    // // Simulation des capteurs
    // float temperature = random(200, 360) / 10.0;  // 20.0-35.9°C
    // float humidite = random(400, 610) / 10.0;     // 40.0-60.9%
    float temperature = dht.readTemperature();
float humidite = dht.readHumidity();
if (isnan(temperature) || isnan(humidite)) {
  Serial.println("Erreur de lecture du capteur simulé !");
  return;
}
    // Affichage sur Serial Monitor
    Serial.print("Température simulée : ");
    Serial.print(temperature);
    Serial.print(" °C | Humidité simulée : ");
    Serial.print(humidite);
    Serial.println(" %");

    // Affichage sur LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidite);
    lcd.print(" %");

    // Création du payload JSON
    String payload = "{\"temperature\":" + String(temperature) + ", \"humidite\":" + String(humidite) + "}";
    
    // Envoi via MQTT
    if (tbClient.publish("v1/devices/me/telemetry", payload.c_str())) {
      Serial.println("Données envoyées : " + payload);
    } else {
      Serial.println("Échec d'envoi");
    }
  }
}