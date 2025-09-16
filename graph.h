#ifndef GRAPH
#define GRAPH

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Função pública: recebe ano, mês, dia
void drawDayGraph(int year, int month, int day);

#ifdef __cplusplus
}
#endif

#endif