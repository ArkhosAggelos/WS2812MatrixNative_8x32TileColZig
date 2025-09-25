#pragma once
/*
  WS2812MatrixNative_8x32TileColZig (header-only)
  - Painel: 8x32 (4 tiles 8x8 lado a lado, esquerda->direita)
  - Mapeamento interno: serpentina por COLUNA (coluna par top->bottom, ímpar bottom->top)
  - Driver WS2812 nativo (800 kHz) para AVR 16 MHz (Arduino UNO/Nano)
  - Texto 6x4 e rolagem simples

  Feito para fins didáticos. Sem dependências externas.
*/

#include <Arduino.h>

namespace wsnative {

// =================== Driver WS2812 (AVR 16 MHz) ===================
class WS2812Driver {
public:
  WS2812Driver() : _port(nullptr), _mask(0) {}

  void begin(uint8_t pin) {
    _mask = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);
    _port = portOutputRegister(port);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // Envia N bytes (ordem: GRB…GRB)
  void send(const uint8_t* data, uint16_t nbytes) {
    if (!_port || !data || nbytes == 0) return;

    noInterrupts();
    for (uint16_t i = 0; i < nbytes; i++) {
      sendByte(data[i]);
    }
    interrupts();

    // Reset code: manter baixo por >= 50 us
    // (o loop acima já deixa em LOW; garantimos o latch)
    delayMicroseconds(60);
  }

private:
  volatile uint8_t* _port;
  uint8_t _mask;

  // Timings aproximados para 16 MHz:
  // T0H ~0.35us, T0L ~0.8us ; T1H ~0.7us, T1L ~0.6us
  // Cada NOP ~62.5 ns. Sequência: HIGH -> NOPs -> LOW -> NOPs.

  inline void sendBit1() {
    *_port |=  _mask;         // HIGH
    // ~0.75 us HIGH
    asm volatile(
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t""nop\n\t"::);
    *_port &= ~_mask;         // LOW
    // ~0.6 us LOW
    asm volatile(
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t"::);
  }

  inline void sendBit0() {
    *_port |=  _mask;         // HIGH
    // ~0.35 us HIGH
    asm volatile(
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t"::);
    *_port &= ~_mask;         // LOW
    // ~0.8 us LOW
    asm volatile(
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
      "nop\n\t""nop\n\t""nop\n\t"::);
  }

  inline void sendByte(uint8_t b) {
    for (uint8_t i = 0; i < 8; i++) {
      (b & 0x80) ? sendBit1() : sendBit0();
      b <<= 1;
    }
  }
};

// =================== Matriz 8x32 (tile 8x8 col-zig) ===================
class Matrix8x32 {
public:
  static const uint8_t TILE_W = 8, TILE_H = 8;
  static const uint8_t TILES_X = 4, TILES_Y = 1;
  static const uint8_t WIDTH   = TILE_W * TILES_X;   // 32
  static const uint8_t HEIGHT  = TILE_H * TILES_Y;   // 8
  static const uint16_t NUM_LEDS = WIDTH * HEIGHT;   // 256

  explicit Matrix8x32(uint8_t dataPin)
  : _brightness(255) {
    _driver.begin(dataPin);
    clear();
  }

  // ---- Básico ----
  void begin() {} // já inicializado no construtor
  void setBrightness(uint8_t b) { _brightness = b; } // 0..255 (aplicado no envio)
  void clear() { memset(_buf, 0, sizeof(_buf)); }
  inline uint8_t width()  const { return WIDTH; }
  inline uint8_t height() const { return HEIGHT; }

  // Define um pixel (RGB 0..255). Ignora fora dos limites.
  void drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
    uint16_t idx = indexFromXY((uint8_t)x, (uint8_t)y);     // índice de LED (0..255)
    uint16_t p = idx * 3;                                   // posição no buffer (GRB)
    _buf[p+0] = g; _buf[p+1] = r; _buf[p+2] = b;
  }

  // Cor pronta (GRB ou RGB? — usamos RGB aqui por didática)
  static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)r << 16 | (uint32_t)g << 8 | b;
  }
  void drawPixel(int16_t x, int16_t y, uint32_t color) {
    drawPixel(x, y, (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF);
  }

  // Envia para o painel aplicando brilho global por escala simples.
  void show() {
    // Geramos um buffer temporário escalado (para manter _buf intacto).
    // Obs.: 256*3 = 768 bytes -> temporário cabe na RAM do UNO (2 kB) com folga.
    static uint8_t out[NUM_LEDS * 3];
    const uint8_t br = _brightness;
    if (br == 255) {
      memcpy(out, _buf, sizeof(out));
    } else {
      for (uint16_t i = 0; i < NUM_LEDS*3; i++) {
        out[i] = (uint16_t(_buf[i]) * br) >> 8; // escala rápida
      }
    }
    _driver.send(out, NUM_LEDS * 3);
  }

  // ---- Texto 6x4 ----
  void drawChar6x4(char ch, int16_t row, int16_t col, uint32_t color) {
    const uint8_t* g = glyph(ch);
    uint8_t r = (color>>16)&0xFF, gr = (color>>8)&0xFF, b = color&0xFF;
    for (uint8_t rr = 0; rr < 6; rr++) {
      uint8_t line = g[rr] & 0x0F; // 4 colunas
      for (uint8_t cc = 0; cc < 4; cc++) {
        if (line & (1 << cc)) drawPixel(col + cc, row + rr, r, gr, b);
      }
    }
  }

  void drawText6x4(const char* s, int16_t row, int16_t col, uint32_t color) {
    int16_t x = col;
    for (const char* p = s; *p; ++p) {
      drawChar6x4(*p, row, x, color);
      x += 5; // 4 + 1 espaço
    }
  }

  // Passo de rolagem para a esquerda; retorna novo x
  int16_t scrollText6x4Step(const char* s, int16_t y, int16_t x, uint32_t color) {
    clear();
    drawText6x4(s, y, x, color);
    show();
    int16_t wpx = (int16_t)strlen(s) * 5;
    x--;
    if (x < -wpx) x = WIDTH;
    return x;
  }

private:
  WS2812Driver _driver;
  uint8_t _buf[NUM_LEDS * 3]; // GRB por LED
  uint8_t _brightness;

  // Conversão XY -> índice linear (tile 8x8; serpentina por COLUNA)
  static inline uint16_t indexFromXY(uint8_t x, uint8_t y) {
    uint8_t tileX = x / TILE_W;         // 0..3
    uint8_t lx    = x % TILE_W;         // 0..7 (coluna no tile)
    uint8_t ly    = y % TILE_H;         // 0..7 (linha no tile)
    uint16_t inTile = (lx & 1) ? (lx * TILE_H + (TILE_H - 1 - ly))
                               : (lx * TILE_H + ly);
    return uint16_t(tileX) * (TILE_W * TILE_H) + inTile; // +0, +64, +128, +192
  }

  // ----- Fonte 6x4 -----
  static const uint8_t* glyph(char ch) {
    static const uint8_t SP[6]={0,0,0,0,0,0};
    // A–Z
    static const uint8_t A_[6]={0b0110,0b1001,0b1111,0b1001,0b1001,0b1001};
    static const uint8_t B_[6]={0b1110,0b1001,0b1110,0b1001,0b1001,0b1110};
    static const uint8_t C_[6]={0b0111,0b1000,0b1000,0b1000,0b1000,0b0111};
    static const uint8_t D_[6]={0b1110,0b1001,0b1001,0b1001,0b1001,0b1110};
    static const uint8_t E_[6]={0b1111,0b1000,0b1110,0b1000,0b1000,0b1111};
    static const uint8_t F_[6]={0b1111,0b1000,0b1110,0b1000,0b1000,0b1000};
    static const uint8_t G_[6]={0b0111,0b1000,0b1011,0b1001,0b1001,0b0111};
    static const uint8_t H_[6]={0b1001,0b1001,0b1111,0b1001,0b1001,0b1001};
    static const uint8_t I_[6]={0b0111,0b0010,0b0010,0b0010,0b0010,0b0111};
    static const uint8_t J_[6]={0b0011,0b0001,0b0001,0b0001,0b1001,0b0110};
    static const uint8_t K_[6]={0b1001,0b1010,0b1100,0b1010,0b1010,0b1001};
    static const uint8_t L_[6]={0b1000,0b1000,0b1000,0b1000,0b1000,0b1111};
    static const uint8_t M_[6]={0b1001,0b1111,0b1111,0b1001,0b1001,0b1001};
    static const uint8_t N_[6]={0b1001,0b1101,0b1101,0b1011,0b1011,0b1001};
    static const uint8_t O_[6]={0b0110,0b1001,0b1001,0b1001,0b1001,0b0110};
    static const uint8_t P_[6]={0b1110,0b1001,0b1001,0b1110,0b1000,0b1000};
    static const uint8_t Q_[6]={0b0110,0b1001,0b1001,0b1011,0b1010,0b0101};
    static const uint8_t R_[6]={0b1110,0b1001,0b1001,0b1110,0b1010,0b1001};
    static const uint8_t S_[6]={0b0111,0b1000,0b0110,0b0001,0b0001,0b1110};
    static const uint8_t T_[6]={0b1111,0b0010,0b0010,0b0010,0b0010,0b0010};
    static const uint8_t U_[6]={0b1001,0b1001,0b1001,0b1001,0b1001,0b0110};
    static const uint8_t V_[6]={0b1001,0b1001,0b1001,0b1001,0b0110,0b0110};
    static const uint8_t W_[6]={0b1001,0b1001,0b1001,0b1111,0b1111,0b1001};
    static const uint8_t X_[6]={0b1001,0b1001,0b0110,0b0110,0b1001,0b1001};
    static const uint8_t Y_[6]={0b1001,0b1001,0b0110,0b0010,0b0010,0b0010};
    static const uint8_t Z_[6]={0b1111,0b0001,0b0010,0b0100,0b1000,0b1111};
    // 0–9
    static const uint8_t D0[6]={0b0110,0b1001,0b1001,0b1001,0b1001,0b0110};
    static const uint8_t D1[6]={0b0010,0b0110,0b0010,0b0010,0b0010,0b0111};
    static const uint8_t D2[6]={0b1110,0b0001,0b0010,0b0100,0b1000,0b1111};
    static const uint8_t D3[6]={0b1110,0b0001,0b0110,0b0001,0b0001,0b1110};
    static const uint8_t D4[6]={0b1001,0b1001,0b1111,0b0001,0b0001,0b0001};
    static const uint8_t D5[6]={0b1111,0b1000,0b1110,0b0001,0b0001,0b1110};
    static const uint8_t D6[6]={0b0110,0b1000,0b1110,0b1001,0b1001,0b0110};
    static const uint8_t D7[6]={0b1111,0b0001,0b0010,0b0100,0b0100,0b0100};
    static const uint8_t D8[6]={0b0110,0b1001,0b0110,0b1001,0b1001,0b0110};
    static const uint8_t D9[6]={0b0110,0b1001,0b1001,0b0111,0b0001,0b0110};

    if (ch==' ') return SP;
    if (ch>='A'&&ch<='Z'){ static const uint8_t* const T[26]={A_,B_,C_,D_,E_,F_,G_,H_,I_,J_,K_,L_,M_,N_,O_,P_,Q_,R_,S_,T_,U_,V_,W_,X_,Y_,Z_}; return T[ch-'A']; }
    if (ch>='a'&&ch<='z'){ static const uint8_t* const T[26]={A_,B_,C_,D_,E_,F_,G_,H_,I_,J_,K_,L_,M_,N_,O_,P_,Q_,R_,S_,T_,U_,V_,W_,X_,Y_,Z_}; return T[ch-'a']; }
    if (ch>='0'&&ch<='9'){ static const uint8_t* const T[10]={D0,D1,D2,D3,D4,D5,D6,D7,D8,D9}; return T[ch-'0']; }
    return SP;
  }
};

} // namespace wsnative

