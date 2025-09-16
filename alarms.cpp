#include <Arduino.h>
#include "alarms.h"
#include <TFT_eSPI.h>
#include <stdio.h>
#include "alarm_storage.h"
#include "auxtime.h"


extern TFT_eSPI tft;
Alarm alarms[MAX_ALARMS];
int selectedAlarm;

#define PLUS_MINUS_TOUCH_OFFSET_X  -8  // ajuste do alinhamento do touch dos +/-

static bool uiActive = false;
bool showErrorMessage = false;

// Layout parameters
static const int TEXT_SIZE = 2;
static const char *TITLE = "Alarmes";
static const char *DOWNAMES = "STQQSSD"; // Seg..Dom
static const int DATE_CHARS = 16; // "YYYY.MM.DD HH:MM"

static int baseX = 8;
static int titleY = 6;
static int baseY = 30;
static int lineSpacing = 18; // compact
static int charW = 12;
static int charH = 16;

static int plusRowY = 0;
static int minusRowY = 0;
static int dateStartX = 0;
static int daysStartX = 0;
static int xMarkX = 0;
static int headerExitX = 0;
static int headerExitY = 0;

// touch zones
typedef struct {
  int x,y,w,h;
  int alarmIdx;   // -1 if not per-line
  int charPos;    // -1 if not a char zone
  int dowIdx;     // 0..6 for day-of-week zones, -1 else
  bool exitTop;
  bool plus;
  bool minus;
} Zone;

#define MAX_ZONES 256
static Zone zones[MAX_ZONES];
static int zoneCount = 0;

void buzzerInit() {
        // Inicializa pino com LEDC
    ledcAttach(BUZZER_PIN, 1000, LEDC_TIMER_12_BIT);
    ledcWrite(BUZZER_PIN, 0); // pino desligado
   
}

void buzzerStart(int freqHz) {
    ledcAttach(BUZZER_PIN, freqHz, LEDC_TIMER_12_BIT);
    ledcWrite(BUZZER_PIN, 2047); // duty = 50%
    Serial.printf("Buzzer on\n");

}

void buzzerStop() {
    // Duty 0 → silêncio
    ledcWrite(BUZZER_PIN, 0);
    Serial.printf("Buzzer off\n");
}

static void addZone(int x,int y,int w,int h,int alarmIdx,int charPos,int dowIdx,bool exitTop,bool plus,bool minus) {
  if (zoneCount >= MAX_ZONES) return;
  zones[zoneCount++] = {x,y,w,h,alarmIdx,charPos,dowIdx,exitTop,plus,minus};
}

// build date string "YYYY.MM.DD HH:MM"
static void buildDateStr(const Alarm &a, char *out) {
  sprintf(out, "%04d.%02d.%02d %02d:%02d", a.year, a.month, a.day, a.hour, a.minute);
}

// map charPos -> field & digit index in field
static int charPosToFieldAndIndex(int pos, int *idxInField) {
  if (pos >=0 && pos <=3) { *idxInField = pos; return 0; }
  if (pos == 5) { *idxInField = 0; return 1; }
  if (pos == 6) { *idxInField = 1; return 1; }
  if (pos == 8) { *idxInField = 0; return 2; }
  if (pos == 9) { *idxInField = 1; return 2; }
  if (pos == 11) { *idxInField = 0; return 3; }
  if (pos == 12) { *idxInField = 1; return 3; }
  if (pos == 14) { *idxInField = 0; return 4; }
  if (pos == 15) { *idxInField = 1; return 4; }
  return -1;
}

static void getFieldDigits(int field, int value, int digits[], int *len) {
  if (field == 0) { digits[0]=(value/1000)%10; digits[1]=(value/100)%10;
                     digits[2]=(value/10)%10; digits[3]=value%10; *len=4; return; }
  digits[0] = (value/10)%10; digits[1] = value%10; *len = 2;
}

static int composeFromDigits(int digits[], int len) {
  int v=0; for (int i=0;i<len;i++) v=v*10+digits[i]; return v;
}

static void clampField(int field, int *val) {
  if (field==0){ if(*val<0)*val=0; if(*val>9999)*val=9999; }
  if (field==1){ if(*val<0)*val=0; if(*val>12)*val=12; }
  if (field==2){ if(*val<0)*val=0; if(*val>31)*val=31; }
  if (field==3){ if(*val<0)*val=0; if(*val>23)*val=23; }
  if (field==4){ if(*val<0)*val=0; if(*val>59)*val=59; }
}

static void applyPlusMinusToSelected(int alarmIdx, int charPos, bool plus) {
  if (alarmIdx<0||alarmIdx>=MAX_ALARMS) return;
  Alarm &a=alarms[alarmIdx];
  int idxInField; 
  int field=charPosToFieldAndIndex(charPos,&idxInField);
  if(field<0) return;

  // Desativa se estava ativo
  if(a.active) a.active = false;

  int digits[4],len;
  int value=0;
  if(field==0)value=a.year;
  else if(field==1)value=a.month;
  else if(field==2)value=a.day;
  else if(field==3)value=a.hour;
  else if(field==4)value=a.minute;

  getFieldDigits(field,value,digits,&len);
  int cur=digits[idxInField];
  cur = plus ? (cur+1)%10 : (cur+9)%10;
  digits[idxInField]=cur;
  int newVal=composeFromDigits(digits,len);
  clampField(field,&newVal);

  if(field==0)a.year=newVal;
  if(field==1)a.month=newVal;
  if(field==2)a.day=newVal;
  if(field==3)a.hour=newVal;
  if(field==4)a.minute=newVal;
}

// validação do alarme
static bool validateAlarm(const Alarm &a) {
    // Apenas ano, mês, dia
    bool dateAllZero = (a.year==0 && a.month==0 && a.day==0);

    int dowCount = 0;
    for(int i=0;i<7;i++) if(a.daysOfWeek[i]) dowCount++;

    if(dateAllZero) {
        // válido se dias da semana selecionados ou só hora/minuto
        if(a.hour>23 || a.minute>59) return false;
        return true;
    } else {
        if(dowCount>0) return false;
        if(a.month<1 || a.month>12) return false;
        if(a.day<1 || a.day>31) return false;
        if(a.day > diasNoMes(a.month - 1, a.year)) return false;        
        if(a.hour>23) return false;
        if(a.minute>59) return false;
    }
    return true;
}

static void redrawAlarmLine(int i) {
  int y=baseY + i*lineSpacing;
  tft.fillRect(0,y-2,320,lineSpacing+2,TFT_BLACK);
  tft.setTextColor(i==selectedAlarm?TFT_GREEN:TFT_WHITE,TFT_BLACK);

  char dateStr[32]; buildDateStr(alarms[i],dateStr);
  for(int p=0;p<DATE_CHARS;p++){
    int x=dateStartX + p*charW;
    tft.setCursor(x,y);
    char tmp[2]={dateStr[p],0}; tft.print(tmp);
  }

  for(int d=0;d<7;d++){
    int dx=daysStartX + d*charW;
    bool activeDay = alarms[i].daysOfWeek[d];
    if(activeDay) 
      tft.setTextColor(i==selectedAlarm?TFT_YELLOW:TFT_WHITE,TFT_BLUE);
    else 
      tft.setTextColor(i==selectedAlarm?TFT_YELLOW:TFT_WHITE,TFT_BLACK);
    char s[2]={DOWNAMES[d],0};
    tft.setCursor(dx,y); tft.print(s);
  }

  int dxX=xMarkX;
  tft.setCursor(dxX,y);
  tft.setTextColor(alarms[i].active?TFT_GREEN:TFT_RED,i==selectedAlarm?TFT_YELLOW:TFT_BLACK);
  tft.print("X");
}

static void buildAndDrawUI() {
  zoneCount=0;
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(TEXT_SIZE);

  charW=tft.textWidth("0"); if(charW<=0)charW=12;
  charH=charW+4;
  dateStartX=baseX;
  daysStartX=dateStartX+DATE_CHARS*charW+charW;
  xMarkX=daysStartX + 7*charW + charW;
  headerExitX=280; headerExitY=4;
  tft.fillRect(0, 0, 320, 28, TFT_BLUE);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.drawString(TITLE,baseX + 110,titleY);

  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.drawString("X",headerExitX,headerExitY);
  addZone(headerExitX,headerExitY,36,18,-1,-1,-1,true,false,false);

  for(int i=0;i<MAX_ALARMS;i++){
    int y=baseY + i*lineSpacing;
    tft.setTextColor(i==selectedAlarm?TFT_GREEN:TFT_WHITE,TFT_BLACK);

    char dateStr[32]; buildDateStr(alarms[i],dateStr);
    for(int p=0;p<DATE_CHARS;p++){
      int x=dateStartX + p*charW;
      tft.setCursor(x,y);
      char tmp[2]={dateStr[p],0}; tft.print(tmp);
      addZone(x,y,charW,charH,i,p,-1,false,false,false);
    }

    for(int d=0;d<7;d++){
      int dx=daysStartX + d*charW;
      if(i==selectedAlarm){ 
        addZone(dx,y,charW,charH,i,-1,d,false,false,false);
      }
      bool activeDay = alarms[i].daysOfWeek[d];
      if(activeDay) 
        tft.setTextColor(i==selectedAlarm?TFT_YELLOW:TFT_WHITE,TFT_BLUE);
      else 
        tft.setTextColor(i==selectedAlarm?TFT_YELLOW:TFT_WHITE,TFT_BLACK);
      char s[2]={DOWNAMES[d],0};
      tft.setCursor(dx,y); tft.print(s);
    }

    int dxX=xMarkX;
    tft.setCursor(dxX,y);
    tft.setTextColor(alarms[i].active?TFT_GREEN:TFT_RED,i==selectedAlarm?TFT_YELLOW:TFT_BLACK);
    tft.print("X");
    addZone(dxX,y,charW,charH,i,-1,-1,false,false,false);
  }

  // + / - apenas sobre os dígitos, offset ajustável
  plusRowY = baseY + MAX_ALARMS*lineSpacing + 8;
  minusRowY = plusRowY + charH + 4;
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);

  static const int digitPos[] = {0,1,2,3,5,6,8,9,11,12,14,15};
  static const int numDigits = sizeof(digitPos)/sizeof(digitPos[0]);
  int plusMinusOffsetX = PLUS_MINUS_TOUCH_OFFSET_X;

  for(int d=0; d<numDigits; d++){
      int p = digitPos[d];
      int x = dateStartX + p*charW;

      tft.setCursor(x,plusRowY); tft.print("+");
      addZone(x + plusMinusOffsetX, plusRowY, charW, charH, -1, p, -1, false, true, false);

      tft.setCursor(x, minusRowY); tft.print("-");
      addZone(x + plusMinusOffsetX, minusRowY, charW, charH, -1, p, -1, false, false, true);
  }
}

// Public API
void alarmUI_enter() { uiActive=true; buildAndDrawUI(); }
void alarmUI_exit()  { uiActive=false; }

bool alarmUI_handleTouch(int tx,int ty){
  if(!uiActive) return false;

      if(showErrorMessage == true)
        {
          tft.fillRect(0,240-16,320,16,TFT_BLACK);
          showErrorMessage= false;

        }


  for(int z=0; z<zoneCount; z++){
    Zone *cz = &zones[z];
    if(tx>=cz->x && tx<cz->x+cz->w && ty>=cz->y && ty<cz->y+cz->h){
      if(cz->exitTop) 
      {
        alarmsSave();
        return true;
      }
      if(cz->plus || cz->minus){ 
          applyPlusMinusToSelected(selectedAlarm,cz->charPos,cz->plus); 
          redrawAlarmLine(selectedAlarm); 
          return false; 
      }


      if(cz->alarmIdx>=0 && cz->dowIdx>=0){ 
          // só permite alterar no alarme selecionado
          if(cz->alarmIdx==selectedAlarm){
            Alarm &a = alarms[cz->alarmIdx];
            if(a.active) a.active=false;
            a.daysOfWeek[cz->dowIdx] = !a.daysOfWeek[cz->dowIdx]; 
            redrawAlarmLine(cz->alarmIdx); 
          }
          return false; 
      }

      if(cz->alarmIdx>=0 && cz->charPos==-1 && cz->dowIdx==-1 && !cz->plus && !cz->minus){ 
          // só ativa/desativa o alarme da linha selecionada
          if(cz->alarmIdx == selectedAlarm) {
              Alarm &a = alarms[cz->alarmIdx];
              if(validateAlarm(a)){
                  a.active = !a.active;
                  redrawAlarmLine(cz->alarmIdx);
              } else {
                  tft.fillRect(0,240-16,320,16,TFT_BLACK);
                  tft.setCursor(0,240-16);
                  tft.setTextColor(TFT_RED,TFT_BLACK);
                  tft.print("Erro: data/dias da semana!");
                  showErrorMessage = true;

              }
          }
          return false;
      }

      if(cz->alarmIdx>=0 && cz->charPos>=0){ 
          selectedAlarm=cz->alarmIdx; 
          buildAndDrawUI(); 
          return false; 
      }
    }
  }
  return false;
}

// Função para verificar alarmes e tocar buzzer se for o caso
bool alarmsCheckAndBuzz(int year, int month, int day, int hour, int minute, int wday) {
  bool match = false;

    for (int i = 0; i < MAX_ALARMS; i++) {
        Alarm &a = alarms[i];

        // Serial.print("Alarme "); Serial.println(i);

        // Só continua se alarme estiver ativo
        if (!a.active) 
          continue;
        // Serial.print(year ); Serial.print(" "); Serial.print(month); Serial.print(" "); Serial.print(day); Serial.print(" "); Serial.print(hour); Serial.print(" "); Serial.print(minute); Serial.print(" "); Serial.println(wday);
        // Serial.print("Alarmes act"); Serial.println(i);
        // Serial.print(a.year ); Serial.print(" "); Serial.print(a.month); Serial.print(" "); Serial.print(a.day); Serial.print(" "); Serial.print(a.hour); Serial.print(" "); Serial.print(a.minute); Serial.print(" ");
        // Serial.print(a.daysOfWeek[0]); Serial.print(" "); Serial.print(a.daysOfWeek[1]);Serial.print(" "); Serial.print(a.daysOfWeek[2]); Serial.print(" "); Serial.print(a.daysOfWeek[3]); Serial.print(" ");
        // Serial.print(a.daysOfWeek[4]); Serial.print(" "); Serial.print(a.daysOfWeek[5]); Serial.print(" ");
        // Serial.println(a.daysOfWeek[6]);

         match = false;

        // Caso 1: data completa (ano, mês, dia)
        if (a.year != 0 && a.month != 0 && a.day != 0) 
          {
                Serial.print("Alarmes com data ");
                Serial.println(i);

            if (a.year == year && a.month == month && a.day == day && a.hour == hour && a.minute == minute) 
              {
                match = true;
                // Serial.print("disparou Alarmes com data ");
                // Serial.println(i);
              }
          }

        // Caso 2: alarme só para hora/minuto (data zerada)
        else 
          if (a.year == 0 && a.month == 0 && a.day == 0) 
          {
            // Serial.print("Alarmes hora ");
            // Serial.println(i);
            if (a.hour == hour && a.minute == minute) 
             {
                // Pode ser com dias da semana ou todos os dias
                if (a.daysOfWeek[0] || a.daysOfWeek[1] || a.daysOfWeek[2] || a.daysOfWeek[3] || a.daysOfWeek[4] || a.daysOfWeek[5] || a.daysOfWeek[6]) 
                {
                  // Serial.print(" disparou Alarmes semana ");
                  // Serial.println(i);
                   if (a.daysOfWeek[wday]) 
                    {
                      match = true;                      
                    }
                      
                } 
                else 
                  {
                    // Serial.print(" disparou Alarmes hora ");
                    // Serial.println(i);
                    match = true; // todos os dias
                    a.active= false;
                    alarmsSave();              
                 }
              }
          }

        if (match) {
            Serial.printf("⏰ Alarme %d disparou!\n", i + 1);
            return true;
                       // toca apenas o primeiro que encontrar
        }
    }
    return match;  
}

