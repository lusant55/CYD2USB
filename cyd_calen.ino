#include <TFT_eSPI.h>
////#include <ESP32Time.h>
#include "DHT.h"
#include <SD.h>
#include <SPI.h>
////#include <NTPClient.h>  //https://github.com/taranais/NTPClient
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include <esp_wifi.h>
//#include <HTTPClient.h>
//HTTPClient http;
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include "feriados.h"
#include "auxtime.h"
#include "datalog.h"
#include "graph.h"
#include "alarms.h"
#include "alarm_storage.h"

#define TFT_BLL 21
  static int modoescuro= 0;


#define CS_SD 5  // no CYD2USB normalmente é o GPIO5
File root;

#define DHTPIN 27  // DHT11 on GPIO 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define WIFI_SSID "TP-LINK_7F0A"
#define WIFI_PWD "152459150"


// ---- Config NTP ----
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;   // offset fuso horário (ex: +3600 para GMT+1)
const int daylightOffset_sec = 0;  // horário de verão se aplicável
int connected = 0;
int synced = 0;



int digpos = 102;
int digsize = 12;
int blinktime = 0;

int iDigitPos[8] = { digpos + digsize, digpos + 2 * digsize, digpos + 3 * digsize, digpos + 4 * digsize, digpos + 5 * digsize, digpos + 6 * digsize, digpos + 7 * digsize, digpos + 8 * digsize };
//int iDigitPos[8] = {10, 60, 110, 160, 210, 260, 310, 360};
int iStartY = 190;  // altura da linha
bool changeall = 1;

// === DISPLAY ===
TFT_eSPI tft = TFT_eSPI();
#define FONT FreeSansBold9pt7b  //FreeMono9pt7b //DSEG7 //FreeSansBold24pt7b // podes trocar por FSS12, DSEG7, etc

// === TOUCH XPT2046 (bit-banging) ===
#define T_CLK 25
#define T_CS 33
#define T_DIN 32
#define T_DO 39
#define T_IRQ 36  // ativo em LOW

// === CALENDÁRIO ===
const char *diasSemana[7] = { "S", "T", "Q", "Q", "S", "S", "D" };
const char *nomesMes[12] = {
  "Janeiro", "Fevereiro", "Marco", "Abril", "Maio", "Junho",
  "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"
};
tm timeinfo;
void setInitialTime(void);
int mesAtual = 9;
int anoAtual = 2025;
int diaAtual = 1;
int mesAtualG = 9;
int anoAtualG = 2025;
int diaAtualG = 1;

bool sdcardAvailable = false;

// =====================================================
// Funções Touch (bit-banging)
// =====================================================
void sendCommand(uint8_t cmd) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(T_CLK, LOW);
    digitalWrite(T_DIN, (cmd >> i) & 1);
    digitalWrite(T_CLK, HIGH);
  }
}

uint16_t readData(uint8_t cmd) {
  uint16_t val = 0;

  digitalWrite(T_CS, LOW);

  sendCommand(cmd);

  for (int i = 11; i >= 0; i--) {
    digitalWrite(T_CLK, HIGH);
    digitalWrite(T_CLK, LOW);
    val |= (digitalRead(T_DO) << i);
  }

  digitalWrite(T_CS, HIGH);
  return val;
}

bool touchGetPoint(int &x, int &y) {
  if (digitalRead(T_IRQ) == HIGH) {
    return false;  // sem toque
  }

  // duas leituras para estabilizar
  uint16_t rawX1 = readData(0x90);
  uint16_t rawY1 = readData(0xD0);
  uint16_t rawX2 = readData(0x90);
  uint16_t rawY2 = readData(0xD0);

  uint16_t rawX = (rawX1 + rawX2) / 2;
  uint16_t rawY = (rawY1 + rawY2) / 2;
  //Serial.printf("rawX: %d\n", rawX);
  //Serial.printf("rawY: %d\n", rawY);

  // valida faixa 
  if (rawX < 100 || rawX > 4000 || rawY < 100 || rawY > 4000) {
    return false;
  }

  // mapeia para tela 320x240
  //  x = map(rawX, 320, 3900, 0, tft.width());
  //  y = map(rawY, 240, 3900, 0, tft.height());
  x = map(rawX, 140, 1918, 0, tft.width());
  y = map(rawY, 149, 1863, 0, tft.height());
  Serial.printf("Touch detectado: X=%d Y=%d\n", x, y);
  delay(200);
  return true;
}


void desenharCalendario(int mes, int ano) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // título
  tft.fillRect(0, 0, 320, 30, TFT_BLUE);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString(String(nomesMes[mes]) + " " + String(ano), tft.width() / 2, 10);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // botões
  tft.fillRect(0, 0, 70, 30, TFT_BLUE);
  tft.setTextDatum(CC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("<<", 20, 15);
  tft.drawString("<", 50, 15);

  tft.fillRect(tft.width() - 70, 0, 70, 30, TFT_BLUE);
  tft.drawString(">>", tft.width() - 20, 15);
  tft.drawString(">", tft.width() - 50, 15);

  // cabeçalho
  int x0 = 52;
  int y0 = 32;
  int colWidth = 35;
  int rowHeight = 22;
  tft.setTextDatum(TL_DATUM);
  // grafico e alarmes
  // tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  //if (sdcardAvailable) 
  {
    tft.drawString("Graph", 0, 193);
  }
  tft.drawString("Alarm", 260, 193);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);




  //tft.setTextDatum(CC_DATUM);
  for (int i = 0; i < 7; i++) {
    tft.drawString(diasSemana[i], x0 + i * colWidth, y0);
  }

  // dias
  int startDay = diaDaSemana(1, mes, ano);
  int totalDias = diasNoMes(mes, ano);

  int linha = 1;
  int coluna = startDay;
  for (int d = 1; d <= totalDias; d++) {
    const char *sigla = feriadoSigla(d, mes);

    int x = x0 + coluna * colWidth;
    int y = y0 + linha * rowHeight;

    bool hoje = (d == timeinfo.tm_mday && mes == timeinfo.tm_mon && ano == timeinfo.tm_year + 1900);

    // --- Realce do dia atual ---
    if (hoje) {
      tft.setTextColor(TFT_BLACK, TFT_WHITE);  // texto preto sobre branco
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);  // estilo normal
    }

    // --- Conteúdo do dia ---
    if (sigla) {
      // feriado → sigla
      if (!hoje)
        tft.setTextColor(TFT_RED, TFT_BLACK);  // se não for hoje, fica vermelho
      else
        tft.setTextColor(TFT_RED, TFT_WHITE);  // se não for hoje, fica vermelho

      tft.drawString(sigla, x, y);
    } else {
      // dia normal → número
      char buf[4];
      sprintf(buf, "%d", d);
      int espacoextra = (d > 9) ? -7 : 0;
      tft.drawString(buf, x + espacoextra, y);
      //Serial.printf("x + espacoextra %d, y %d\n", x + espacoextra, y);
    }

    // avança coluna/linha
    coluna++;
    if (coluna > 6) {
      coluna = 0;
      linha++;
    }
  }
}

void DisplayTime(void) {
  static int lastSecond = -1;
  static char szOldTime[9] = "--------";
  static bool reversed = 0;
  char szTemp[2];
  char szTime[9];
  int foregr = TFT_WHITE;
  int backgr = TFT_BLACK;
  //Serial.println("entrou em DisplayTime");

  //struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("DisplayTime Falha ao obter tempo via NTP");
    while (1)
      ;
  }
  sprintf(szTime, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Extrai o segundo para controlo do update
  int iSec = timeinfo.tm_sec;

  if ((iSec != lastSecond) || changeall == 1) 
  {
    if (timeinfo.tm_mday != diaAtual) 
      {
        changeall = 1;
        mesAtual = timeinfo.tm_mon;
        anoAtual = timeinfo.tm_year + 1900;
        diaAtual = timeinfo.tm_mday;
        prepararFeriadosMes(anoAtual, mesAtual);
        desenharCalendario(mesAtual, anoAtual);
      }

  
  
    lastSecond = iSec;


    szTemp[1] = 0;

    //tft.setFreeFont(&FONT);

    // Atualiza só dígitos que mudaram
    for (int i = 0; i < 8; i++) {
      if ((szTime[i] != szOldTime[i]) || changeall == 1) {
        // Apaga antigo caractere
        szTemp[0] = szOldTime[i];
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawString(szTemp, iDigitPos[i], iStartY);

        // Desenha novo caractere
        szTemp[0] = szTime[i];
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(szTemp, iDigitPos[i], iStartY);
      }
    }

    // Guarda hora atual para próxima comparação
    strcpy(szOldTime, szTime);
  }
}


// =====================================================
// Setup / Loop
// =====================================================
void setup() {

  int ntries = 0;
  //  struct tm timeinfo;
  int tx, ty;
  WiFiManager wm;
  Serial.begin(115200);
  Serial.println("=== Calendário com Touch (IRQ) ===");

  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  // touch
  pinMode(T_CLK, OUTPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(T_DIN, OUTPUT);
  pinMode(T_DO, INPUT);
  pinMode(T_IRQ, INPUT);
  digitalWrite(T_CS, HIGH);

  
  // display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  // pinMode(TFT_BLL, OUTPUT);
  // digitalWrite(TFT_BLL, HIGH);

  while (1) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    tft.drawString("Choose an option", 70, 20);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("1-Start last AP connection", 10, 80);
    tft.drawString("2-Adj. date/time manually", 10, 110);
    tft.drawString("3-Use AP mode to connect", 10, 140);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    while (!touchGetPoint(tx, ty))
      ;

    Serial.printf("Touch detectado: X=%d Y=%d\n", tx, ty);
    //continue;
    if (tx > 0 && tx < 320 && ty > 80 && ty < 98) {
      tft.fillScreen(TFT_BLACK);
      tft.drawString("Connecting ...", 20, 80);

      // Serial.println("rede anterior");
      // continue;

      WiFi.begin();
      while ((WiFi.status() != WL_CONNECTED) && (ntries++ < 3)) {
        delay(500);
        Serial.println("Connecting Wifi");
      }
      if (WiFi.status() == WL_CONNECTED) {
        connected = 1;
        // ---- Inicializa NTP ----
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        if (!(synced = getLocalTime(&timeinfo))) {
          tft.fillScreen(TFT_BLACK);
          tft.drawString("getLocalTime error", 20, 100);
          tft.drawString("Touch display to restart", 20, 140);
          Serial.println("Falha ao obter tempo via NTP");
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          while (!touchGetPoint(tx, ty))
            ;
          continue;
        }
        break;
      } else {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Connection error", 20, 100);
        tft.drawString("Touch display to restart", 20, 140);
        Serial.println("Falha ao obter tempo via NTP");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        while (!touchGetPoint(tx, ty))
          ;
        continue;
      }
    }
    if (tx > 0 && tx < 320 && ty > 110 && ty < 128) {
      // Serial.println("acerto manual");
      // continue;
      setInitialTime();
      break;
    }
    if (tx > 0 && tx < 320 && ty > 140 && ty < 158) {
      tft.fillScreen(TFT_BLACK);
      tft.drawString("Waiting Connection", 20, 80);
      tft.drawString("AP - ESP32_ConfigAP", 20, 120);
      // Serial.println("modo AP");
      // continue;

      wm.setConnectTimeout(60);        // tempo máximo para tentar conectar (segundos)
      wm.setConfigPortalTimeout(120);  // portal expira em 2 minutos
      wm.startConfigPortal("ESP32_ConfigAP");

      if (WiFi.status() == WL_CONNECTED) {
        connected = 1;
        // ---- Inicializa NTP ----
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        if (!(synced = getLocalTime(&timeinfo))) {
          tft.fillScreen(TFT_BLACK);
          tft.drawString("getLocalTime error", 20, 100);
          tft.drawString("Touch display to restart", 20, 140);
          Serial.println("Falha ao obter tempo via NTP");
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          while (!touchGetPoint(tx, ty))
            ;
          continue;
        }
        break;
      } else {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Connection error", 20, 100);
        tft.drawString("Touch display to restart", 20, 140);
        Serial.println("Falha ao obter tempo via NTP");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        while (!touchGetPoint(tx, ty))
          ;
        continue;
      }
    }
  }


  mesAtual = timeinfo.tm_mon;
  anoAtual = timeinfo.tm_year + 1900;
  diaAtual = timeinfo.tm_mday;

  mesAtualG = timeinfo.tm_mon;
  anoAtualG = timeinfo.tm_year + 1900;
  diaAtualG = timeinfo.tm_mday;

  // Serial.println(timeinfo.tm_year);
  // Serial.println(timeinfo.tm_mon);
  // Serial.println(timeinfo.tm_mday);
  // Serial.println(timeinfo.tm_hour);
  // Serial.println(timeinfo.tm_min);
  // Serial.println(timeinfo.tm_sec);


  dht.begin();

  // ==== Inicializa SD Card ====

  // if (!SD.begin(CS_SD)) {
  //   Serial.println("Falha ao montar SD!");
  // } else {
  //   Serial.println("SD montado com sucesso.");
  //   sdcardAvailable = true;
  //   openLogFile(timeinfo);

  //   root = SD.open("/");
  //   while (true) {
  //     File entry = root.openNextFile();
  //     if (!entry) {
  //       root.seek(0);
  //       break;
  //     }
  //     Serial.println(entry.name());
  //     entry.close();
  //   }
  // }
  sdcardAvailable = true;
  prepararFeriadosMes(anoAtual, mesAtual);
  desenharCalendario(mesAtual, anoAtual);
  alarmsLoad();
  buzzerInit();
}

enum Touch { invalidTouch, incrementYear, incrementMonth, decrementYear, decrementMonth, gotoCalState, gotoGrapfState, gotoAlarmState, gdecrementDay, gdecrementWeek,
             gdecrementMonth, gincrementDay, gincrementWeek, gincrementMonth, geXit, altouch };
enum FundMode { calState, grapfState, alarmState, alarmState1 };

int decodeTouch(int tx, int ty, int state) {
  switch (state) {
    case calState:
      if (ty < 30 && tx < 30)
        return decrementYear;
      if (tx > 40 && tx < 60 && ty < 30)
        return decrementMonth;
      if (tx > 250 && tx < 280 && ty < 30)
        return incrementMonth;
      if (ty < 30 && tx > 290)
        return incrementYear;
      if (tx > 0 && tx < 50 && ty > 200 && ty < 220)  // <
        return gotoGrapfState;
      if (tx > 258 && tx < 320 && ty > 200 && ty < 220)  // <
        return gotoAlarmState;
      break;
    case grapfState:
      if (tx > 95 && tx < 112 && ty > 10 && ty < 24)  // <
        return gdecrementDay;
      if (tx > 71 && tx < 82 && ty > 9 && ty < 22)  // <<
        return gdecrementWeek;
      if (tx > 38 && tx < 57 && ty > 12 && ty < 24)  // <<<
        return gdecrementMonth;
      if (tx > 209 && tx < 225 && ty > 10 && ty < 22)  // >
        return gincrementDay;
      if (tx > 241 && tx < 252 && ty > 9 && ty < 23)  // >>
        return gincrementWeek;
      if (tx > 266 && tx < 279 && ty > 10 && ty < 22)  // >>>
        return gincrementMonth;
      if (tx > 280 && tx < 320 && ty > 0 && ty < 10)  // X
        return geXit;
    case alarmState:
    case alarmState1: return altouch;
    default: return invalidTouch;
  }

  return invalidTouch;
}




void showTempHum(void) 
  {
    float humidity = dht.readHumidity();        // Read temperature
    float temperature= dht.readTemperature();  // Read humidity
    if (isnan(humidity) || isnan(temperature))  // If error
      {
        Serial.println("Failed to read from DHT11");
      } 
    else 
      {
        String tem = "Temp " + String(temperature) + "C";
        String hum = "Hum " + String(humidity) + "%";
        tft.drawString(tem, 20, 220);
        tft.drawString(hum, 180, 220);
      }
  
  }



void loop() {
  int tx, ty;
  static int old_min = -1;
  static int old_min1 = -1;
  static int old_min2 = -1;
  int touch = invalidTouch;
  static int state = calState;
  static int laststate = calState;
  static bool buzzerON= false;
  static bool alarmON= false;
  static int old_sec= -1;
  static long int runningtime= millis();

  if (touchGetPoint(tx, ty)) 
  {
    //Serial.println("passou 5s");
    changeall = 1;
    runningtime= millis();  
    digitalWrite (TFT_BLL, 1);
    modoescuro= 0;


    switch (touch = decodeTouch(tx, ty, state)) {
      case decrementYear:
        --anoAtual;
        break;
      case decrementMonth:
        --mesAtual;
        if (mesAtual < 0) {
          mesAtual = 11;
          --anoAtual;
        }
        break;
      case incrementMonth:
        ++mesAtual;
        if (mesAtual > 11) {
          mesAtual = 0;
          --anoAtual;
        }
        break;
      case incrementYear:
        ++anoAtual;
        break;
      case gotoCalState:
        laststate = state;
        state = calState;

        break;
      case gotoGrapfState:
        laststate = state;
        state = grapfState;
        break;
      case gotoAlarmState:
        laststate = state;
        state = alarmState;
        break;
      case gdecrementDay:
        decDia(&diaAtualG, &mesAtualG, &anoAtualG);
        break;
      case gdecrementWeek:
        for (int ii = 0; ii < 7; ++ii)
          decDia(&diaAtualG, &mesAtualG, &anoAtualG);
        break;
      case gdecrementMonth:
        --mesAtualG;
        if (mesAtualG < 0) {
          mesAtualG = 11;
          --anoAtualG;
        }
        break;
      case gincrementDay:
        incDia(&diaAtualG, &mesAtualG, &anoAtualG);
        break;
      case gincrementWeek:
        for (int ii = 0; ii < 7; ++ii)
          incDia(&diaAtualG, &mesAtualG, &anoAtualG);
        break;
      case gincrementMonth:
        ++mesAtualG;
        if (mesAtualG > 11) {
          mesAtualG = 0;
          ++anoAtualG;
        }
        break;
      case geXit:
        state = calState;
        break;
      case invalidTouch:
      default: break;
    }
    if (alarmON)
    {
      //Serial.println("passei");
      alarmON= false;
      buzzerStop(); 
      buzzerON= false;
    }

  }
  // Serial.println("passei");
  switch (state) {
    case calState:
      if (touch == decrementYear || touch == decrementMonth || touch == incrementMonth || touch == incrementYear || touch == geXit || ((changeall == 1) && (laststate == alarmState))) 
      {
        prepararFeriadosMes(anoAtual, mesAtual);
        desenharCalendario(mesAtual, anoAtual);
      }
      //getLocalTime(&timeinfo);
      DisplayTime();
      if (((timeinfo.tm_min % 2) && (timeinfo.tm_min != old_min1))|| (changeall == 1)) {
        old_min1= timeinfo.tm_min;
        showTempHum();
      }
      changeall = 0;
      break;
    case grapfState:
      if (touch != invalidTouch) {
        // Serial.println(anoAtualG);
        // Serial.println(mesAtualG + 1);
        // Serial.println(diaAtualG);
        drawDayGraph(anoAtualG, mesAtualG + 1, diaAtualG);
      }
      break;
    case alarmState:
      alarmUI_enter();
      state = alarmState1;
      break;
    case alarmState1:
      if (touch != invalidTouch) {
        if (alarmUI_handleTouch(tx, ty)) {
          state = calState;
          laststate = alarmState;
          changeall = 1;
        }
        break;
        default: break;
      }
  }

  if (timeinfo.tm_min != old_min2)
  {
    old_min2= timeinfo.tm_min;
    if (alarmsCheckAndBuzz(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, (timeinfo.tm_wday + 6) % 7))  
      alarmON= true;
      //digitalWrite (TFT_BLL, 1);
  }

  if (alarmON && timeinfo.tm_sec != old_sec)
    {
      old_sec= timeinfo.tm_sec;
      if (buzzerON)
      {
        buzzerStop(); 
        buzzerON= false;
      }
      else
        {
          buzzerStart(1000);
          buzzerON= true;
        }
    }
  // if (timeinfo.tm_sec != old_sec)
  //   {
  //     Serial.print(digitalRead(21));
  //     old_sec= timeinfo.tm_sec;
  //   }



  touch = invalidTouch;
  if (state != calState)
    getLocalTime(&timeinfo);
  if (((timeinfo.tm_min % 5) == 0) && (timeinfo.tm_min != old_min) && sdcardAvailable) {
    old_min = timeinfo.tm_min;
    float humidity = dht.readHumidity();        // Read temperature
    float temperature= dht.readTemperature();  // Read humidity
    if (isnan(humidity) || isnan(temperature))  // If error
    {
      Serial.println("Failed to read from DHT11");
    } else {
      if (!writelog(temperature, humidity, timeinfo))
        ;
    }
  }
long int mils= millis();
char num[20];
  if (mils - runningtime > 1000 * 60 * 2)
    {
      // Serial.println("");
      // sprintf (num, "%ld ",mils);
      // Serial.print(mils);
      // sprintf (num, " %ld ",runningtime);
      // Serial.print(num);
      // sprintf (num, " %ld ",mils - runningtime);
      // Serial.println(num);
      runningtime= millis();
      digitalWrite (TFT_BLL, 0);
      modoescuro= 1;


    }

  //changeall = 0;
  delay(200);
  
}

