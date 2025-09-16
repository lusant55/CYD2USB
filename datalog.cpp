#include <Arduino.h>
//#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h>
//#include "sd_card.h"

#define CS_PIN 5   // CS do SD Card (ajuste conforme o seu hardware)

// // ---- Config WiFi ----
// const char* ssid     = "SUA_REDE";
// const char* password = "SUA_PASS";

// // ---- Config NTP ----
// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 0;       // offset fuso horário (ex: +3600 para GMT+1)
// const int   daylightOffset_sec = 0;  // horário de verão se aplicável

File logFile;
int currentYear, currentMonth, currentDay;

// função auxiliar: cria diretório se não existir
void ensureDir(const String &path) {
  if (!SD.exists(path)) {
    if (!SD.mkdir(path)) {
      Serial.print("Erro a criar diretoria: ");
      Serial.println(path);
    }
  }
}

bool writelog(float temperatura, float humidade, struct tm timeinfo) {
  // 1. verifica se o cartão está acessível
  if (!SD.begin()) {  
    Serial.println("ERRO: Cartão ausente!");
    return false;
  }

  // 2. constrói o caminho para o ficheiro
  char dirYear[16], dirMonth[24], fileName[48];

  sprintf(dirYear, "/%04d", timeinfo.tm_year + 1900);
  ensureDir(dirYear);

  sprintf(dirMonth, "/%04d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
  ensureDir(dirMonth);

  sprintf(fileName, "/%04d/%02d/%04d%02d%02d.txt", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);

  // 3. abre o ficheiro no modo append
  File logFile = SD.open(fileName, FILE_APPEND);
  if (!logFile) {
    Serial.print("ERRO: não consegui abrir ficheiro ");
    Serial.println(fileName);
    return false;
  }

  // 4. escreve a linha
  char line[80];
  sprintf(line, "%04d-%02d-%02d %02d:%02d  %.2f  %.2f",
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
          timeinfo.tm_hour, timeinfo.tm_min,
          temperatura, humidade);

  logFile.println(line);
  logFile.close();   // fecha sempre

  Serial.print("OK: ");
  Serial.println(line);

  return true;
}

// // abre o ficheiro do dia correto
// void openLogFile(struct tm timeinfo) {
//   char dirYear[10], dirMonth[10], fileName[32];

//   sprintf(dirYear, "/%04d", timeinfo.tm_year + 1900);
//   ensureDir(dirYear);

//   sprintf(dirMonth, "/%04d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
//   ensureDir(dirMonth);

//   sprintf(fileName, "/%04d/%02d/%04d%02d%02d.txt", 
//           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
//           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);

//   logFile = SD.open(fileName, FILE_APPEND);
//   if (!logFile) {
//     Serial.print("Falha ao abrir ficheiro: ");
//     Serial.println(fileName);
//   } else {
//     Serial.print("A escrever em: ");
//     Serial.println(fileName);
//   }

//   currentYear  = timeinfo.tm_year + 1900;
//   currentMonth = timeinfo.tm_mon + 1;
//   currentDay   = timeinfo.tm_mday;
// }



// bool writelog (float temperatura, float humidade, tm timeinfo)
// {
//     // muda de ficheiro se mudou o dia
//     if ((timeinfo.tm_year + 1900 != currentYear) || (timeinfo.tm_mon + 1 != currentMonth) || (timeinfo.tm_mday != currentDay)) 
//     {
//       if (logFile) 
//         logFile.close();
//       openLogFile(timeinfo);
//     }
//     // escreve linha
//     if (logFile) 
//     {
//       char line[80];
//       sprintf(line, "%04d-%02d-%02d %02d:%02d  %.2f  %.2f\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, temperatura, humidade);
//       logFile.print(line))
//       logFile.flush();  // garante gravação imediata
//       Serial.print("Amostra: ");
//       Serial.print(line);
//       return true;
//     }
//     return false;
// }

