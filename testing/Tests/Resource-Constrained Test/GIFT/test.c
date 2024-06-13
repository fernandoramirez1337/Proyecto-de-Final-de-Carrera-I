#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include "GIFT/GIFT.h"

#define DATA_SIZE (1 * 1024 * 1024)  // 2 MB

// Function to get current time in microseconds (for higher precision in embedded systems)
uint64_t get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

// Prints string as hex
static void phex(uint8_t* str) {
    uint8_t len = 16;
    unsigned char i;
    for (i = 0; i < len; ++i)
        printf("%.2x", str[i]);
    printf("\n");
}

void PrintData(const char* title, uint8_t* data, size_t length) {
    printf("%s:\n", title);
    for (size_t i = 0; i < length; i += 16) {
        phex(data + i);
    }
    printf("\n");
}

int main() {
    uint8_t *PlainText = (uint8_t *)malloc(DATA_SIZE);
    uint8_t *CipherText = (uint8_t *)malloc(DATA_SIZE);
    uint8_t *NewPlainText = (uint8_t *)malloc(DATA_SIZE);
    uint8_t MasterKey[16];

    if (!PlainText || !CipherText || !NewPlainText) {
        fprintf(stderr, "Memory allocation failed\n");
        free(PlainText);
        free(CipherText);
        free(NewPlainText);
        return EXIT_FAILURE;
    }

    int file = open("data.bin", O_RDONLY);
    if (file < 0) {
        fprintf(stderr, "Failed to open file\n");
        free(PlainText);
        free(CipherText);
        free(NewPlainText);
        return EXIT_FAILURE;
    }

    ssize_t bytesRead = read(file, PlainText, DATA_SIZE);
    if (bytesRead != DATA_SIZE) {
        fprintf(stderr, "Failed to read file\n");
        close(file);
        free(PlainText);
        free(CipherText);
        free(NewPlainText);
        return EXIT_FAILURE;
    }
    close(file);

    memset(MasterKey, 0, sizeof(MasterKey));

    // Hexadecimal representation of the NIST key
    uint8_t NIST_key[] = {
        0x2b, 0x7e, 0x15, 0x16,
        0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88,
        0x09, 0xcf, 0x4f, 0x3c
    };

    // Copy the key values to MasterKey
    memcpy(MasterKey, NIST_key, sizeof(NIST_key));

    // Print key
    PrintData("Key", (uint8_t*)MasterKey, sizeof(MasterKey));

    // Print first 64 bytes of plaintext
    PrintData("Plaintext (first 64 bytes)", (uint8_t*)PlainText, 64);

    // Encryption
    uint64_t start = get_time_us();

    gift128_encrypt_ecb((uint8_t*)CipherText, (const uint8_t*)PlainText, DATA_SIZE, (const uint8_t*)MasterKey);

    uint64_t end = get_time_us();
    uint64_t encryption_time = end - start;

    // Calculate throughput
    double encryption_throughput = (double)DATA_SIZE / encryption_time;  // MB/s

    // Print first 64 bytes of ciphertext
    PrintData("Ciphertext (first 64 bytes)", (uint8_t*)CipherText, 64);

    // Decryption
    start = get_time_us();

    gift128_decrypt_ecb((uint8_t*)NewPlainText, (const uint8_t*)CipherText, DATA_SIZE, (const uint8_t*)MasterKey);

    end = get_time_us();
    uint64_t decryption_time = end - start;

    // Calculate throughput
    double decryption_throughput = (double)DATA_SIZE / decryption_time;  // MB/s

    // Print first 64 bytes of decrypted plaintext
    PrintData("Decrypted Plaintext (first 64 bytes)", (uint8_t*)NewPlainText, 64);

    // Check if the original plaintext matches the decrypted plaintext
    if (memcmp(PlainText, NewPlainText, DATA_SIZE)) {
        printf("Decryption failed: Plaintext does not match decrypted plaintext.\n");
        free(PlainText);
        free(CipherText);
        free(NewPlainText);
        return EXIT_FAILURE;
    }

    // Print results
    printf("Encryption Time for 2MB data: %" PRIu64 " microseconds\n", encryption_time);
    printf("Encryption Throughput: %.2f MB/s\n", encryption_throughput);
    printf("Decryption Time for 2MB data: %" PRIu64 " microseconds\n", decryption_time);
    printf("Decryption Throughput: %.2f MB/s\n", decryption_throughput);

    free(PlainText);
    free(CipherText);
    free(NewPlainText);

    return EXIT_SUCCESS;
}
