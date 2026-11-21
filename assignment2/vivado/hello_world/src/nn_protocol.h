#ifndef NN_PROTOCOL_H
#define NN_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

/* Wire format is documented in docs/RUN_SWITCHING.md. */
#define NN_VERSION 1U
#define NN_FIRMWARE_REVISION 2U
#define NN_HEADER_BYTES 8U
#define NN_MAX_PAYLOAD 9600U
#define NN_CNN_BYTES 9600U
#define NN_SNN_BYTES 768U
enum { NN_NONE = 0, NN_CNN = 1, NN_SNN = 2 };
/* NN_ECHO returns the received payload unchanged (UART latency test, after
 * Alessandro's UART_ECHO_TEST); it never touches the accelerator. */
enum { NN_STATUS = 1, NN_SELECT = 2, NN_INFER = 3, NN_INFO = 4, NN_ECHO = 5 };
enum { NN_OK = 0, NN_BAD_COMMAND, NN_BAD_MODEL, NN_BAD_LENGTH,
       NN_BAD_CRC, NN_TIMEOUT, NN_NOT_READY, NN_HW_ERROR, NN_FAULT,
       NN_BAD_VERSION };
enum { NN_UNSELECTED = 0, NN_READY = 1, NN_FAILED = 2 };

uint16_t nn_crc16(const uint8_t *data, size_t length);
uint16_t nn_get16(const uint8_t *p);
uint32_t nn_get32(const uint8_t *p);
void nn_put16(uint8_t *p, uint16_t value);
void nn_put32(uint8_t *p, uint32_t value);
int32_t nn_logit16(uint32_t packed, unsigned high);
uint16_t nn_input_bytes(uint8_t model);
/* Frame excludes the two magic bytes, and includes its trailing CRC. */
int nn_validate(const uint8_t *frame, size_t length);
#endif
