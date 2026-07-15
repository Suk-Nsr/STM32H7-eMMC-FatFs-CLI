# STM32-eMMC-FatFs-CLI

A Command Line Interface (CLI) project for STM32 microcontrollers to manage an eMMC file system using the FatFs middleware. This project allows users to interact with the eMMC storage via a serial terminal, providing capabilities to create, read, list, and delete files directly from a PC.

## Features
* **FatFs Integration:** Fully functional FAT32 file system implementation on an eMMC module.
* **Interactive CLI:** A user-friendly Command Line Interface accessible via UART (ST-LINK Virtual COM Port).
* **Long File Name (LFN) Support:** Capable of handling filenames longer than the traditional 8.3 format.
* **Strict Case-Sensitive Handling:** Custom implementation to ensure exact case matching for file operations, preventing accidental deletions or reads of similarly named files (a common quirk in standard FAT32).

## Hardware Requirements
* **Microcontroller:** STM32 Nucleo Board (Developed on NUCLEO-H723ZG, but easily adaptable to other STM32 series).
* **Storage:** eMMC Module (connected via SDMMC interface).
* **Connection:** Standard USB cable for ST-LINK (Provides both programming and Virtual COM Port for the CLI).

## Software Requirements
* **IDE:** [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
* **Terminal Emulator:** PuTTY, Tera Term, or any standard Serial Monitor.

## STM32CubeMX Configuration
If you are recreating this project from scratch, configure the following peripherals in STM32CubeMX:

### 1. SDMMC1 (eMMC Interface)
* **Mode:** SD 4 bits Wide bus (or 8 bits depending on your hardware)
* **Clock divide factor:** Adjust to match your eMMC speed requirements (e.g., `48-1` for stable testing).

### 2. FATFS (Middleware)
* **Mode:** SD Card (or eMMC)
* **USE_LFN (Long File Name):** `Enabled with static working buffer on the BSS`
* **MAX_LFN:** `255`

### 3. USART3 (Virtual COM Port)
* *Note: The USART peripheral number may vary depending on your specific Nucleo board.*
* **Mode:** Asynchronous
* **Baud Rate:** `115200` Bits/s
* **Word Length:** 8 Bits (Including Parity)
* **Stop Bits:** 1
* **Parity:** None
