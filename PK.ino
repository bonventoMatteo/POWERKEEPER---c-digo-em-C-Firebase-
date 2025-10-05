#include <WiFi.h>
#include "EmonLib.h"  // Biblioteca para o sensor de corrente

// Suas credenciais do WiFi
#define WIFI_SSID "Augusto"
#define WIFI_PASSWORD "internet100"

// Definições de Hardware
EnergyMonitor SCT013;
const int pinSCT = 34;  // Pino ADC onde o sensor está conectado
#define BOTAO 14
#define LED_220V 25
#define LED_110V 26
#define LED_OFF 27

// Variáveis globais
int tensao = 110;
int estadoBotaoAnterior = HIGH;
int contadorTensao = 0;
const double preco_kWh = 0.80;
double Irms = 0;
double potencia = 0;
double energia_Wh = 0;
double custo = 0;
unsigned long ultimoUpdate = 0;
unsigned long intervaloUpdate = 2000;
unsigned long tempoAnterior = 0;
unsigned long tempoDesligamentoVermelho = 0;
bool vermelhoDesligando = false;

// Protótipos
void alternarTensao();
void atualizarLEDs();

void setup() {
  Serial.begin(115200);

  // Configuração do Hardware
  SCT013.current(pinSCT, 1.45);
  pinMode(BOTAO, INPUT_PULLUP);
  pinMode(LED_220V, OUTPUT);
  pinMode(LED_110V, OUTPUT);
  pinMode(LED_OFF, OUTPUT);
  atualizarLEDs();

  // Conexão WiFi (apenas para exemplo, não usado no resto do código)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  tempoAnterior = millis();
  Serial.println("====== INICIANDO POWERKEEPER ======");
}

void loop() {
  unsigned long agora = millis();
  alternarTensao();

  if (vermelhoDesligando && (agora - tempoDesligamentoVermelho >= 10000)) {
    digitalWrite(LED_OFF, LOW);
    vermelhoDesligando = false;
    Serial.println("LED vermelho desligado.");
  }

  if (agora - ultimoUpdate >= intervaloUpdate) {
    ultimoUpdate = agora;

    // LEITURA REAL DO SENSOR
    Irms = SCT013.calcIrms(4096);
    if (Irms < 0.160) Irms = 0;

    unsigned long tempoAtual = millis();
    unsigned long deltaTempo = tempoAtual - tempoAnterior;
    tempoAnterior = tempoAtual;

    potencia = Irms * tensao;
    if (potencia > 0) {
      // Fórmula ajustada para calcular em Watt-hora (Wh)
      energia_Wh += (potencia * (deltaTempo / 3600000.0));
    }
    if (energia_Wh < 0) energia_Wh = 0;

    // Cálculo do custo agora converte Wh para kWh
    custo = (energia_Wh / 1000.0) * preco_kWh;

    Serial.printf("CONSOLE -> Tensão: %dV | I: %.3f A | P: %.2f W | E: %.3f Wh | Custo: R$ %.2f\n",
                  tensao, Irms, potencia, energia_Wh, custo);
  }
}

// Funções auxiliares
void alternarTensao() {
  int estadoBotaoAtual = digitalRead(BOTAO);
  if (estadoBotaoAnterior == HIGH && estadoBotaoAtual == LOW) {
    contadorTensao = (contadorTensao + 1) % 3;
    switch (contadorTensao) {
      case 0: tensao = 110; break;
      case 1: tensao = 220; break;
      case 2: tensao = 0; break;
    }
    atualizarLEDs();
    Serial.printf("\n--- Botão Pressionado -> Nova Tensão: %dV ---\n\n", tensao);
    vermelhoDesligando = (tensao == 0);
    if (vermelhoDesligando) tempoDesligamentoVermelho = millis();
    delay(300);
  }
  estadoBotaoAnterior = estadoBotaoAtual;
}

void atualizarLEDs() {
  digitalWrite(LED_220V, LOW);
  digitalWrite(LED_110V, LOW);
  digitalWrite(LED_OFF, LOW);
  switch (tensao) {
    case 110: digitalWrite(LED_110V, HIGH); break;
    case 220: digitalWrite(LED_220V, HIGH); break;
    case 0: digitalWrite(LED_OFF, HIGH); break;
  }
}
