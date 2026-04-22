/*

MIT License

Copyright (c) 2026 Metehan Semerci

StarDustLite Communication Protocol Implementation
Author: Metehan Semerci
Contact: furkanmetehansemerci@gmail.com

*/




#include "StarDustLite.h"
#include "config.h"// İsteğe bağlı konfigürasyon dosyası / Optional configuration file

// Global instance / Global Nesne
StarDustLiteClass StarDustLite;



// ======================== YAPICI / CONSTRUCTOR ========================
StarDustLiteClass::StarDustLiteClass() {
    _port       = nullptr;
    _state      = SDLiteState::WAIT_START;
    _rxIdx      = 0;
    _lastByteMs = 0;
    memset(_rxBuf, 0, sizeof(_rxBuf));
}

void StarDustLiteClass::begin(Stream& port) {
    _port = &port;
    resetRx(); // RX durumunu sıfırla / Reset RX state
}

void StarDustLiteClass::resetRx() {
    _state      = SDLiteState::WAIT_START;
    _rxIdx      = 0;
    _lastByteMs = 0;
    // RX bufferını temizle / Clear RX buffer
}



// ======================== GÜVENLİK / SECURITY ========================

// CRC-8, polynomial 0x07, initial value 0x00 (standart CRC-8)
uint8_t StarDustLiteClass::_crc8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    return crc;
    // Not: CRC-8 hesaplama, şifrelemeden sonra yapılır. Alımda da CRC kontrolü şifreli veriye göre yapılır.
    // Note: CRC-8 is calculated after encryption. On receive, CRC check is also done on the encrypted data.
}



// XOR şifreleme/çözme (simetrik, aynı fonksiyon)
// XOR encrypt/decrypt (symmetric, same function)
void StarDustLiteClass::_crypt(uint8_t* payload, uint8_t len) {
#ifdef SDLITE_CRYPTO_ENABLE
// Basit XOR şifreleme, her byte pozisyonuna göre farklı bir anahtar kullanır
// Simple XOR encryption, uses a different key based on byte position
    for (uint8_t i = 0; i < len; i++) {
        payload[i] ^= (uint8_t)(SDLITE_CRYPTO_KEY + i);
    }
#else
// Şifreleme devre dışı, fonksiyon boş / Encryption disabled, function is empty
    (void)payload;
    (void)len;
#endif
}



// ======================== GÖNDERİM / SEND ========================
SDLiteStatus StarDustLiteClass::send(uint8_t functionID,
                                      uint8_t paramID,
                                      uint8_t subID,
                                      uint8_t actionID,
                                      int16_t data)
{
    if (_port == nullptr) return SDLiteStatus::ERR_FRAMING; // Port atanmadı / Port not assigned
    // Paket yapısı: START | B1..B6 (şifreli) | CRC | END
    // Packet structure: START | B1..B6 (encrypted) | CRC | END

    // Payload oluştur / Build payload
    uint8_t payload[6] = {
        functionID,
        paramID,
        subID,
        actionID,
        (uint8_t)((uint16_t)data & 0xFF),
        (uint8_t)(((uint16_t)data >> 8) & 0xFF)
    };

    // Şifrele (CRC'den önce) / Encrypt (before CRC)
    _crypt(payload, 6); // Şifreleme, payload'u yerinde değiştirir / Encryption modifies the payload in place

    // CRC hesapla / Calculate CRC
    uint8_t crc = _crc8(payload, 6); // CRC, şifreli payload üzerinden hesaplanır / CRC is calculated over the encrypted payload

    // Gönder / Transmit
    _port->write(SDLITE_START_BYTE);
    for (uint8_t i = 0; i < 6; i++) _port->write(payload[i]);
    _port->write(crc);
    _port->write(SDLITE_END_BYTE);
    // _port->flush(); // İsteğe bağlı, bazı Stream türlerinde gerekebilir / Optional, may be needed for some Stream types

    return SDLiteStatus::OK;
}



// ======================== ALIM / RECEIVE ========================
SDLiteStatus StarDustLiteClass::poll(SDLitePacket& outPacket) {
    if (_port == nullptr) return SDLiteStatus::ERR_FRAMING;

    while (_port->available() > 0) {
        uint8_t  byte = (uint8_t)_port->read();
        // If u read there, may the force be with u :D
        uint32_t now  = millis();

        // Byte arası timeout / Inter-byte timeout
        if (_state != SDLiteState::WAIT_START) {
            if ((now - _lastByteMs) > SDLITE_RX_TIMEOUT_MS) {
                resetRx();
                // Güncel byte yeni START olabilir, WAIT_START'a düşüp devam et
                // Current byte might be new START, fall through to WAIT_START
            }
        }
        _lastByteMs = now;
        // State machine

        switch (_state) {

            case SDLiteState::WAIT_START:
                if (byte == SDLITE_START_BYTE) {
                    _rxBuf[0] = byte;
                    _rxIdx    = 1;
                    _state    = SDLiteState::READ_PAYLOAD;
                }
                break;

            case SDLiteState::READ_PAYLOAD:
                _rxBuf[_rxIdx++] = byte;
                if (_rxIdx == 7) _state = SDLiteState::READ_CRC; // 6 byte payload + 1 byte CRC = 7 bytes total before END
                break;

            case SDLiteState::READ_CRC:
                _rxBuf[_rxIdx++] = byte;  // B7 = CRC
                _state = SDLiteState::READ_END;
                break;

            case SDLiteState::READ_END:
                _rxBuf[_rxIdx++] = byte;  // B8 = END
                resetRx();                // State'i sıfırla / Reset state

                // END byte kontrolü / Check END byte
                if (_rxBuf[8] != SDLITE_END_BYTE) return SDLiteStatus::ERR_FRAMING;

                // CRC kontrolü (şifreli payload üzerinden) / CRC check (over encrypted payload)
                if (_crc8(&_rxBuf[1], 6) != _rxBuf[7]) return SDLiteStatus::ERR_CRC;

                // Şifre çöz / Decrypt
                _crypt(&_rxBuf[1], 6);

                // Paketi doldur / Fill packet
                outPacket.functionID = _rxBuf[1];
                outPacket.paramID    = _rxBuf[2];
                outPacket.subID      = _rxBuf[3];
                outPacket.actionID   = _rxBuf[4];
                outPacket.data       = (int16_t)((uint16_t)_rxBuf[5] | ((uint16_t)_rxBuf[6] << 8));

                return SDLiteStatus::OK;
                // Başarılı alım / Successful receive
        }
    }

    return SDLiteStatus::NO_DATA;
    // Henüz paket yok / No packet yet
}