#ifndef FERIADOS
#define FERIADOS

typedef struct {
  int dia;          // 1..31
  const char *sigla; // "F", "P", "E", "N"
} Feriado;

#define MAX_FERIADOS_MES 10
typedef struct {
  int count;
  Feriado lista[MAX_FERIADOS_MES];
} FeriadosMes;

void prepararFeriadosMes(int ano, int mes);
const char* feriadoSigla(int dia, int mes);

#endif