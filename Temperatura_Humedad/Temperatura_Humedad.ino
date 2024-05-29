#include <Wire.h>
#include "SparkFun_SHTC3.h"
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "WIFI-DCI";
const char* password = "DComInf_2K24";
const char* mqttServer = "industrial.api.ubidots.com";
const int mqttPort = 1883;
const char* mqttUser = "BBUS-3rhwZg49UVNuTVzqPufQfqyxF6jyMe"; // Reemplaza con tu token de Ubidots
const char* mqttPassword = "";
const char* mqttTopic = "/v1.6/devices/Humedad_Temperatura"; // Reemplaza "RAK1" con el nombre de tu dispositivo en Ubidots

SHTC3 g_shtc3;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void errorDecoder(SHTC3_Status_TypeDef message) {
  switch (message) {
    case SHTC3_Status_Nominal:
      Serial.print("Nominal");
      break;
    case SHTC3_Status_Error:
      Serial.print("Error");
      break;
    case SHTC3_Status_CRC_Fail:
      Serial.print("CRC Fail");
      break;
    default:
      Serial.print("Unknown return code");
      break;
  }
}

void shtc3_read_data() {
  float Temperature = 0;
  float Humidity = 0;

  g_shtc3.update();
  if (g_shtc3.lastStatus == SHTC3_Status_Nominal) {
    Temperature = g_shtc3.toDegC();
    Humidity = g_shtc3.toPercent();

    Serial.print("RH = ");
    Serial.print(g_shtc3.toPercent());
    Serial.print("% (checksum: ");

    if (g_shtc3.passRHcrc) {
      Serial.print("pass");
    } else {
      Serial.print("fail");
    }

    Serial.print("), T = ");
    Serial.print(g_shtc3.toDegC());
    Serial.print(" deg C (checksum: ");

    if (g_shtc3.passTcrc) {
      Serial.print("pass");
    } else {
      Serial.print("fail");
    }
    Serial.println(")");
  } else {
    Serial.print("Update failed, error: ");
    errorDecoder(g_shtc3.lastStatus);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando al WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP obtenida: ");
  Serial.println(WiFi.localIP());

  // Conexión al broker MQTT
  client.setServer(mqttServer, mqttPort);

  Serial.print("Conectando al broker MQTT...");
  while (!client.connected()) {
    if (client.connect("Humedad_Temperatura", mqttUser, mqttPassword)) {
      Serial.println("Conectado al broker MQTT");
    } else {
      Serial.print("Error al conectar al broker MQTT: ");
      Serial.println(client.state());
      delay(2000);
    }
  }

  if (g_shtc3.begin() != SHTC3_Status_Nominal) {
    Serial.println("Error al inicializar el sensor SHTC3");
    while (1);
  }
}

void loop() {
  shtc3_read_data();

  float temperature = g_shtc3.toDegC();
  float humidity = g_shtc3.toPercent();

  // Construir el mensaje JSON para Ubidots
  String payload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";

  if (client.publish(mqttTopic, payload.c_str())) {
    Serial.println("Datos enviados correctamente: " + payload);
  } else {
    Serial.println("Error al enviar datos");
  }

  delay(5000); // Modifica si quieres cambiar la frecuencia de envío

  client.loop();
}
