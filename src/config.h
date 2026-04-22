/*
 * StarDustLite_config.h
 * İsteğe bağlı ayarlar / Optional settings
 * Bu dosyayı projenize kopyalayıp düzenleyin.
 * Copy this file to your project and edit as needed.
 */

#ifndef STARDUSTLITE_CONFIG_H
#define STARDUSTLITE_CONFIG_H

/* Şifrelemeyi etkinleştir / Enable encryption */
#define SDLITE_CRYPTO_ENABLE

/* XOR anahtarı (her iki tarafta aynı olmalı) / XOR key (must match on both sides) */
#define SDLITE_CRYPTO_KEY     0x5A

/* Byte arası maksimum bekleme süresi (ms) / Max inter-byte gap (ms) */
#define SDLITE_RX_TIMEOUT_MS  50

#endif
