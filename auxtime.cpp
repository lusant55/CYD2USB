#include <Arduino.h>
#include "auxtime.h"


// =====================================================
// Lógica do calendário
// =====================================================
bool isLeapYear(int ano) {
  return ((ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0));
}

int diasNoMes(int mes, int ano) {
  switch (mes) {
    case 0:
    case 2: 
    case 4: 
    case 6: 
    case 7:
    case 9:
    case 11: return 31;
    case 1: return isLeapYear(ano) ? 29 : 28;
    default: return 30;
  }
}

// Zeller: 0=domingo
int diaDaSemana(int dia, int mes, int ano) {
  mes= mes + 1;
  if (mes < 3) {
    mes += 12;
    ano -= 1;
  }
  int K = ano % 100;
  int J = ano / 100;
  int h = (dia + 13 * (mes + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  //  return ((h + 6) % 7);
  return ((h + 5) % 7);
}

void incDia (int *dia, int *mes, int *ano)
{
  *dia= *dia + 1;
  if (*dia > diasNoMes (*mes, *ano))
  {
    *dia= 1;
    *mes= *mes + 1;
    if (*mes > 11)
    {
      *mes= 0;
      *ano= *ano + 1;
    }
  }
}

void decDia (int *dia, int *mes, int *ano)
{
  *dia= *dia - 1;
  if (*dia < 1 )
  {
    *mes= *mes - 1;
    if (*mes < 0)
    {
      *ano= *ano - 1;
      *mes= 11;
      *dia= 31;
    }
    else
      *dia= diasNoMes (*mes, *ano);
  }
}

