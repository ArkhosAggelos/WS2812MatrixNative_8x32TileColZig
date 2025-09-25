# WS2812MatrixNative_8x32TileColZig

Biblioteca **autoral e didática** para controlar painéis **WS2812B** no formato **8x32**  
(composto por **4 tiles 8x8 enfileirados**, esquerda→direita),  
com **mapeamento serpentina por coluna**:

- Colunas pares (0,2,4,6): top → bottom  
- Colunas ímpares (1,3,5,7): bottom → top  

---

## ✨ Recursos
- **Driver nativo WS2812** (bit-banging em AVR 16 MHz).  
- **Sem dependências externas** (não precisa Adafruit ou FastLED).  
- Funções básicas:
  - `drawPixel(x,y,r,g,b)`
  - `clear()`
  - `show()`
  - `setBrightness(val)`
- **Texto 6x4** pronto para A–Z, 0–9 e espaço.  
- **Rolagem horizontal** simples.

---

## 📦 Instalação
1. Baixe este repositório como `.zip`.  
2. Na IDE Arduino: **Sketch > Include Library > Add .ZIP Library…**  
3. Inclua no seu código:
   ```cpp
   #include <WS2812MatrixNative_8x32TileColZig.h>
   using namespace wsnative;

##🔌 Ligações

- DIN do painel → pino D6 do Arduino UNO/Nano.

- VCC 5 V do painel → fonte 5 V externa (≥3 A recomendado).

- GND comum entre painel e Arduino.

⚠️ Em full-white 256 LEDs podem passar de 4 A.
Use brilho baixo (20–40) para experimentos.

## 🖥️ Exemplo de uso
#include "WS2812MatrixNative_8x32TileColZig.h"
using namespace wsnative;

Matrix8x32 panel(6);   // pino de dados D6
int16_t x = Matrix8x32::WIDTH;
const char* TXT = "  BEM-VINDOS 8x32  ";

void setup() {
  panel.begin();
  panel.setBrightness(40);
}

void loop() {
  x = panel.scrollText6x4Step(TXT, 1, x, Matrix8x32::rgb(255,160,20));
  delay(35);
}

## 📸 Mapeamento validado

- LED índice 0 → canto superior esquerdo (tile 0).

- LED índice 63 → canto superior direito (tile 0).

- LED índice 192 → canto superior esquerdo do 4º tile.

- LED índice 255 → canto superior direito (tile 3).

## ⚠️ Limitações

Implementado para AVR 16 MHz (Arduino UNO/Nano).

Pode ser ajustado para outros MCUs alterando os nop no driver.

Interrupções ficam desativadas durante show() (alguns ms).

## 📜 Licença MIT

Este projeto é de uso didático e educacional.
Você pode modificar e compartilhar livremente, mantendo a atribuição ao autor original.
