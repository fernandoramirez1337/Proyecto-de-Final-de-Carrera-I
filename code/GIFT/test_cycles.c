#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <papi.h>
#include "GIFT/GIFT.h"

#define DATA_SIZE (1 * 1024 * 1024)

uint64_t get_time_us() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

int main() {
  uint8_t *PlainText = (uint8_t *)malloc(DATA_SIZE);
  uint8_t *CipherText = (uint8_t *)malloc(DATA_SIZE);
  uint8_t MasterKey[16];

  if (!PlainText || !CipherText) {
    fprintf(stderr, "Memory allocation failed\n");
    free(PlainText);
    free(CipherText);
    return EXIT_FAILURE;
  }

  int file = open("data.bin", O_RDONLY);
  if (file < 0) {
    fprintf(stderr, "Failed to open file\n");
    free(PlainText);
    free(CipherText);
    return EXIT_FAILURE;
  }

  ssize_t bytesRead = read(file, PlainText, DATA_SIZE);
  if (bytesRead != DATA_SIZE) {
    fprintf(stderr, "Failed to read file\n");
    close(file);
    free(PlainText);
    free(CipherText);
    return EXIT_FAILURE;
  }
  close(file);

  memset(MasterKey, 0, sizeof(MasterKey));

  uint8_t NIST_key[] = {
    0x2b, 0x7e, 0x15, 0x16,
    0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,
    0x09, 0xcf, 0x4f, 0x3c
  };

  memcpy(MasterKey, NIST_key, sizeof(NIST_key));


  int retval;
  int EventSet = PAPI_NULL;
  long long values[2];

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "PAPI library init error!\n");
    exit(1);
  }

  retval = PAPI_create_eventset(&EventSet);
  if (retval != PAPI_OK) {
    fprintf(stderr, "PAPI create event set error!\n");
    exit(1);
  }

  retval = PAPI_add_event(EventSet, PAPI_TOT_CYC);
  if (retval != PAPI_OK) {
    fprintf(stderr, "PAPI add event error!\n");
    exit(1);
  }

  retval = PAPI_add_event(EventSet, PAPI_TOT_INS);
  if (retval != PAPI_OK) {
    fprintf(stderr, "PAPI add event error!\n");
    exit(1);
  }

  uint64_t start = get_time_us();

  retval = PAPI_start(EventSet);
  if (retval != PAPI_OK) {
    fprintf(stderr, "PAPI start error!\n");
    exit(1);
  }

  gift128_encrypt_ecb((uint8_t*)CipherText, (const uint8_t*)PlainText, DATA_SIZE, (const uint8_t*)MasterKey);

  retval = PAPI_stop(EventSet, values);
  if (retval != PAPI_OK) {
    fprintf(stderr, "PAPI stop error!\n");
    exit(1);
  }

  uint64_t end = get_time_us();
  uint64_t encryption_time = end - start;

  double seconds = (double)encryption_time / 1000000.0; // Conversion from microseconds to seconds
  double encryption_throughput = (double)DATA_SIZE / seconds;  // Bytes per secord
  encryption_throughput /= (1024*1024); // MB per secord

  //Calculate latency
  size_t total_blocks = 131072;
  double total_cycles = values[0];
  double total_instructions = values[1];
  double cycles_per_block = total_cycles / total_blocks;

  //Calculate efficiency
  double code_size_kB = 4.5 + 5.6;   // Code size in kB
  double throughput_kbps = encryption_throughput * 8192; // Conversion factor: 1 MB = 8192 kb
  double efficiency = throughput_kbps / code_size_kB;

  //Calculate Code_Size×Cycle_Count/Block_Size
  double s_metric = total_cycles * code_size_kB / 128;


  // Print results
  printf("\n\n\tGIFT-128 only encryption\n\n");
  printf("Exec. Time for 2MB plaintext:\t %" PRIu64 " ms\n", encryption_time);
  printf("Throughput:\t\t\t %.2f MB/s\n", encryption_throughput);
  printf("Throughput:\t\t\t %.2f kb/s\n", throughput_kbps);
  printf("Total Cycles:\t\t\t %.f\n", total_cycles);
  printf("Latency (Cycles per block):\t %.f\n", cycles_per_block);
  printf("Total Instructions:\t\t %.f\n", total_instructions);
  printf("Efficiency:\t\t\t %.2f\n", efficiency);
  printf("Code_Size*Cycle_Count/Block_Size:%.f\n", s_metric);
  free(PlainText);
  free(CipherText);

  return EXIT_SUCCESS;
}
