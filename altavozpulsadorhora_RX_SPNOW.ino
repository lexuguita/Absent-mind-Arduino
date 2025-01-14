#include <WiFi.h>
#include <esp_now.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DFRobotDFPlayerMini.h"

// Configuración WiFi
const char* ssid = "Elrichard"; // Reemplaza con tu SSID
const char* password = "olaquetal123"; // Reemplaza con tu contraseña

// Configuración de NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // UTC+1, actualiza cada 60s

// Pines del DFPlayer
#define TX_PIN 17  // TX del DFPlayer a RX de la ESP32
#define RX_PIN 16  // RX del DFPlayer a TX de la ESP32

// Pin del botón
#define BOTON_PIN 18

// Instancia del DFPlayer Mini
HardwareSerial mySerial(1); // Usaremos el UART1 de la ESP32
DFRobotDFPlayerMini myDFPlayer;

// Variables de la alarma
int alarmaHora = -1;
int alarmaMinuto = -1;
bool alarmaActiva = false; // Indica si la alarma está configurada
bool condicionRecibida = false; // Indica si se recibió la señal del emisor

// Estructura para recibir datos ESP-NOW
typedef struct {
  int RSSIStatus; // Estado de la RSSI (E)
} struct_message;

struct_message receivedData; // Datos recibidos

// Callback para recibir datos ESP-NOW
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  int RSSIStatus;
  memcpy(&RSSIStatus, data, sizeof(RSSIStatus)); // Copia el dato recibido (entero)
  Serial.printf("Datos recibidos de MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  Serial.printf("RSSIStatus recibido: %d\n", RSSIStatus);

  // Verifica si la condición del emisor se cumple
 if(receivedData.RSSIStatus == 0)
 {
   condicionRecibida = true;
   
 }
}

void setup() {
  Serial.begin(115200);

  // Configura el botón
  pinMode(BOTON_PIN, INPUT_PULLUP); // Resistencia pull-up para el botón

  // Conexión a WiFi
  Serial.println("Conectando al WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Intentar conectarse durante 10 segundos
  unsigned long tiempoInicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - tiempoInicio < 10000) {
    Serial.print(".");
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi!");
  } else {
    Serial.println("\nNo se pudo conectar al WiFi. Verifica SSID y contraseña.");
  }

  // Inicia el cliente NTP
  timeClient.begin();

  // Configura el DFPlayer
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // UART1
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("No se pudo inicializar el DFPlayer Mini.");
    while (true);
  }
  Serial.println("DFPlayer Mini inicializado correctamente.");
  myDFPlayer.volume(10); // Ajusta el volumen (0-30)

  // Configuración de ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESP-NOW.");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Receptor ESP-NOW configurado.");
}

void loop() {
  // Espera a que se reciba la condición del emisor
  Serial.println(condicionRecibida);
  if (condicionRecibida==true) {
    Serial.println("señal recibida");
  }
  timeClient.update(); // Actualiza la hora desde NTP

  // Obtiene la hora actual
  int horaActual = timeClient.getHours();
  int minutoActual = timeClient.getMinutes();
  int segundoActual = timeClient.getSeconds();

  // Muestra la hora en el monitor serie
  Serial.printf("Hora actual: %02d:%02d:%02d\n", horaActual, minutoActual, segundoActual);

  // Verifica si la alarma está configurada
  if (alarmaActiva && horaActual == alarmaHora && minutoActual == alarmaMinuto) {
    Serial.println("¡Alarma activada! Presiona el botón para detener.");
    if (digitalRead(BOTON_PIN) == LOW) { // Si el botón está presionado
      Serial.println("Reproduciendo audio de alarma...");
      myDFPlayer.play(1); // Reproduce el archivo 0001.mp3
      delay(1000); // Anti-rebote
      alarmaActiva = false; // Desactiva la alarma
    }
  }

  // Configurar alarma desde el monitor serie
  if (Serial.available()) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim(); // Elimina espacios en blanco
    int separador = entrada.indexOf(':');
    if (separador != -1) {
      String hora = entrada.substring(0, separador);
      String minuto = entrada.substring(separador + 1);
      alarmaHora = hora.toInt();
      alarmaMinuto = minuto.toInt();
      alarmaActiva = true;
      Serial.printf("Alarma configurada para las %02d:%02d\n", alarmaHora, alarmaMinuto);
    } else {
      Serial.println("Formato incorrecto. Usa HH:MM");
    }
  }

  delay(1000); // Actualiza cada segundo

}
