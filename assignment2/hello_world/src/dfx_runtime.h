#ifndef DFX_RUNTIME_H
#define DFX_RUNTIME_H
#include <stdint.h>
int dfx_init(void);
uint8_t dfx_model(void);
uint8_t dfx_state(void);
int dfx_select(uint8_t model, uint32_t *switch_us);
int dfx_infer(uint8_t model, const uint8_t *input, uint16_t length,
              uint8_t *prediction, uint32_t *inference_us, int32_t logits[4]);
#endif
