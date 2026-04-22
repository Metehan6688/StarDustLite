/*
 
MIT License
 
Copyright (c) 2026 Metehan Semerci
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 
StarDustLite Communication Protocol
Author: Metehan Semerci
Contact: furkanmetehansemerci@gmail.com
 
StarDust protokolünün hafif versiyonu.
AVR 9600 baud'da bile rahat çalışacak şekilde 9 byte'a indirilmiştir.
 
Lightweight version of the StarDust protocol.
Reduced to 9 bytes so it runs comfortably on AVR at 9600 baud.
 
Paket yapısı / Packet structure:
  [0xAA][FID][PID][SID][AID][DATA_L][DATA_H][CRC8][0x55]
     B0   B1   B2   B3   B4    B5      B6     B7    B8
 
*/
 
#ifndef STARDUSTLITE_H
#define STARDUSTLITE_H
 
#include <stdint.h>
#include <string.h>
 
#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <cstddef>
    // PC veya RTOS ortamı için temel Stream sınıfı
    // Basic Stream class for PC or RTOS environments
    class Stream {
    public:
        virtual size_t write(uint8_t byte)  { return 0; }
        virtual int    available()           { return 0; }
        virtual int    read()                { return -1; }
    };
    extern uint32_t millis();
#endif
 
/* -- Şifreleme (isteğe bağlı) ─────────────────────────────────────────────
/* -- Encryption (optional) ─────────────────────────────────────────────
 * Etkinleştirmek için StarDustLite_config.h içinde tanımlayın:
 *   #define SDLITE_CRYPTO_ENABLE
 *   #define SDLITE_CRYPTO_KEY  0x5A
 *
 * To enable, define in StarDustLite_config.h:
 *   #define SDLITE_CRYPTO_ENABLE
 *   #define SDLITE_CRYPTO_KEY  0x5A
 * 
 *  0x5A gibi basit bir XOR anahtarı kullanır. Her byte, anahtar + byte indexi ile XOR'lanır.
 *  Uses a simple XOR key like 0x5A. Each byte is XORed with the key plus the byte index.
 * 
 *  Şifreleme, CRC hesaplamasından önce yapılır, böylece CRC şifreli veriye göre hesaplanır.
 *  Encryption is   done before CRC calculation, so CRC is calculated over the encrypted data.  
 */
#if __has_include("StarDustLite_config.h")
    #include "StarDustLite_config.h"
#endif
 
#ifndef SDLITE_CRYPTO_KEY
    #define SDLITE_CRYPTO_KEY   0x5A
#endif
 
#ifndef SDLITE_RX_TIMEOUT_MS
    #define SDLITE_RX_TIMEOUT_MS  50
#endif
 
/* ── Protokol sabitleri / Protocol constants ──────────────────────────── */
constexpr uint8_t SDLITE_START_BYTE = 0xAA; // Başlangıç byte'ı / Start byte
constexpr uint8_t SDLITE_END_BYTE   = 0x55; // Bitiş byte'ı / End byte
constexpr uint8_t SDLITE_PACKET_SIZE = 9;   // Toplam paket boyutu (START + 6 byte payload + CRC + END) / Total packet size (START + 6 byte payload + CRC + END)
 
/* ── Paket yapısı / Packet structure ─────────────────────────────────── */
struct __attribute__((packed)) SDLitePacket {
    uint8_t functionID; // FID for custom bash
    uint8_t paramID;    // PID for custom bash
    uint8_t subID;      // SID for custom bash 
    uint8_t actionID;   // AID for custom bash
    int16_t data;       // 16-bit signed data field
};
 
/* ── Dönüş kodları / Return codes ────────────────────────────────────── */
enum class SDLiteStatus : int8_t {
    OK          =  0,   // Başarılı / Success
    NO_DATA     =  1,   // Henüz paket yok / No packet yet
    ERR_CRC     = -1,   // CRC uyuşmazlığı / CRC mismatch
    ERR_FRAMING = -2,   // END byte hatalı / Wrong END byte
    ERR_TIMEOUT = -3,   // Byte arası süre aşıldı / Inter-byte timeout
};
 
/* ── Parser durumları / Parser states ────────────────────────────────── */
enum class SDLiteState : uint8_t {
    WAIT_START,      // START byte'ını bekle / Waiting for START byte
    READ_PAYLOAD,    // Payload'u oku / Reading payload
    READ_CRC,        // CRC byte'ını oku / Reading CRC byte
    READ_END         // END byte'ını oku / Reading END byte
};
 




/* === StarDustLite sınıfı / StarDustLite class === */
class StarDustLiteClass {
public:
 
    StarDustLiteClass();
    // Kopyalama ve atama engellendi / Copy and assignment disabled
 
    // Sistemi başlat / Initialize
    void begin(Stream& port);
 
    // Paket gönder / Send a packet
    SDLiteStatus send(uint8_t functionID,
                      uint8_t paramID,
                      uint8_t subID,
                      uint8_t actionID,
                      int16_t data);
                      // Maybe I will add new things like float blablabalaaa. If u want add more that's open source!
 
    // Ana döngüde çağrılır, non-blocking / Call in loop(), non-blocking
    SDLiteStatus poll(SDLitePacket& outPacket);
 
    // RX durumunu sıfırla / Reset RX state machine
    void resetRx();
 
private:
    Stream*      _port;                           // İletişim portu / Communication port
    SDLiteState  _state;                          // RX state machine durumu / RX state machine state
    uint8_t      _rxBuf[SDLITE_PACKET_SIZE];      // RX buffer (START + 6 byte payload + CRC + END) / RX buffer (START + 6 byte payload + CRC + END)
    uint8_t      _rxIdx;                          // RX buffer indexi / RX buffer index
    uint32_t     _lastByteMs;                     // Son byte alım zamanı / Timestamp of last byte received
  
    static uint8_t _crc8(const uint8_t* data, uint8_t len);  // CRC-8 hesaplama / CRC-8 calculation
    static void    _crypt(uint8_t* payload, uint8_t len);    // Basit XOR şifreleme / Simple XOR encryption
};
 
extern StarDustLiteClass StarDustLite;
// StarDustLite global nesnesi / StarDustLite global instance
 
#endif
 