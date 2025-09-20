#include "graph.h"
#include <SD.h>
#include <TFT_eSPI.h>

// O objeto TFT é global, declarado no main
extern TFT_eSPI tft;

#define MAX_POINTS 288  // máximo de pontos num dia (5 min)

static float temps[MAX_POINTS];
static float hums[MAX_POINTS];
static int minutesOfDay[MAX_POINTS];  // minutos desde 0h
static int pointCount = 0;

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) /
         (in_max - in_min) + out_min;
}

float getMax (float temp, float tmax, int * hmaxt, int * mmaxt, int hh, int min)
  {
    if (temp > tmax)
      {
        *hmaxt= hh;
        *mmaxt= min;
        return temp;
      }
    return tmax;
  }

float getMin (float temp, float tmin, int * hmint, int * mmint, int hh, int min)
  {
    if (temp < tmin)
      {
        *hmint= hh;
        *mmint= min;
        return temp;
      }
    return tmin;
  }


void drawDayGraph(int year, int month, int day) 
  {
    // Caminho: /2025/09/20250913.txt
    char path[64];
    int hmaxt,hmint,mmint,mmaxt, hmaxh,hminh, mmaxh,mminh;
    float tmin= 100, tmax= 0, hmin= 100, hmax= 0;
    int escmaxt= 40, escmint= 0, escmaxh= 100, escminh= 0;
 
    //int lastpointtime= 0;


            // Margens do gráfico
    const int x0 = 20, y0 = 220, w = 280, h = 200;

    if (!SD.begin()) 
      {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("Cartao ausente!", 80, 120);
        //return;
      }
    else
      {
        snprintf(path, sizeof(path), "/%04d/%02d/%04d%02d%02d.txt", year, month, year, month, day);

        File f = SD.open(path);
        if (!f) {
            Serial.printf("Falha ao abrir %s\n", path);
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString("Sem ficheiro!", 80, 120);
            //return;
        }
        else
        {
          pointCount = 0;
          while (f.available() && pointCount < MAX_POINTS) {
              String line = f.readStringUntil('\n');
              if (line.length() < 10) continue;

              float temp, hum;
              int yy, mm, dd, hh, min;
              // Formato esperado: YYYY-MM-DD HH:MM temp hum
              int parsed = sscanf(line.c_str(), "%d-%d-%d %d:%d %f %f",
                                  &yy, &mm, &dd, &hh, &min, &temp, &hum);
              if (parsed == 7) {
                  temps[pointCount] = temp;
                  hums[pointCount]  = hum;
                  minutesOfDay[pointCount] = hh * 60 + min;  // 0..1439
                  //Serial.printf("pointtime %d lastpointtime %d difftime %d\n", minutesOfDay[pointCount], lastpointtime, minutesOfDay[pointCount]- lastpointtime);
                  //Serial.printf("pointCount %d\n", minutesOfDay[pointCount]);
                  //lastpointtime= minutesOfDay[pointCount];
                  pointCount++;
                  tmax= getMax (temp, tmax, &hmaxt, &mmaxt, hh, min);
                  tmin= getMin (temp, tmin, &hmint, &mmint, hh, min);
                  hmax= getMax (hum, hmax, &hmaxh, &mmaxh, hh, min);
                  hmin= getMin (hum, hmin, &hminh, &mminh, hh, min);
              }
          }
          f.close();

          tft.fillScreen(TFT_BLACK);
    
          if (pointCount == 0) {
              tft.setTextColor(TFT_YELLOW, TFT_BLACK);
              tft.drawString("Sem dados!", 100, 120);
              //return;
          }
          else
          {
            tft.setTextSize(1);
            sprintf(path, "Tmax %3.1f @ %02d:%02d", tmax, hmaxt, mmaxt);           
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString(path, 25, 25);
            sprintf(path, "Tmin %3.1f @ %02d:%02d", tmin, hmint, mmint);
            tft.drawString(path, 25, 205);
            tft.setTextColor(TFT_BLUE, TFT_BLACK);
            sprintf(path, "Hmax %3.1f @ %02d:%02d", hmax, hmaxh, mmaxh);
            tft.drawString(path, 195, 25);
            sprintf(path, "Hmin %3.1f @ %02d:%02d", hmin, hminh, mminh);
            tft.drawString(path, 195, 205);
            tft.setTextSize(2);

          escmaxt= (((int) tmax) / 10) * 10 + 10;
          escmint= (((int) tmin) / 10) * 10;
          escmaxh= (((int) hmax) / 10) * 10 + 10;
          escminh= (((int) hmin) / 10) * 10;
          //Serial.printf("maxt %d\nmint %d\nmaxh %d\nminh %d\n", escmaxt, escmint, escmaxh, escminh);
          //Serial.printf("Pointcount %d\n", pointCount);
          //Serial.println("passei");
            // ---- Curva Temperatura ----
            //lastpointtime= minutesOfDay[0];
            for (int i = 1; i < pointCount; i++) {
                int x1 = map(minutesOfDay[i-1], 0, 24*60, x0, x0 + w);
                int y1 = map(temps[i-1], escmint, escmaxt, y0, y0 - h);
                int x2 = map(minutesOfDay[i],   0, 24*60, x0, x0 + w);
                int y2 = map(temps[i],   escmint, escmaxt, y0, y0 - h);
                if (minutesOfDay[i] - minutesOfDay[i-1] > 5)
                  tft.drawLine(x1, y1, x2, y2, TFT_BLACK);
                else
                  tft.drawLine(x1, y1, x2, y2, TFT_RED);
            }

            // ---- Curva Humidade ----
            for (int i = 1; i < pointCount; i++) {
                int x1 = map(minutesOfDay[i-1], 0, 24*60, x0, x0 + w);
                int y1 = map(hums[i-1], escminh, escmaxh, y0, y0 - h);
                int x2 = map(minutesOfDay[i],   0, 24*60, x0, x0 + w);
                int y2 = map(hums[i],   escminh, escmaxh, y0, y0 - h);
                if (minutesOfDay[i] - minutesOfDay[i-1] > 5)
                  tft.drawLine(x1, y1, x2, y2, TFT_BLACK);
                else
                  tft.drawLine(x1, y1, x2, y2, TFT_BLUE);
            }

            Serial.printf("Plotados %d pontos\n", pointCount);
          }
        }
      }
    tft.setTextSize(1);
      // Eixos
      tft.drawRect(x0, y0 - h, w, h, TFT_WHITE);

       // ---- Escalas Y ----
     // Temperatura 0-40
      for (int v = escmint; v <= escmaxt; v += (escmaxt - escmint) / 4) {
          int y = map(v , escmint, escmaxt, y0, y0 - h);
          tft.drawLine(x0 - 3, y, x0, y, TFT_WHITE);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.drawNumber(v, x0 - 15, y - 3);
      }
      // Humidade 0-100
      for (int v = escminh; v <= escmaxh; v += (escmaxh - escminh) / 4) {
          int y = map(v , escminh, escmaxh, y0, y0 - h);
          tft.drawLine(x0 + w, y, x0 + w + 3, y, TFT_WHITE);
          tft.setTextColor(TFT_BLUE, TFT_BLACK);
          tft.drawNumber(v, x0 + w + 5, y - 3);
      }

      // ---- Escala X (0h–24h) ----
      const char* labels[] = {"0h", "6h", "12h", "18h", "24h"};
      for (int i = 0; i < 5; i++) {
          int hour = i * 6;
          int minutes = hour * 60;
          int x = map(minutes, 0, 24 * 60, x0, x0 + w);
          tft.drawLine(x, y0, x, y0 + 3, TFT_WHITE);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.drawString(labels[i], x - 10, y0 + 10);
      }
      // ---- Legenda ----
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("T(C)", 5, 0);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.drawString("H(%)", 300, 0);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    sprintf(path, "%4d.%02d.%02d", year, month, day); 
    tft.drawString(path, 135, 0);
    
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("<<<  <<   <", 45, 10);
    tft.drawString(">   >>  >>>", 220, 10);
    tft.drawString("X", 285, 00);
    tft.setTextSize(2);

}