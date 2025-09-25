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
