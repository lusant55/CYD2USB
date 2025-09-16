
#include <Arduino.h>

#include "feriados.h"
#include "auxtime.h"

FeriadosMes feriadosAno[12]; // índice = mes 0..11

// -------------------------
// Algoritmo da Páscoa (Gauss/Knuth)
// -------------------------
void easterDate(int ano, int *mes, int *dia) {
  int a = ano % 19;
  int b = ano / 100;
  int c = ano % 100;
  int d = b / 4;
  int e = b % 4;
  int f = (b + 8) / 25;
  int g = (b - f + 1) / 3;
  int h = (19*a + b - d - g + 15) % 30;
  int i = c / 4;
  int k = c % 4;
  int l = (32 + 2*e + 2*i - h - k) % 7;
  int m = (a + 11*h + 22*l) / 451;
  int mesCalc = (h + l - 7*m + 114) / 31; // 3=Março, 4=Abril
  int diaCalc = ((h + l - 7*m + 114) % 31) + 1;

  *mes = mesCalc - 1; // ajusta para 0..11
  *dia = diaCalc;
}

// -------------------------
// Funções auxiliares para offset de dias
// -------------------------
int toDayNumber(int ano, int mes, int dia) {
  static const int diasAntesMes[] = {0,31,59,90,120,151,181,212,243,273,304,334};
  int leap = (mes > 1 && isLeapYear(ano)) ? 1 : 0;
  return ano*365 + ano/4 - ano/100 + ano/400 + diasAntesMes[mes] + dia + leap;
}

void fromDayNumber(int dn, int *ano, int *mes, int *dia) {
  int y = dn / 366;
  while (toDayNumber(y+1, 0, 1) <= dn) y++;
  int dAno = dn - toDayNumber(y, 0, 1) + 1;

  int m = 0;
  while (dAno > diasNoMes(m, y)) {
    dAno -= diasNoMes(m, y);
    m++;
  }
  *ano = y;
  *mes = m;
  *dia = dAno;
}

void addDays(int ano, int mes, int dia, int offset, int *mesOut, int *diaOut) {
  int dn = toDayNumber(ano, mes, dia);
  dn += offset;
  int y;
  fromDayNumber(dn, &y, mesOut, diaOut);
}

// -------------------------
// Gestão de feriados
// -------------------------
void addFeriado(int mes, int dia, const char* sigla) {
  if (feriadosAno[mes].count < MAX_FERIADOS_MES) {
    int idx = feriadosAno[mes].count++;
    feriadosAno[mes].lista[idx].dia = dia;
    feriadosAno[mes].lista[idx].sigla = sigla;
  }
}

static int pascoaAno = -1;
static int pascoaMes = -1;
static int pascoaDia = -1;

void prepararFeriadosMes(int ano, int mes) {
  feriadosAno[mes].count = 0;  // limpa só este mês

  // ---------- Fixos ----------
  switch (mes) {
    case 0:  addFeriado(0,  1, "F"); break;   // 1 Janeiro
    case 3:  addFeriado(3, 25, "F"); break;   // 25 Abril
    case 4:  addFeriado(4,  1, "F"); break;   // 1 Maio
    case 5:  addFeriado(5, 10, "F"); break;   // 10 Junho
    case 7:  addFeriado(7, 15, "F"); break;   // 15 Agosto
    case 9:  addFeriado(9,  5, "F"); break;   // 5 Outubro
    case 10: addFeriado(10, 1, "F"); break;   // 1 Novembro
    case 11:
      addFeriado(11, 1, "F");   // 1 Dezembro
      addFeriado(11, 8, "F");   // 8 Dezembro
      addFeriado(11,25, "N");   // Natal
      break;
  }

  // ---------- Páscoa (com cache) ----------
  if (ano != pascoaAno) {
    easterDate(ano, &pascoaMes, &pascoaDia);
    pascoaAno = ano;
  }

  // Páscoa
  if (pascoaMes == mes) addFeriado(pascoaMes, pascoaDia, "P");

  int mm, dd;

  // Carnaval (Páscoa - 47)
  addDays(ano, pascoaMes, pascoaDia, -47, &mm, &dd);
  if (mm == mes) addFeriado(mm, dd, "E");

  // Sexta-feira Santa (Páscoa - 2)
  addDays(ano, pascoaMes, pascoaDia, -2, &mm, &dd);
  if (mm == mes) addFeriado(mm, dd, "F");

  // Dia da Espiga (Páscoa + 39)
  addDays(ano, pascoaMes, pascoaDia, +39, &mm, &dd);
  if (mm == mes) addFeriado(mm, dd, "F");

  // Corpo de Deus (Páscoa + 60)
  addDays(ano, pascoaMes, pascoaDia, +60, &mm, &dd);
  if (mm == mes) addFeriado(mm, dd, "F");
}
const char* feriadoSigla(int dia, int mes) {
  for (int i = 0; i < feriadosAno[mes].count; i++) {
    if (feriadosAno[mes].lista[i].dia == dia) {
      return feriadosAno[mes].lista[i].sigla;
    }
  }
  return NULL;
}