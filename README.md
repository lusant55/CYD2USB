# CYD2USB -Cheap Yellow Display com 2 portas USB

## Hardware
Usa o CYD2USB com o display ST7789, leitor de cartões, um sensor DHT11 (quanto a mim pouco preciso) e a saída de aúdio para ligar um altifalante (um de 4 ou 8 Ohm com uma resistência de 100 Ohm em série).
Ter em atenção ao pinout do CYD pois há muitas variantes. Neste caso, o display, o sdcard e o XPT2046 não partilham pinos, por isso o display foi associado ao SPI HSPI, o SDCARD ao VSPI e o XPT2046 usa bitbanging (duas funções no código, sendCommand(uint8_t cmd) e readData(uint8_t cmd) - não usa libraries)

## Funcionalidades
- Calendário perpétuo
  
  Tem um calendário perpétuo onde se pode escolher um ano e um mês, sendo também marcados os feriados fixos e móveis (os que dependem da data da Páscoa).
  É visivel a hora corrente, a qual pode ser acertada manualmente ou sincronizada por NTP.

- Indicação de temperatura e humidade
  
  No écran principal estão visíveis os respectivos valores obtidos do sensor DHT11. Os valores são actualizados a cada 2 minutos.

- Datalogger
  
  Permite guardar os dados da temperatura e da humidade num sdcard, numa estrutura de directorias a partir da raíz (exemplo 2025\09\20250916.txt).
  Podem ser visualizados gráficos diários da evolução das variáveis.

- Alarmes
  
  Permite definir até 5 alarmes, por hora, por data/hora e semanais, num ou mais dias da semana.

## Utilização
Tudo que está escrito em amarelo faz alguma coisa ... experimentem ...

Já faz muitos anos que não mexia no C, por isso não esperem ver código perfeito ... até porque a maior parte foi a IA que fez ... 



