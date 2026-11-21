#include "nn_protocol.h"

uint16_t nn_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xffffU;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U));
    }
    return crc;
}

uint16_t nn_get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint32_t nn_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void nn_put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

void nn_put32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8U * i));
}

int32_t nn_logit16(uint32_t packed, unsigned high)
{
    uint32_t value = (packed >> (high ? 16U : 0U)) & 0xffffU;
    return (value & 0x8000U) ? (int32_t)value - 65536 : (int32_t)value;
}

uint16_t nn_input_bytes(uint8_t model)
{
    return model == NN_CNN ? NN_CNN_BYTES : model == NN_SNN ? NN_SNN_BYTES : 0;
}

int nn_validate(const uint8_t *frame, size_t length)
{
    if (length < NN_HEADER_BYTES + 2U) return NN_BAD_LENGTH;
    size_t payload = nn_get16(frame + 6);
    if (payload > NN_MAX_PAYLOAD || length != NN_HEADER_BYTES + payload + 2U)
        return NN_BAD_LENGTH;
    if (nn_crc16(frame, length - 2U) != nn_get16(frame + length - 2U))
        return NN_BAD_CRC;
    if (frame[0] != NN_VERSION) return NN_BAD_VERSION;
    return NN_OK;
}
