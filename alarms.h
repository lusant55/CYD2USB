#ifndef ALARMS_H
#define ALARMS_H

#include <Arduino.h>

#define MAX_ALARMS 5

#define BUZZER_PIN 26   // pino ligado ao amplificador
#define BUZZER_CHANNEL 0 // canal PWM (0..15)
#define BUZZER_RES 8    // resolução em bits (0..255 duty)
#define LEDC_TIMER_12_BIT 12


typedef struct {
  int year;    // 0..9999 (mostrado como 0000 se 0)
  int month;   // 0..12
  int day;     // 0..31
  int hour;    // 0..23
  int minute;  // 0..59
  bool daysOfWeek[7]; // segunda..domingo (0=Seg .. 6=Dom)
  bool active;        // ativo/inactivo (X)
} Alarm;

extern Alarm alarms[MAX_ALARMS];
extern int selectedAlarm;

// funções públicas do UI
void alarmUI_enter();                     // desenha e activa UI
void alarmUI_exit();                      // desactiva UI
bool alarmUI_handleTouch(int tx, int ty); // retorna true se pediu saída (top [X])

void buzzerInit();
void buzzerStart(int freqHz);
void buzzerStop(); 
bool alarmsCheckAndBuzz(int year, int month, int day, int hour, int minute, int wday);

#endif // ALARMS_H