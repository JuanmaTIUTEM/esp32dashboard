#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

// --- Configuración de pines ---
#define TRIG_PIN 5           // Pin para el Trigger del ultrasónico
#define ECHO_PIN 18          // Pin para el Echo del ultrasónico
#define DHTPIN 4             // Pin para el DHT11
#define DHTTYPE DHT11        // Tipo de sensor DHT11

// --- Configuración de la LCD ---
#define LCD_ADDRESS 0x3F
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// --- Instancias ---
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
DHT dht(DHTPIN, DHTTYPE);

// --- Configuración WiFi ---
// 1) WiFi AP (punto de acceso)
const char* ap_ssid = "Sensor1";      // Nombre del AP
const char* ap_password = "123456789";  // Contraseña del AP

// 2) Conectar a red WiFi externa
const char* wifi_ssid = "ModemLab_Soporte";
const char* wifi_password = "DSM_2025";

// --- Servidor ---
AsyncWebServer server(80);

// --- Variables ---
int numDevicesConnected = 0;
unsigned long lastSendTime = 0;
const long sendInterval = 5000;  // Enviar datos cada 5 segundos

void setup() {
  Serial.begin(115200);

  // Inicialización de sensores y LCD
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // --- Iniciar WiFi AP ---
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("WiFi AP iniciado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.softAPIP());

  // Mostrar IP del AP en la LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AP IP: ");
  lcd.print(WiFi.softAPIP());

  // --- Conectar a red WiFi externa ---
  Serial.println("\nConectando a la red WiFi externa...");
  WiFi.begin(wifi_ssid, wifi_password);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {  // Espera 10 seg
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a la red WiFi externa!");
    Serial.print("IP externa: ");
    Serial.println(WiFi.localIP());
    
    // Mostrar IP externa en la LCD
    lcd.setCursor(0, 1);
    lcd.print("Ext IP: ");
    lcd.print(WiFi.localIP());
  } else {
    Serial.println("\nNo se pudo conectar a la red WiFi externa.");
    lcd.setCursor(0, 1);
    lcd.print("Sin WiFi Ext");
  }

  // --- Rutas del servidor ---
  server.on("/api/distancia", HTTP_GET, [](AsyncWebServerRequest *request) {
    float distance = getDistance();
    String json = "{\"distancia\": " + String(distance) + "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/api/temperatura", HTTP_GET, [](AsyncWebServerRequest *request) {
    float temperatura = dht.readTemperature();
    if (isnan(temperatura)) temperatura = 0;

    String json = "{\"temperatura\": " + String(temperatura) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/api/humedad", HTTP_GET, [](AsyncWebServerRequest *request) {
    float humedad = dht.readHumidity();
    if (isnan(humedad)) humedad = 0;

    String json = "{\"humedad\": " + String(humedad) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><body>";
    html += "<h1>Datos de Sensores</h1>";
    html += "<p><a href='/api/distancia'>Ver Distancia</a></p>";
    html += "<p><a href='/api/temperatura'>Ver Temperatura</a></p>";
    html += "<p><a href='/api/humedad'>Ver Humedad</a></p>";
    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  server.begin();
}

// --- Función para medir distancia ---
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  return distance;
}

// --- Enviar datos a un servidor externo ---
void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {  // Solo enviar si está conectado a red externa
    WiFiClient client;
    HTTPClient http;

    http.begin(client, "http://192.168.4.1:3001/api/datos");  // IP del servidor
    http.addHeader("Content-Type", "application/json");

    float distancia = getDistance();
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    String ap_ip = WiFi.softAPIP().toString();
    String wifi_ip = WiFi.localIP().toString();

    if (isnan(temperatura)) temperatura = 0;
    if (isnan(humedad)) humedad = 0;
    if (isnan(distancia)) distancia = 0;

    String json = "{\"distancia\": " + String(distancia) + 
                  ", \"temperatura\": " + String(temperatura) + 
                  ", \"humedad\": " + String(humedad) + 
                  ", \"ap_ip\": \"" + ap_ip + "\", " +
                  "\"wifi_ip\": \"" + wifi_ip + "\"}";

    int httpResponseCode = http.POST(json);

    if (httpResponseCode > 0) {
      Serial.println("Datos enviados correctamente.");
    } else {
      Serial.println("Error al enviar datos.");
    }

    http.end();
  }
}

void loop() {
  // Mostrar datos en la LCD
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();
  float distancia = getDistance();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperatura);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humedad);
  lcd.print("%");

  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distancia);
  lcd.print(" cm");

  if (millis() - lastSendTime >= sendInterval) {
    sendDataToServer();
    lastSendTime = millis();
  }

  // Detectar nuevos dispositivos conectados
  int currentDevices = WiFi.softAPgetStationNum();
  if (currentDevices > numDevicesConnected) {
    Serial.println("Nuevo dispositivo conectado");
    numDevicesConnected = currentDevices;
  }

  delay(2000);
}
