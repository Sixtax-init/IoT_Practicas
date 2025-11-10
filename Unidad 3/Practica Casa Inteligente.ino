#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// ==== CONFIGURACIÓN WiFi ====
const char* ssid = "Redmi Note 11S";
const char* password = "Brooklyn99";

// ==== SERVIDOR WEB ====
ESP8266WebServer server(80);

// ==== LEDS ====
#define LED_ROJO D1
#define LED_VERDE D2
#define LED_DETECCION D3

bool ledRojo = false;
bool ledVerde = false;
bool ledDeteccion = false;

// ==== SERVOS Y SENSOR ====
#define SERVO_AUTO D5
#define TRIG_PIN D6
#define ECHO_PIN D7
#define SERVO_WEB D8

Servo servoAuto;
Servo servoWeb;

long duracion;
int distancia;
unsigned long ultimaDeteccion = 0;
bool puertaAbierta = false;
bool servoWebAbierto = false;

// ==== PÁGINA HTML INCRUSTADA ====
String paginaHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Panel de Control Inteligente</title>
<style>
body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
  color: #f8fafc;
  padding: 20px;
  text-align: center;
}
h1 {
  font-size: 2.5rem;
  margin-bottom: 10px;
  background: linear-gradient(45deg, #06b6d4, #f97316);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}
h2 {
  color: #94a3b8;
  margin-bottom: 10px;
}
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 20px;
  margin-top: 30px;
}
.card {
  background: rgba(30, 41, 59, 0.85);
  border: 2px solid #334155;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 0 20px rgba(6, 182, 212, 0.2);
  transition: all 0.3s ease;
}
.card:hover {
  transform: translateY(-5px);
  border-color: #06b6d4;
}
.led-icon {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  margin: 0 auto 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.5rem;
  transition: all 0.3s ease;
}
.led-icon.off {
  background: #475569;
  color: #94a3b8;
}
.led-icon.red {
  background: #ef4444;
  box-shadow: 0 0 20px rgba(239,68,68,0.6);
}
.led-icon.green {
  background: #22c55e;
  box-shadow: 0 0 20px rgba(34,197,94,0.6);
}
.led-icon.builtin {
  background: #f97316;
  box-shadow: 0 0 20px rgba(249,115,22,0.6);
}
button {
  background: linear-gradient(45deg, #06b6d4, #0891b2);
  border: none;
  color: white;
  padding: 12px 24px;
  border-radius: 8px;
  font-size: 1rem;
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.3s ease;
}
button:hover {
  transform: scale(1.05);
  box-shadow: 0 0 20px rgba(6,182,212,0.4);
}
.status {
  color: #94a3b8;
  margin-bottom: 8px;
}
footer {
  margin-top: 40px;
  font-size: 0.9rem;
  color: #64748b;
}
</style>
</head>
<body>
  <h1>Panel de Control Inteligente</h1>
  <p>IP del sistema: %IP%</p>

  <h2>💡 Control de LEDs</h2>
  <div class="grid">
    <div class="card">
      <div class="led-icon %LED1_CLASS%">🔴</div>
      <h3>LED Rojo</h3>
      <p class="status">Estado: %LED1_STATUS%</p>
      <form action="/toggle/1" method="get">
        <button>%LED1_BUTTON%</button>
      </form>
    </div>

    <div class="card">
      <div class="led-icon %LED2_CLASS%">🟢</div>
      <h3>LED Verde</h3>
      <p class="status">Estado: %LED2_STATUS%</p>
      <form action="/toggle/2" method="get">
        <button>%LED2_BUTTON%</button>
      </form>
    </div>

    <div class="card">
      <div class="led-icon %LED3_CLASS%">🟠</div>
      <h3>LED Rojo</h3>
      <p class="status">Estado: %LED3_STATUS%</p>
      <form action="/toggle/3" method="get">
        <button>%LED3_BUTTON%</button>
      </form>
    </div>
  </div>

  <h2>⚙ Control de Servos</h2>
  <div class="grid">
    <div class="card">
      <h3>Puerta Automática (Sensor)</h3>
      <p>Se abre al detectar presencia y se cierra después de 5s.</p>
    </div>

    <div class="card">
      <h3>Puerta Manual (Web)</h3>
      <p class="status">Estado: %SERVO_ESTADO%</p>
      <form action="/toggleServoWeb" method="get">
        <button>%SERVO_BOTON%</button>
      </form>
    </div>
  </div>

  <footer>
    Sistema en línea 🛰 | NodeMCU ESP8266
  </footer>
</body>
</html>
)rawliteral";

// ==== FUNCIONES ====
int medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duracion = pulseIn(ECHO_PIN, HIGH, 30000);
   if (duracion == 0) {
    Serial.println("⚠ Sensor no detecta eco. Revisa conexiones o alimentación.");
    return -1; // valor inválido
  }
  distancia = duracion * 0.034 / 2;

  Serial.print("📏 Distancia medida: ");
  Serial.print(distancia);
  Serial.println(" cm");

  return distancia;
}

void handleRoot() {
  String html = paginaHTML;
  html.replace("%IP%", WiFi.localIP().toString());

  html.replace("%LED1_STATUS%", ledRojo ? "Encendido" : "Apagado");
  html.replace("%LED1_BUTTON%", ledRojo ? "Apagar" : "Encender");
  html.replace("%LED1_CLASS%", ledRojo ? "active red" : "off");

  html.replace("%LED2_STATUS%", ledVerde ? "Encendido" : "Apagado");
  html.replace("%LED2_BUTTON%", ledVerde ? "Apagar" : "Encender");
  html.replace("%LED2_CLASS%", ledVerde ? "active green" : "off");

  html.replace("%LED3_STATUS%", ledDeteccion ? "Encendido" : "Apagado");
  html.replace("%LED3_BUTTON%", ledDeteccion ? "Apagar" : "Encender");
  html.replace("%LED3_CLASS%", ledDeteccion ? "active builtin" : "off");

  html.replace("%SERVO_ESTADO%", servoWebAbierto ? "Cerrada" : "Abierta");
  html.replace("%SERVO_BOTON%", servoWebAbierto ? "Abrir" : "Cerrar");

  server.send(200, "text/html", html);
}

void handleToggleLED1() {
  ledRojo = !ledRojo;
  digitalWrite(LED_ROJO, ledRojo ? HIGH : LOW);
  handleRoot();
}
void handleToggleLED2() {
  ledVerde = !ledVerde;
  digitalWrite(LED_VERDE, ledVerde ? HIGH : LOW);
  handleRoot();
}
void handleToggleLED3() {
  ledDeteccion = !ledDeteccion;
  digitalWrite(LED_DETECCION, ledDeteccion ? HIGH : LOW);
  handleRoot();
}
void handleToggleServoWeb() {
  servoWebAbierto = !servoWebAbierto;
  servoWeb.write(servoWebAbierto ? 180 : 0);
  handleRoot();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando sistema...");

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_DETECCION, OUTPUT);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_DETECCION, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  servoAuto.attach(SERVO_AUTO);
  servoWeb.attach(SERVO_WEB);
  servoAuto.write(0);
  servoWeb.write(0);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle/1", handleToggleLED1);
  server.on("/toggle/2", handleToggleLED2);
  server.on("/toggle/3", handleToggleLED3);
  server.on("/toggleServoWeb", handleToggleServoWeb);

  server.begin();
  Serial.println("Servidor iniciado en puerto 80");
}

void loop() {
  server.handleClient();

  int d = medirDistancia();

  // 🔹 SOLO controla el servomotor automático, NO el LED
  if (d > 0 && d < 15) {
    if (!puertaAbierta) {
      Serial.println("🚪 Objeto detectado. Abriendo puerta automática.");
      servoAuto.write(180);
      puertaAbierta = true;
      ultimaDeteccion = millis();
    }
  }

  // 🔹 Cerrar la puerta después de 5 segundos
  if (puertaAbierta && (millis() - ultimaDeteccion > 5000)) {
    Serial.println("⏳ Tiempo agotado. Cerrando puerta automática.");
    servoAuto.write(0);
    puertaAbierta = false;
  }
}