#ifndef PI_ATECC_H
#define PI_ATECC_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define I2C_DEVICE "/dev/i2c-1"         // I2C device file
#define ATECC_I2C_ADDRESS 0x60          // Default I2C address for ATECC608A
#define ATECC_CMD_SIZE 128              // Maximum command size
#define ATECC_RESPONSE_SIZE 128         // Maximum response size
#define ATECC_WAKE_DELAY_US 1500        // Delay after wake command
#define ATECC_SLEEP_DELAY_US 500        // Delay after sleep command
#define ATECC_MAX_RETRIES 3             // Maximum number of retries for I2C operations
#define ATECC_WAKE_TOKEN 0x00           // Wake token byte
#define ATECC_CMD_WAKE 0x00             // Wake command
#define ATECC_CMD_SLEEP 0x01            // Sleep command
#define ATECC_CMD_READ 0x02             // Read command
#define ATECC_CMD_WRITE 0x03            // Write command
#define ATECC_CMD_RANDOM 0x1B           // Random number command
#define ATECC_CMD_SHA 0x47              // SHA command
#define ATECC_STATUS_SUCCESS 0x00       // Success status
#define ATECC_STATUS_WAKE 0x11          // Wake token status
#define ATECC_STATUS_ERROR 0xFF         // Generic error status
#define ATECC_SERIAL_NUMBER_SIZE 9      // 9 bytes serial number size
#define ATECC_TOTAL_READ_SIZE 32        // 128 bytes command + 32 bytes response
#define ATECC_WORDADDR_CMD 0x03         // Command word address
#define ATECC_WORDADDR_STATUS 0x00      // Status word address 
#define ATECC_WORDADDR_SLEEP 0x01       // Sleep word address
#define ATECC_CMD_AES_ENCRYPT 0xAE      // AES Encrypt command
#define ATECC_CMD_AES_DECRYPT 0xAF      // AES Decrypt command

#endif // PI_ATECC_H