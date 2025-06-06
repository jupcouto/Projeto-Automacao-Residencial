#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

// === OBJETOS ===
RTC_DS3231 rtc;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// === PINOS ===
#define pino A0       // Sensor de temperatura
#define bot1 7        // Botão -
#define bot2 8        // Botão +
#define bot3 10       // Botão de troca de tela
#define led 9         // LED indicador
#define motor 6       // Relé
#define lampada 13    // LED de status RTC (opcional)

// === VARIÁVEIS ===
float insTemp, calcTemp, setTemp = 25.0, range = 2.0, media[10];
int ctrl = 0, x = 0;
unsigned long delayLeitura = 0, delayLeitura2 = 0, delayClick = 0, blinkTime = 0;
bool ledState = false;
bool horarioAtivo = false;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  Wire.begin();
  rtc.begin();

  pinMode(bot1, INPUT_PULLUP);
  pinMode(bot2, INPUT_PULLUP);
  pinMode(bot3, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  pinMode(motor, OUTPUT);
  pinMode(lampada, OUTPUT);

  digitalWrite(motor, HIGH);  // Desliga motor
  digitalWrite(led, LOW);     // LED desligado

  // Inicializa array de média
  for (int i = 0; i < 10; i++) {
    media[i] = map(analogRead(pino), 20, 358, -40, 125);
  }
  calcTemp = calculaMedia();
}

void loop() {
  unsigned long tempoAtual = millis();

  DateTime agora = rtc.now();
  int hora = agora.hour();
  int minuto = agora.minute();

  // === VERIFICA HORÁRIO ATIVO ===
  if ((hora == 7 && minuto >= 0 && minuto < 30) || (hora == 15 && minuto >= 0 && minuto < 30)) {
    horarioAtivo = true;
    digitalWrite(lampada, HIGH);  // Sinaliza RTC ativo
  } else {
    horarioAtivo = false;
    digitalWrite(lampada, LOW);
  }

  // === LEITURA DA TEMPERATURA MÉDIA A CADA 10s ===
  if (tempoAtual - delayLeitura >= 10000) {
    media[x] = map(analogRead(pino), 20, 358, -40, 125);
    x = (x + 1) % 10;
    calcTemp = calculaMedia();
    delayLeitura = tempoAtual;
  }

  // === LEITURA INSTANTÂNEA A CADA 0.5s ===
  if (tempoAtual - delayLeitura2 >= 500) {
    insTemp = map(analogRead(pino), 20, 358, -40, 125);
    delayLeitura2 = tempoAtual;
  }

  // === CONTROLE DO MOTOR: TEMPERATURA + HORÁRIO ===
  if (calcTemp >= setTemp + range && horarioAtivo) {
    digitalWrite(motor, LOW);   // Liga motor
    digitalWrite(led, HIGH);
  } else if (calcTemp <= setTemp - range || !horarioAtivo) {
    digitalWrite(motor, HIGH);  // Desliga motor
    digitalWrite(led, LOW);
  }

  // === TROCA DE TELA COM BOT3 ===
  if (!digitalRead(bot3) && (tempoAtual - delayClick > 350)) {
    ctrl = (ctrl + 1) % 3;
    lcd.clear();
    delayClick = tempoAtual;
  }

  // === INTERFACE LCD ===
  if (ctrl == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Atual: ");
    lcd.print(calcTemp, 1);
    lcd.setCursor(0, 1);
    lcd.print("Set: ");
    lcd.print(setTemp, 1);
    lcd.setCursor(10, 1);
    lcd.print(hora);
    lcd.print(":");
    if (minuto < 10) lcd.print("0");
    lcd.print(minuto);

    if (!digitalRead(bot1) && (tempoAtual - delayClick > 350) && setTemp > 22.0) {
      setTemp -= 0.5;
      delayClick = tempoAtual;
    }
    if (!digitalRead(bot2) && (tempoAtual - delayClick > 350) && setTemp < 27.0) {
      setTemp += 0.5;
      delayClick = tempoAtual;
    }

  } else if (ctrl == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Tela Config.");
    lcd.setCursor(0, 1);
    lcd.print("Range ");
    lcd.print(range, 1);

    if (!digitalRead(bot1) && (tempoAtual - delayClick > 350) && range > 0.5) {
      range -= 0.5;
      delayClick = tempoAtual;
    }
    if (!digitalRead(bot2) && (tempoAtual - delayClick > 350) && range < 2.5) {
      range += 0.5;
      delayClick = tempoAtual;
    }

  } else if (ctrl == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Temp Inst.:");
    lcd.setCursor(0, 1);
    lcd.print(insTemp, 1);
  }

  // === PISCAR LED (Lâmpada) SEM DELAY ===
  if (tempoAtual - blinkTime >= 1200) {
    ledState = !ledState;
    digitalWrite(2, ledState);
    blinkTime = tempoAtual;
  }

  // === DEBUG SERIAL ===
  Serial.print("Temp Media: ");
  Serial.print(calcTemp);
  Serial.print(" | Set: ");
  Serial.print(setTemp);
  Serial.print(" | Range: ");
  Serial.print(range);
  Serial.print(" | Motor: ");
  Serial.print(digitalRead(motor));
  Serial.print(" | Hora: ");
  Serial.print(hora);
  Serial.print(":");
  Serial.println(minuto);
}

// === FUNÇÃO DE MÉDIA ===
float calculaMedia() {
  float soma = 0;
  for (int i = 0; i < 10; i++) soma += media[i];
  return soma / 10.0;
}