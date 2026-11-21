// Bit-accurate C model of the CNN IP: runs the real ds_cnn() HLS source on
// int16 Q8.8 inputs (exactly what the ARM writes) and prints one prediction per line.
// Usage: harness inputs.bin > preds.txt
#include <cstdio>
#include <cstdint>
#include <vector>
#include "ds_cnn_weights.h"

void ds_cnn(const data_t *input_img, int *pred_out, data_t logits_out[N_CLASSES]);

int main(int argc, char **argv) {
    FILE *f = std::fopen(argv[1], "rb");
    if (!f) { std::perror(argv[1]); return 1; }
    std::vector<int16_t> buf(IMG_H * IMG_W);
    std::vector<data_t> img(IMG_H * IMG_W);
    while (std::fread(buf.data(), sizeof(int16_t), buf.size(), f) == buf.size()) {
        for (size_t i = 0; i < buf.size(); ++i) img[i].range(15, 0) = (uint16_t)buf[i];
        int pred = -1;
        data_t logits[N_CLASSES];
        ds_cnn(img.data(), &pred, logits);
        std::printf("%d", pred);
        for (int k = 0; k < N_CLASSES; ++k) std::printf(" %d", (int)logits[k].range(15, 0).to_int64() << 16 >> 16);
        std::printf("\n");
    }
    std::fclose(f);
    return 0;
}
