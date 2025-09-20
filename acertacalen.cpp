#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>

extern TFT_eSPI tft;
extern tm timeinfo;
// extern int mesAtual;
// extern int anoAtual;
bool touchGetPoint(int &x, int &y);

// Enum para identificar campos
enum FieldType { YEAR,
                 MONTH,
                 DAY,
                 HOUR,
                 MIN };

// Estrutura de campo
struct Field {
  FieldType type;
  int *value;
  int minVal;
  int maxVal;
  int x;
  int y;
};

// Estrutura de cada dígito desenhado
struct DigitArea {
  FieldType fieldType;
  int digitIndex;  // posição dentro do número
  int x, y, w, h;
};

Field fields[5];
DigitArea digitAreas[20];  // max 20 dígitos
int digitCount = 0;


int daysInMonth(int month, int year) {
  month += 1;  // 0..11 -> 1..12
  if (month == 2) return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 29 : 28;
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

// // --- a função simples e correta: somar place e fazer wrap no intervalo ---
// int incrementDigitField(int value, int digitIndex, int width, int minValue, int maxValue) {
//   // calcular place = 10^(width-1-digitIndex)
//   int place = 1;
//   int steps = (width - 1 - digitIndex);

//   Serial.println("digit");
//   Serial.println(digitIndex);
//   Serial.println("valor antes");
//   Serial.println(value);

//   for (int i = 0; i < steps; ++i) place *= 10;

//   int range = maxValue - minValue + 1;
//   // offset dentro do intervalo
//   int offset = ((value - minValue) + place) % range;
//   if (offset < 0) offset += range;
//   int newValue = minValue + offset;
//   Serial.println("valor apos");
//   Serial.println(newValue);
//   return newValue;
// }
int incrementDigitField(int value, int digitIndex, int width, int minValue, int maxValue) {
  // calcular 10^(width-1-digitIndex) sem floats
  int place = 1;
  Serial.println("digit");
  Serial.println(digitIndex);
  Serial.println("valor antes");
  Serial.println(value);

  for (int i = 0; i < (width - 1 - digitIndex); i++) place *= 10;

  int newValue = value + place;
  if (newValue > maxValue) {
    // volta ao início da gama
    newValue = minValue;  // + (newValue - maxValue - 1);
  }
  Serial.println("valor apos");
  Serial.println(newValue);
  return newValue;
}
// desenhar campo com caret '^' 8px acima e registar áreas de toque
void drawField(Field &f) {
  String label;
  char buf[6];  // suficiente para "%04d\0"

  switch (f.type) {
    case YEAR:
      label = "Year:";
      sprintf(buf, "%04d", *f.value + 2017);
      break;
    case MONTH:
      label = "Month:";
      sprintf(buf, "%02d", *f.value + 1);
      break;
    case DAY:
      label = "Day:";
      sprintf(buf, "%02d", *f.value);
      break;
    case HOUR:
      label = "Hour:";
      sprintf(buf, "%02d", *f.value);
      break;
    case MIN:
      label = "Min:";
      sprintf(buf, "%02d", *f.value);
      break;
  }

  tft.drawString(label, f.x, f.y);

  int dx = f.x + 100;
  int dy = f.y;

  for (int i = 0; i < (int)strlen(buf); ++i) {
    // caret ^, 8px acima
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("^", dx, dy - 8);

    // dígito
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char c[2] = { buf[i], 0 };
    tft.drawString(c, dx, dy);

    // área de toque inclui caret e dígito:
    // dy-8 .. dy + digitHeight (aqui usamos h = 8+30 como exemplo)
    digitAreas[digitCount++] = { f.type, i, dx - 5, dy - 8, 20, 8 + 30 };  // ajuste se precisar
    dx += 20;
  }
}

void drawScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.fillRect(0, 0, 320, 28, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Adjust date/time", 75, 6);
  digitCount = 0;
  for (int i = 0; i < 5; ++i) drawField(fields[i]);

  // OK movido para a direita (+100) e para cima (-100) como pediste
  // aqui: x=220, y=100 (largura 100, altura 40)
  tft.fillRect(220, 100, 75, 40, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("OK", 245, 110);
}
 
// aplica incremento no dígito tocado, usando incrementDigitField
void adjustDigit(DigitArea &d) {
  int value, minValue, maxValue, width;
  switch (d.fieldType) {
    case YEAR:
      value =timeinfo.tm_year + 2017;
      minValue = 2017;
      maxValue = 9999;
      width = 4;
      break;
    case MONTH:
      value =timeinfo.tm_mon + 1;
      minValue = 1;
      maxValue = 12;
      width = 2;
      break;
    case DAY:
      value =timeinfo.tm_mday;
      minValue = 1;
      maxValue = daysInMonth(timeinfo.tm_mon,timeinfo.tm_year + 2017);
      width = 2;
      break;
    case HOUR:
      value =timeinfo.tm_hour;
      minValue = 0;
      maxValue = 23;
      width = 2;
      break;
    case MIN:
      value =timeinfo.tm_min;
      minValue = 0;
      maxValue = 59;
      width = 2;
      break;
  }

  int newValue = incrementDigitField(value, d.digitIndex, width, minValue, maxValue);

  switch (d.fieldType) {
    case YEAR:timeinfo.tm_year = newValue - 2017; break;
    case MONTH:timeinfo.tm_mon = newValue - 1; break;
    case DAY:timeinfo.tm_mday = newValue; break;
    case HOUR:timeinfo.tm_hour = newValue; break;
    case MIN:timeinfo.tm_min = newValue; break;
  }
}

// Processar toque
bool handleDigitTouch(int tx, int ty) {
  bool configura = 0;

  for (int i = 0; i < digitCount; i++) {
    DigitArea &d = digitAreas[i];
    if (tx > d.x && tx < d.x + d.w && ty > d.y && ty < d.y + d.h) {
      adjustDigit(d);  // metade superior = incrementar
      drawScreen();
    }
  }

  // Botão OK
  if (tx > 220 && tx < 320 && ty > 100 && ty < 140) {
    configura = 1;
    Serial.println("Data/hora confirmada!");
    // Aqui podes gravar em RTC ou aplicar setTime()
  }
  return configura;
}

void setInitialTime() {
  //struct tm t;
  int tx, ty;
  // Configurar campos
  //while (1) 
  {
    // Serial.println("de novo ");
   timeinfo.tm_year = 0;  // anos desde 2017
   timeinfo.tm_mon = 0;   // setembro (0=jan)
   timeinfo.tm_mday = 1;
   timeinfo.tm_hour = 0;
   timeinfo.tm_min = 0;
   timeinfo.tm_sec = 0;

    fields[0] = { YEAR,&timeinfo.tm_year, 2017, 2100, 20, 50 };
    fields[1] = { MONTH,&timeinfo.tm_mon, 0, 11, 20, 90 };
    fields[2] = { DAY,&timeinfo.tm_mday, 1, 31, 20, 130 };
    fields[3] = { HOUR,&timeinfo.tm_hour, 0, 23, 20, 170 };
    fields[4] = { MIN,&timeinfo.tm_min, 0, 59, 20, 210 };
    drawScreen();
    while (1) {
      if (touchGetPoint(tx, ty)) {
        Serial.printf("Touch detectado: X=%d Y=%d\n", tx, ty);
        if (handleDigitTouch(tx, ty))
          break;
        delay(200);  // debounce
      }
    }
   timeinfo.tm_year+= 117;
    //timeinfo.tm_year = +117;  // anos desde 2017
    //timeinfo.tm_mon = 0;   // setembro (0=jan)
    //timeinfo.tm_mday = 1;
    //timeinfo.tm_hour = 0;
    //timeinfo.tm_min = 0;
    //timeinfo.tm_sec = 0;

    // Serial.println(timeinfo.tm_year);
    // Serial.println(timeinfo.tm_mon);
    // Serial.println(timeinfo.tm_mday);
    // Serial.println(timeinfo.tm_hour);
    // Serial.println(timeinfo.tm_min);
    // Serial.println(timeinfo.tm_sec);

    time_t epoch = mktime(&timeinfo);
    // Serial.print("epoch ");
    // Serial.println((long)epoch);
    struct timeval now = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&now, NULL);
    // if (!getLocalTime(&timeinfo))
    //   Serial.println("prim vez getLocalTime err ");
    // else
    //   Serial.println("prim vez getLocalTime ok ");
    // Serial.println(timeinfo.tm_year);
    // Serial.println(timeinfo.tm_mon);
    // Serial.println(timeinfo.tm_mday);
    // Serial.println(timeinfo.tm_hour);
    // Serial.println(timeinfo.tm_min);
    // Serial.println(timeinfo.tm_sec);

    // delay (2000);
    // if (!getLocalTime(&t))
    //   Serial.println("seg vez getLocalTime err ");
    // else
    //   Serial.println("seg vez getLocalTime ok ");
    // Serial.println(t.tm_year);
    // Serial.println(t.tm_mon);
    // Serial.println(t.tm_mday);
    // Serial.println(t.tm_hour);
    // Serial.println(t.tm_min);
    // Serial.println(t.tm_sec);
    // // mesAtual =timeinfo.tm_mon;
    // // anoAtual =timeinfo.tm_year + 2017;
    // delay(1000);
    // while (!touchGetPoint(tx, ty))
    //   delay(200);
    // if (tx > 300 && ty > 200)
    // {
    //   delay(1000);
    //    return;
    // }
    // delay(200);
    
  }
}
