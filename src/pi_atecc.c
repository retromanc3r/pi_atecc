#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "pi_atecc.h"

/**
 * @brief CRC16-CCITT (0x8005) checksum (little-endian) taken from CryptoAuthLib
 * 
 * @param length Number of bytes in data
 * @param data Pointer to data bytes
 * @param crc_le Pointer to 2-byte array to store little-endian CRC result
 */
static void calc_crc16_ccitt(size_t length, const uint8_t *data, uint8_t *crc_le) {
    size_t counter;
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;

    for (counter = 0; counter < length; counter++) {
        for (shift_register = 0x01; shift_register > 0x00u; shift_register <<= 1) {
            data_bit = ((data[counter] & shift_register) != 0u) ? 1u : 0u;
            crc_bit = (uint8_t)(crc_register >> 15);
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }
    crc_le[0] = (uint8_t)(crc_register & 0x00FFu);
    crc_le[1] = (uint8_t)(crc_register >> 8u);
}

/**
 * @brief Compute CRC of the data bytes (excluding the CRC bytes)
 * 
 * @param length Number of bytes in data
 * @param data Pointer to data bytes
 * @param crc Pointer to 2-byte array to store little-endian CRC result
 * 
 */
static void compute_crc(uint8_t length, uint8_t *data, uint8_t *crc) {
    calc_crc16_ccitt(length, data, crc);
}

/**
 * @brief Validate CRC16-CCITT checksum of response data
 * 
 * @param response Pointer to response data including CRC bytes
 * @param length Length of response data including CRC bytes
 * @return true if CRC is valid, false otherwise
 */
static bool validate_crc(uint8_t *response, size_t length) {
    if (length < 3) return false; // Not enough bytes for CRC
    uint8_t computed_crc[2];
    compute_crc(length - 2, response, computed_crc);
    return (computed_crc[0] == response[length - 2] && computed_crc[1] == response[length - 1]);
}

/**
 * @brief Debug function to print expected and computed CRC values
 * 
 * @param data Pointer to data bytes including CRC
 * @param length Length of data bytes including CRC
 * @param expected_crc Pointer to expected CRC bytes
 */
static void debug_crc_mismatch(uint8_t *data, size_t length, uint8_t *expected_crc) {
    uint8_t computed_crc[2];
    compute_crc(length - 2, data, computed_crc);

    printf("🔍 Expected CRC: %02X %02X\n", expected_crc[0], expected_crc[1]);
    printf("🔍 Computed CRC: %02X %02X\n", computed_crc[0], computed_crc[1]);

    if (computed_crc[0] == expected_crc[0] && computed_crc[1] == expected_crc[1]) {
        printf("✅ CRC MATCH\n");
    } else {
        printf("❌ CRC MISMATCH\n");
    }
}

/**
 * @brief Sleep for specified milliseconds
 * 
 * @param milliseconds Number of milliseconds to sleep
 */
static void sleep_ms(unsigned int milliseconds) {
    usleep(milliseconds * 1000U);
}

/**
 * @brief Sends a command to an ATECC device over the I2C bus.
 *
 * This function constructs the command with the given parameters, calculates the CRC16-CCITT checksum,
 * and sends the full command using the I2C file descriptor.
 *
 * @param[in] fd The I2C file descriptor.
 * @param[in] opcode The command opcode to send to the ATECC device.
 * @param[in] param1 The first parameter for the command.
 * @param[in] param2 The second parameter for the command.
 * @param[in] data The data to send with the command (can be NULL if data_len is 0).
 * @param[in] data_len The length of the data to send.
 * @param[in] resp Response buffer (unused, kept for API compatibility).
 * @param[in] resp_max Response buffer size (unused, kept for API compatibility).
 * @return bool Returns true on success, false on failure.
 */
static bool send_atecc_cmd(int fd, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data,
                    uint8_t data_len, uint8_t *resp, uint16_t resp_max) {
    (void)resp;
    (void)resp_max;

    if (data_len > (ATECC_CMD_SIZE - 7)) {
        errno = EINVAL;
        return false;
    }

    uint8_t command[7 + data_len];
    command[0] = 0x07 + data_len;
    command[1] = opcode;
    command[2] = param1;
    command[3] = param2 & 0xFF;
    command[4] = (param2 >> 8) & 0xFF;

    if (data_len > 0) {
        if (data == NULL) {
            errno = EINVAL;
            return false;
        }
        memcpy(&command[5], data, data_len);
    }

    calc_crc16_ccitt(5 + data_len, command, &command[5 + data_len]);

    uint8_t full_command[8 + data_len];
    full_command[0] = ATECC_WORDADDR_CMD;
    memcpy(&full_command[1], command, sizeof(command));

    struct i2c_rdwr_ioctl_data write_data = {0};
    struct i2c_msg write_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = 0,
        .len   = (uint16_t)sizeof(full_command),
        .buf   = full_command
    };
    write_data.msgs  = &write_msg;
    write_data.nmsgs = 1;
    if (ioctl(fd, I2C_RDWR, &write_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        perror("send_atecc_cmd: I2C write failed");
        return false;
    }

    return true;
}

/**
 * @brief Receives a response from an ATECC device over the I2C bus.
 * 
 * @param fd I2C file descriptor
 * @param buffer Buffer to store the received response
 * @param length Expected length of the response data
 * @param full_response Whether to read the full response including CRC
 * @return true if response received successfully, false otherwise
 */
static bool receive_atecc_response(int fd, uint8_t *buffer, size_t length, bool full_response) {
    if (!buffer || length == 0) {
        errno = EINVAL;
        return false;
    }

    uint8_t response[ATECC_RESPONSE_SIZE] = {0};
    size_t read_length = full_response ? (length + 3U) : (length + 1U); // count + data + optional CRC

    if (read_length > sizeof(response)) {
        read_length = sizeof(response);
    }

    // Read response from I2C bus
    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = (uint16_t)read_length,
        .buf   = response
    };
    read_data.msgs  = &read_msg;
    read_data.nmsgs = 1;
    if (ioctl(fd, I2C_RDWR, &read_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        perror("receive_atecc_response: I2C read failed");
        return false;
    }

    uint8_t count = response[0];
    if (count < 4U) {
        fprintf(stderr, "receive_atecc_response: invalid count byte %u\n", count);
        return false;
    }

    if (count == 4U) {
        uint8_t status = response[1];
        if (status != ATECC_STATUS_SUCCESS) {
            fprintf(stderr, "receive_atecc_response: device status 0x%02X\n", status);
        } else {
            fprintf(stderr, "receive_atecc_response: no data returned\n");
        }
        return false;
    }

    size_t available = count - 1U; // subtract count byte
    if (available < length) {
        fprintf(stderr, "receive_atecc_response: expected %zu bytes, got %zu\n", length, available);
        return false;
    }

    memcpy(buffer, &response[1], length);
    return true;
}

/**
 * @brief Wake the ATECC device from sleep
 * 
 * @param fd I2C file descriptor
 * @return true if wake successful, false otherwise
 */
static bool atecc_wake(int fd) {
    uint8_t wake_token[1] = {ATECC_WAKE_TOKEN};

    printf("⏰ Sending wake command...\n");

    struct i2c_rdwr_ioctl_data wake_data = {0};
    struct i2c_msg wake_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = 0,
        .len   = 1,
        .buf   = wake_token
    };
    wake_data.msgs  = &wake_msg;
    wake_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &wake_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        perror("atecc_wake: I2C write failed");
        return false;
    }

    // Wait for device to wake up
    sleep_ms(10);

    // Read wake response
    uint8_t response[4] = {0};
    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = sizeof(response),
        .buf   = response
    };
    read_data.msgs  = &read_msg;
    read_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &read_data) < 0) {
        perror("atecc_wake: I2C read failed");
        return false;
    }

    // Validate wake response
    if (response[0] != 0x04 || response[1] != ATECC_STATUS_WAKE) {
        fprintf(stderr, "atecc_wake: invalid wake response\n");
        fprintf(stderr, "Received: %02X %02X %02X %02X\n", response[0], response[1], response[2], response[3]);
        return false;
    }

    printf("📬 Wake response: ");
    for (size_t i = 0; i < sizeof(response); i++) {
        printf("%02X ", response[i]);
    }
    printf("\n");
    printf("✅ ATECC608A is awake!\n");

    return true;
}

/**
 * @brief Put the ATECC device to sleep
 * 
 * @param fd I2C file descriptor
 * @return true if sleep command successful, false otherwise
 */
static bool atecc_sleep(int fd) {
    uint8_t sleep_cmd = ATECC_CMD_SLEEP;
    struct i2c_rdwr_ioctl_data sleep_data = {0};
    struct i2c_msg sleep_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = 0,
        .len   = 1,
        .buf   = &sleep_cmd
    };
    sleep_data.msgs  = &sleep_msg;
    sleep_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &sleep_data) < 0) {
        perror("atecc_sleep: I2C write failed");
        return false;
    }

    usleep(ATECC_SLEEP_DELAY_US);
    return true;
}

/**
 * @brief Read the serial number from the ATECC device
 * 
 * @param fd I2C file descriptor
 * @param serial_number Buffer to store the serial number (must be at least ATECC_SERIAL_NUMBER_SIZE bytes)
 * @return true if successful, false otherwise
 */
static bool read_atecc_serial_number(int fd, uint8_t *serial_number) {
    if (!serial_number) {
        errno = EINVAL;
        return false;
    }

    uint8_t serial[ATECC_SERIAL_NUMBER_SIZE] = {0};
    uint8_t last_response[4] = {0};

    if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, 0x0000, NULL, 0, NULL, 0)) {
        return false;
    }
    sleep_ms(5);
    
    if (!receive_atecc_response(fd, &serial[0], 4, true)) {
        return false;
    }

    if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, 0x0002, NULL, 0, NULL, 0)) {
        return false;
    }
    sleep_ms(5);

    if (!receive_atecc_response(fd, &serial[4], 4, true)) {
        return false;
    }

    if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, 0x0003, NULL, 0, NULL, 0)) {
        return false;
    }
    sleep_ms(5);

    if (!receive_atecc_response(fd, last_response, 4, true)) {
        return false;
    }

    serial[8] = last_response[0];
    printf("🆔 Serial Number: ");
    for (int i = 0; i < (int)sizeof(serial); i++) {
        printf("%02X", serial[i]);
    }
    printf("\n");

    return true;
}

/**
 * @brief Maps an array of 8 random bytes to a specified range.
 *
 * This function takes an array of 8 random bytes and maps the resulting
 * 64-bit random value to a specified range between min and max (inclusive).
 *
 * @param random_bytes An array of 8 random bytes.
 * @param min The minimum value of the desired range.
 * @param max The maximum value of the desired range.
 * @return A 64-bit random value mapped to the specified range.
 */
static uint64_t map_random_to_range(uint8_t *random_bytes, uint64_t min, uint64_t max) {
    uint64_t random_value = 0;
    for (int i = 0; i < 8; i++) {
        random_value = (random_value << 8) | random_bytes[i];
    }
    return min + (random_value % (max - min + 1));
}

/** 
 * @brief Generate a randome number in range
 * 
 * @param min The minimum value (inclusive)
 * @param max The maximum value (exclusive)
 */
static bool genrate_random_number_in_range(int fd,uint64_t min, uint64_t max) {
    uint8_t resp[32] = {0};
    if (!send_atecc_cmd(fd, ATECC_CMD_RANDOM, 0x00, 0x0000, NULL, 0, NULL, 0)) {
        return false;
    }
    sleep_ms(50);

    if (!receive_atecc_response(fd, resp, sizeof(resp), true)) {
        printf("Failed to receive random number\n");
        return false;
    }

    // Map random value to range
    uint64_t mapped_value = map_random_to_range(&resp[1], min, max);
    printf("🎲 Random number in range %lu-%lu: %lu\n", min, max, mapped_value);
    
    return true;
}

/**
 * @brief Generate a random value of specified length
 * 
 * @param fd I2C file descriptor
 * @param length Length of random value to generate (max 31)
 * @return true if successful, false otherwise
 */
static bool generate_random_value(int fd, uint8_t length) {
    uint8_t resp[32] = {0};
    if (length > sizeof(resp) - 1) {
        errno = EINVAL;
        return false;
    }
    if (!send_atecc_cmd(fd, ATECC_CMD_RANDOM, 0x00, 0x0000, NULL, 0, NULL, 0)) {
        return false;
    }
    sleep_ms(50);

    if (!receive_atecc_response(fd, resp, length, true)) {
        return false;
    }

    printf("🎰 Random Value: ");
    for (uint8_t i = 0; i < length; i++) {
        printf("%02X ", resp[i]);
    }
    printf("\n");

    return true;
}

/**
 * @brief Computes the SHA-256 hash of the given data using the ATECC device.
 * @param fd I2C file descriptor
 * @param data Pointer to the data to hash
 */
static bool compute_sha256(int fd, const uint8_t *data, size_t data_len, uint8_t *output) {
    if (!output || (!data && data_len != 0U)) {
        errno = EINVAL;
        return false;
    }

    if (!send_atecc_cmd(fd, ATECC_CMD_SHA, 0x00, 0x0000, NULL, 0, NULL, 0)) {
        fprintf(stderr, "compute_sha256: SHA start command failed\n");
        return false;
    }
    sleep_ms(5);

    size_t offset = 0U;
    while ((data_len - offset) >= 64U) {
        if (!send_atecc_cmd(fd, ATECC_CMD_SHA, 0x01, 0x0000, &data[offset], (uint8_t)64, NULL, 0)) {
            fprintf(stderr, "compute_sha256: SHA update failed at offset %zu\n", offset);
            return false;
        }
        offset += 64U;
        sleep_ms(5);
    }

    uint8_t remaining = (uint8_t)(data_len - offset);
    const uint8_t *final_block = (remaining > 0U) ? &data[offset] : NULL;
    if (!send_atecc_cmd(fd, ATECC_CMD_SHA, 0x02, (uint16_t)remaining, final_block, remaining, NULL, 0)) {
        fprintf(stderr, "compute_sha256: SHA end command failed\n");
        return false;
    }
    sleep_ms(5);

    uint8_t response[35] = {0};
    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = (uint16_t)sizeof(response),
        .buf   = response
    };
    read_data.msgs = &read_msg;
    read_data.nmsgs = 1;
    if (ioctl(fd, I2C_RDWR, &read_data) < 0) {
        perror("compute_sha256: I2C read failed");
        return false;
    }

    uint8_t count = response[0];
    if (count != 0x23U || count > sizeof(response) || count < 3U) {
        errno = EIO;
        fprintf(stderr, "compute_sha256: invalid response count 0x%02X\n", count);
        return false;
    }

    if (!validate_crc(response, (size_t)count)) {
        fprintf(stderr, "compute_sha256: CRC validation failed\n");
        debug_crc_mismatch(response, (size_t)count, &response[count - 2]);
        return false;
    }

    memcpy(output, &response[1], 32);

    printf("🔒 SHA-256: ");
    for (size_t i = 0; i < 32; i++) {
        printf("%02X", output[i]);
    }
    printf("\n");

    return true;
}

/**
 * @brief Reads the configuration of a specific slot from the ATECC608A device over I2C bus.
 *
 * This function reads the configuration of a specific slot from the ATECC608A device by sending read commands
 * and receiving responses. It then prints the configuration data in hexadecimal format.
 *
 * @param slot The slot number for which to read the configuration.
 * @return true if the slot configuration is successfully read, false otherwise.
 */
bool read_slot_config(int fd, uint8_t slot) {
    uint8_t raw[7] = {0};

    printf("🔎 Checking Slot %d Configuration...\n", slot);

    if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, slot, NULL, 0, NULL, 0)) {
        perror("read_slot_config: I2C write failed");
        return false;
    }
    sleep_ms(20);

    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = (uint16_t)sizeof(raw),
        .buf   = raw
    };
    read_data.msgs  = &read_msg;
    read_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &read_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        perror("read_slot_config: I2C read failed");
        return false;
    }

    size_t count = raw[0];
    if (count > sizeof(raw) || count < 4U) {
        fprintf(stderr, "read_slot_config: invalid count byte 0x%02X\n", raw[0]);
        return false;
    }

    if (!validate_crc(raw, count)) {
        fprintf(stderr, "read_slot_config: CRC validation failed\n");
        debug_crc_mismatch(raw, count, &raw[count - 2]);
        return false;
    }

    printf("🔎 Slot %d Config Data: %02X %02X %02X %02X\n",
           slot, raw[0], raw[1], raw[2], raw[3]);

    return true;
}

/**
 * @brief Reads the configuration data of all slots from the ATECC608A device over I2C bus.
 *
 * This function reads the configuration data of all slots from the ATECC608A device by sending read commands
 * and receiving responses. It then prints the configuration data in hexadecimal format.
 *
 * @return true if the configuration data is successfully read, false otherwise.
 */
bool read_config_zone(int fd) {
    enum { CONFIG_SIZE = 128U, BYTES_PER_BLOCK = 4U, BLOCK_COUNT = CONFIG_SIZE / BYTES_PER_BLOCK };
    uint8_t config_data[CONFIG_SIZE] = {0};

    printf("🔎 Reading Configuration Data...\n");

    for (uint8_t block = 0; block < BLOCK_COUNT; ++block) {
        if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, block, NULL, 0, NULL, 0)) {
            fprintf(stderr, "❌ ERROR: Failed to send read command for block %u\n", block);
            return false;
        }
        sleep_ms(20);

        uint8_t block_data[BYTES_PER_BLOCK] = {0};
        if (!receive_atecc_response(fd, block_data, BYTES_PER_BLOCK, true)) {
            fprintf(stderr, "❌ ERROR: Failed to read configuration for block %u\n", block);
            return false;
        }

        memcpy(&config_data[block * BYTES_PER_BLOCK], block_data, BYTES_PER_BLOCK);
    }

    for (size_t i = 0; i < CONFIG_SIZE; ++i) {
        printf("%02X%s", config_data[i], ((i + 1U) % 16U == 0U) ? "\n" : " ");
    }

    return true;
}

/**
 * @brief Checks the lock status of the ATECC608A device.
 *
 * This function checks the lock status of the ATECC608A device by sending read commands
 * and receiving responses. It then prints the lock status of the device.
 *
 * @return true if the lock status is successfully checked, false otherwise.
 */
bool check_lock_status(int fd) {
    uint8_t raw[7] = {0};
    uint8_t lock_bytes[4] = {0};
    uint8_t expected_address = 0x15;  // Correct address for lock bytes
    
    // 🔹 Send read command for lock status at word address 0x15
    printf("🔍 Checking ATECC608A Lock Status...\n");
    if (!send_atecc_cmd(fd, ATECC_CMD_READ, 0x00, expected_address, NULL, 0, NULL, 0)) {
        printf("❌ ERROR: Failed to send lock status read command!\n");
        return false;
    }

    sleep_ms(23);

    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = (uint16_t)sizeof(raw),
        .buf   = raw
    };
    read_data.msgs = &read_msg;
    read_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &read_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        printf("❌ ERROR: Failed to read lock status response!\n");
        return false;
}

uint8_t count = raw[0];
if (count > sizeof(raw) || count < 4U) {
    printf("❌ ERROR: Invalid lock status response length!\n");
    return false;
}

if (!validate_crc(raw, count)) {
    printf("❌ ERROR: Lock status CRC validation failed!\n");
    debug_crc_mismatch(raw, count, &raw[count - 2]);
    return false;
}

size_t data_len = (size_t)(count - 3U);
if (data_len < sizeof(lock_bytes)) {
    printf("❌ ERROR: Lock status payload too short!\n");
    return false;
}

memcpy(lock_bytes, &raw[1], sizeof(lock_bytes));

printf("🔐 Raw Lock Status Response: ");
for (size_t i = 0; i < count; ++i) {
    printf("%02X ", raw[i]);
}
printf("\n");

uint8_t lock_config = lock_bytes[0];  // Byte 0x15 (Config Lock)
uint8_t lock_value = lock_bytes[1];   // Byte 0x16 (Data Lock)

    printf("🔒 Config Lock Status: %02X\n", lock_config);
    printf("🔒 Data Lock Status: %02X\n", lock_value);

    // 🔐 Determine Lock Status
    if (lock_config == 0x00 && lock_value == 0x00) {
        printf("🔒 Chip is **FULLY LOCKED** (Config & Data).\n");
        return true;
    } 
    else if (lock_config == 0x55 && lock_value == 0x55) {
        printf("🔓 Chip is **UNLOCKED**.\n");
        return true;
    } 
    else if (lock_config == 0x00 && lock_value == 0x55) {
        printf("⚠️ Chip is **PARTIALLY LOCKED** (Config Locked, Data Open).\n");
        return true;
    } 
    else {
        printf("❓ **UNKNOWN LOCK STATE**: Unexpected lock values, possible read error.\n");
        return false;
    }
}

enum {
    AES_BLOCK_SIZE       = 16U,
    AES_RESPONSE_SIZE    = 1U + AES_BLOCK_SIZE + 2U,
    AES_PROCESS_DELAY_MS = 5U
};

bool send_aes_command(int fd, uint8_t mode, uint8_t key_slot, const uint8_t *input_data) {
    if (fd < 0 || !input_data) {
        errno = EINVAL;
        return false;
    }

    if (!send_atecc_cmd(fd, 0x51U, mode, (uint16_t)(key_slot & 0xFFU), input_data, AES_BLOCK_SIZE, NULL, 0)) {
        fprintf(stderr, "send_aes_command: failed to send AES command\n");
        return false;
    }

    return true;
}

bool receive_aes_response(int fd, uint8_t *output_data) {
    if (fd < 0 || !output_data) {
        errno = EINVAL;
        return false;
    }

    uint8_t response[AES_RESPONSE_SIZE] = {0};
    struct i2c_rdwr_ioctl_data read_data = {0};
    struct i2c_msg read_msg = {
        .addr  = ATECC_I2C_ADDRESS,
        .flags = I2C_M_RD,
        .len   = (uint16_t)sizeof(response),
        .buf   = response
    };
    read_data.msgs  = &read_msg;
    read_data.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &read_data) < 0 && errno != EIO && errno != EREMOTEIO) {
        perror("receive_aes_response: I2C read failed");
        return false;
    }

    uint8_t count = response[0];
    if (count < 4U) {
        errno = EIO;
        fprintf(stderr, "receive_aes_response: invalid count byte %u\n", count);
        return false;
    }

    if (count == 4U) {
        errno = EIO;
        fprintf(stderr, "receive_aes_response: device status 0x%02X\n", response[1]);
        return false;
    }

    if (count > sizeof(response)) {
        errno = EIO;
        fprintf(stderr, "receive_aes_response: response length %u exceeds buffer\n", count);
        return false;
    }

    if (count != AES_RESPONSE_SIZE) {
        errno = EIO;
        fprintf(stderr, "receive_aes_response: unexpected response length %u\n", count);
        return false;
    }

    if (!validate_crc(response, count)) {
        errno = EIO;
        fprintf(stderr, "receive_aes_response: CRC validation failed\n");
        debug_crc_mismatch(response, count, &response[count - 2]);
        return false;
    }

    memcpy(output_data, &response[1], AES_BLOCK_SIZE);
    return true;
}

bool aes_encrypt(int fd, const uint8_t *plaintext, uint8_t *ciphertext, uint8_t key_slot) {
    if (fd < 0 || !plaintext || !ciphertext) {
        errno = EINVAL;
        return false;
    }

    if (!send_aes_command(fd, 0x00U, key_slot, plaintext)) {
        fprintf(stderr, "aes_encrypt: AES encrypt command failed\n");
        return false;
    }

    sleep_ms(AES_PROCESS_DELAY_MS);

    if (!receive_aes_response(fd, ciphertext)) {
        fprintf(stderr, "aes_encrypt: AES encrypt response failed\n");
        return false;
    }

    return true;
}

bool aes_decrypt(int fd, const uint8_t *ciphertext, uint8_t *plaintext, uint8_t key_slot) {
    if (fd < 0 || !ciphertext || !plaintext) {
        errno = EINVAL;
        return false;
    }

    if (!send_aes_command(fd, 0x01U, key_slot, ciphertext)) {
        fprintf(stderr, "aes_decrypt: AES decrypt command failed\n");
        return false;
    }

    sleep_ms(AES_PROCESS_DELAY_MS);

    if (!receive_aes_response(fd, plaintext)) {
        fprintf(stderr, "aes_decrypt: AES decrypt response failed\n");
        return false;
    }

    return true;
}

/**
 * @brief Main function for testing ATECC608A communication
 * 
 * @return int Exit status
 */
int main(void) {
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) { perror("open i2c"); return 1; }

    // Set slave (we'll switch as needed inside helper calls)
    if (ioctl(fd, I2C_SLAVE, ATECC_I2C_ADDRESS) < 0) {
        perror("I2C_SLAVE");
        close(fd);
        return 1;
    }

    printf("Waking ATECC608A...\n");
    if (!atecc_wake(fd)) {
        close(fd);
        return 1;
    }
    
    uint8_t serial_number[ATECC_SERIAL_NUMBER_SIZE] = {0};
    if (!read_atecc_serial_number(fd, serial_number)) {
        fprintf(stderr, "❌ ERROR: Failed to read serial number\n");
        close(fd);
        return 1;
    }
    
    if (!genrate_random_number_in_range(fd, 0, 10000000)) {
        fprintf(stderr, "❌ ERROR: Failed to generate random number in range\n");
        close(fd);
        return 1;
    }

    if (!generate_random_value(fd, 16)) {
        fprintf(stderr, "❌ ERROR: Failed to generate random value\n");
        close(fd);
        return 1;
    }

    uint8_t sha_output[32] = {0};
    //const char *data_to_hash = "Hello, ATECC608A!";
    if (!compute_sha256(fd, (const uint8_t *)serial_number, strlen((const char *)serial_number), sha_output)) {
        fprintf(stderr, "❌ ERROR: Failed to compute SHA-256 hash\n");
        close(fd);
        return 1;
    }

    if (!read_slot_config(fd, 3)) {
        fprintf(stderr, "❌ ERROR: Failed to read slot configuration\n");
    }

    if (!read_config_zone(fd)) {
        fprintf(stderr, "❌ ERROR: Failed to read configuration zone\n");
        close(fd);
        return 1;
    }

    if (!check_lock_status(fd)) {
        fprintf(stderr, "❌ ERROR: Failed to check lock status\n");
        close(fd);
        return 1;
    }

    uint8_t plaintext[16] = "Hello, AES!\0\0\0\0";
    uint8_t ciphertext[16] = {0};
    uint8_t decrypted_text[16] = {0};
    uint8_t key_slot = 0x03;

    printf("🔐 Performing AES 128-bit Encryption/Decryption using Slot %d...\n", key_slot);
    printf("🔹 Plaintext: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", plaintext[i]);
    }
    printf("\n");

    if (aes_encrypt(fd, plaintext, ciphertext, key_slot)) {
        printf("🔹 Ciphertext: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", ciphertext[i]);
        }
        printf("\n");
    } else {
        printf("❌ AES 128-bit encryption failed!\n");
        printf("❓ Is the slot configured for AES?\n");
        close(fd);
        return 1;
    }

    if (aes_decrypt(fd, ciphertext, decrypted_text, key_slot)) {
        printf("🔹 Decrypted: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", decrypted_text[i]);
        }
        printf("\n");

        if (memcmp(plaintext, decrypted_text, 16) == 0) {
            printf("✅ AES Decryption Successful! Plaintext Matches!\n");
        } else {
            printf("❌ AES Decryption Failed! Plaintext Mismatch!\n");
        }
    } else {
        printf("❌ AES Decryption Failed!\n");
        close(fd);
        return 1;
    }

    printf("🌙 Putting ATECC608A to sleep...\n");
    if (!atecc_sleep(fd)) {
        fprintf(stderr, "⚠️ Failed to issue sleep command\n");
    }

    printf("🎉 ATECC608A Test Complete!\n");
    close(fd);

    return 0;
}
