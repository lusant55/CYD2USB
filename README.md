# CYD2USB -Cheap Yellow Display com 2 portas USB

## Hardware
Usa o CYD2USB com o display ST7789, leitor de cartões, um sensor DHT11 (quanto a mim pouco preciso) e a saída de aúdio para ligar um altifalante (um de 4 ou 8 Ohm com uma resistência de 100 Ohm em série).
Ter em atenção o pinout do CYD pois há muitas variantes. Neste caso, o display, o sdcard e o XPT2046 não partilham pinos, por isso o display foi associado ao SPI HSPI, o SDCARD ao VSPI e o XPT2046 usa bitbanging (duas funções no código, sendCommand(uint8_t cmd) e readData(uint8_t cmd) - não usa libraries)

## Calendário perpétuo
Tem um calendário perpétuo onde se pode escolher um ano e um mês, sendo também marcados os feriados fixos e móveis (os que dependem da data da Páscoa).
É visivel a hora corrente, a qual pode ser acertada manualmeente ou sincronizada por NTP.

