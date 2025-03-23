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

// --- Configuración de WiFi ---
const char* ssid = "Sensor1";      // Nombre del punto de acceso (SSID)
const char* password = "123456789"; // Contraseña del punto de acceso

// --- Instancia del servidor ---
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

  // Inicialización de WiFi en modo AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP iniciado");
  Serial.print("Dirección IP del ESP32: ");
  Serial.println(WiFi.softAPIP());

  // Mostrar IP del ESP32 en la LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: ");
  lcd.print(WiFi.softAPIP());

  // --- Rutas con CORS ---
  
  server.on("/api/distancia", HTTP_GET, [](AsyncWebServerRequest *request){
    float distance = getDistance();
    String json = "{\"distancia\": " + String(distance) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    
    // Habilitar CORS
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    request->send(response);
  });

  server.on("/api/temperatura", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperatura = dht.readTemperature();
    if (isnan(temperatura)) temperatura = 0;
    
    String json = "{\"temperatura\": " + String(temperatura) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    
    // Habilitar CORS
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    request->send(response);
  });

  server.on("/api/humedad", HTTP_GET, [](AsyncWebServerRequest *request){
    float humedad = dht.readHumidity();
    if (isnan(humedad)) humedad = 0;

    String json = "{\"humedad\": " + String(humedad) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);

    // Habilitar CORS
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    request->send(response);
  });

  // Página principal HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>Datos de Sensores</h1>";
    html += "<p><a href='/api/distancia'>Ver Distancia</a></p>";
    html += "<p><a href='/api/temperatura'>Ver Temperatura</a></p>";
    html += "<p><a href='/api/humedad'>Ver Humedad</a></p>";
    html += "</body></html>";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);

    // Habilitar CORS
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");

    request->send(response);
  });

  server.begin();
}

// --- Función para medir la distancia ---
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

// --- Función para enviar datos al servidor ---
void sendDataToServer() {
  if (WiFi.softAPgetStationNum() > 0) {   // Solo enviar si hay clientes conectados
    WiFiClient client;
    HTTPClient http;

    http.begin(client, "http://192.168.4.1:3001/api/datos");  // IP del servidor
    http.addHeader("Content-Type", "application/json");

    float distancia = getDistance();
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    String ip = WiFi.softAPIP().toString();

    // Verificación para evitar datos NaN
    if (isnan(temperatura)) temperatura = 0;
    if (isnan(humedad)) humedad = 0;
    if (isnan(distancia)) distancia = 0;

    // Crear JSON
    String json = "{\"distancia\": " + String(distancia) + 
                  ", \"temperatura\": " + String(temperatura) + 
                  ", \"humedad\": " + String(humedad) + 
                  ", \"ip\": \"" + ip + "\"}";

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
  // Mostrar temperatura, humedad y distancia en la LCD
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

  delay(2000);  // Actualiza la pantalla cada 2 segundos

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distancia);
  lcd.print(" cm");

  // Verificar si han pasado 5 segundos para enviar los datos
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
