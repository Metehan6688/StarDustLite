# StarDustLite Protocol v1.0.0

Lightweight, Non-Blocking, Encrypted UART Communication Protocol

Author: Metehan Semerci\
License: MIT

------------------------------------------------------------------------

# Overview

**StarDustLite** is customized from **StarDust v2.0.0** for embedded systems and RTOS (Real-Time
Operating System) environments and features:

-   Non-blocking operation
-   CRC8 verification
-   XOR + crypt key based encryption
-   Master / Multi-Slave architecture
-   Extremely fast communication
-  Optimized for slow and speed important projects

It enables PC-MCU communication.

------------------------------------------------------------------------

# Protocol Structure

Each data packet has the following structure:

[START] [Function ID] [Parameter ID] [Sub ID] [Action ID] [Data Low] [Data High] [CRC8] [END]
-  If user want, can use the names for any other data, payload is only 6 bytes !
-  So that structure is optimized for custom bash for embedded systems/hobby projects


## Constants

  Constant              Description
  --------------------- ------------------------------
  `START BYTE`    Packet start byte (0xAA)
  `END BYTE`         Packet end byte (0x55)

------------------------------------------------------------------------

# Basic Usage

## 1. Object Creation
``` cpp
#include "StarDustLite.h"
StarDustLite sdl;
```

## 2. Initialization
``` cpp
sdl.begin(Serial1);
```

## 3. Encryption Key
``` cpp
configure it in "config.h"
```

## 4. Timeout Setting
``` cpp
configure it in "config.h"

------------------------------------------------------------------------

# Non-Blocking Read

``` cpp
SDlite packet;

if (StarDustLite.poll(packet)) {
    // Valid packet received
}
```

-   UART does not block\
-   CRC verification is performed\
-   Decryption happens automatically (if chosed)

It is recommended to call this inside the task loop in RTOS
environments.

------------------------------------------------------------------------

# Security

-   CRC8 verification
-   XOR + crypt key encryption

Note: Encryption is very lightweight and not military-grade. So that protocol carries only basic commands

Note: That protocol is symmetric !

------------------------------------------------------------------------

# RTOS Compatibility

-   No ISR required
-   Non-blocking architecture
-   State reset via timeout
-   Task-safe usage

------------------------------------------------------------------------

# License

MIT License\
Copyright (c) 2026 Metehan Semerci
