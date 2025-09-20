#ifndef AUXTIME
#define AUXTIME

 

// =====================================================
// Lógica do calendário
// =====================================================
bool isLeapYear(int ano);


int diasNoMes(int mes, int ano);

// Zeller: 0=domingo
int diaDaSemana(int dia, int mes, int ano);

void incDia (int *dia, int *mes, int *ano);

void decDia (int *dia, int *mes, int *ano);

int diasNoMes(int mes, int ano);

#endif
