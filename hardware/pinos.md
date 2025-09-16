| ﻿GPIO | Função         | Tipo de uso          | Observações                                       |
|-------|----------------|----------------------|---------------------------------------------------|
| 13    | TFT_MOSI       | Display ST7789 (SPI) | SPI hardware                                      |
| 14    | TFT_SCLK       | Display ST7789 (SPI) | SPI hardware                                      |
| 15    | TFT_CS         | Display ST7789 (SPI) | Chip Select LCD                                   |
| 2     | TFT_DC         | Display ST7789       | Data/Command                                      |
| 21    | TFT_BL         | Backlight            | Saída digital / PWM                               |
| -1    | TFT_RST        | Display RST          | Não usado ou conectado internamente               |
| 32    | XPT2046 MOSI   | Touch resistivo      | Fisicamente ligado ao touch                       |
| 39    | XPT2046 MISO   | Touch resistivo      | Fisicamente ligado ao touch                       |
| 25    | XPT2046 CLK    | Touch resistivo      | Fisicamente ligado ao touch                       |
| 33    | XPT2046 CS     | Touch resistivo      | Fisicamente ligado ao touch                       |
| 36    | XPT2046 PENIRQ | Touch resistivo      | Fisicamente ligado, não usar para outro propósito |
| 5     | SD_CS          | SD Card              | SPI CS                                            |
| 18    | SD_SCLK        | SD Card              | SPI clock                                         |
| 23    | SD_MOSI        | SD Card              | SPI MOSI                                          |
| 19    | SD_MISO        | SD Card              | SPI MISO                                          |
| 26    | I2S BCLK       | Audio I2S            | Saída/entrada I2S                                 |
| 25    | I2S LRC        | Audio I2S            | Saída/entrada I2S                                 |
| 22    | I2S DOUT       | Audio I2S            | Saída I2S para amplificador 8002                  |
| 4     | RGB Red        | LED RGB              | Saída digital                                     |
| 16    | RGB Green      | LED RGB              | Saída digital                                     |
| 17    | RGB Blue       | LED RGB              | Saída digital                                     |
| 34    | LDR / ADC      | Entrada analógica    | Livre para uso                                    |
| 0     | BOOT           | Botão boot / flash   | Usado para boot, não mexer                        |
| 1     | UART TX        | Serial USB           | Não usar para outro fim se usar USB               |
| 3     | UART RX        | Serial USB           | Não usar para outro fim se usar USB               |
| 27    | Livre          | Digital/ADC          | Pode usar para sensores ou outros periféricos     |
| 35    | Livre          | Digital/ADC          | Pode usar para sensores ou outros periféricos     |