#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "polaris.h"

static inline unsigned int fill_float_randomly_seed(float *ptr, int size, float min, float max) {
    int i = 0;
    unsigned int seed = (unsigned int)time(NULL);
    unsigned int seed_bkp = seed;
    float delta = max - min;
    for (i = 0; i < size; ++i) {
        ptr[i] = min + ((float)rand_r(&seed) / (float)RAND_MAX) * delta;
    }

    return seed_bkp;
}

void print_usage() {
    printf("Usage: gemm <m> <n> <k>\n");
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage();
        return -EINVAL;
    }

    int ret = 0;

    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);

    PolarisContext* ctxt = polaris_create_context(0);
    float* mat_a_fpga = NULL;
    float* mat_b_fpga = NULL;
    float* mat_c_fpga = NULL;
    float* mat_bias_fpga = NULL;
    float* mat_a_cpu = (float*) malloc(m * k * sizeof(float));
    float* mat_b_cpu = (float*) malloc(n * k * sizeof(float));
    float* mat_c_cpu = (float*) malloc(m * n * sizeof(float));
    float* mat_bias_cpu = (float*) malloc(n * sizeof(float));

    polaris_malloc(ctxt, m * k * sizeof(float), (void**)&mat_a_fpga);
    polaris_malloc(ctxt, n * k * sizeof(float), (void**)&mat_b_fpga);
    polaris_malloc(ctxt, m * n * sizeof(float), (void**)&mat_c_fpga);
    polaris_malloc(ctxt, n * sizeof(float), (void**)&mat_bias_fpga);

    fill_float_randomly_seed(mat_a_cpu, m * k, -1.0f, 1.0f);
    fill_float_randomly_seed(mat_b_cpu, n * k, -1.0f, 1.0f);
    fill_float_randomly_seed(mat_c_cpu, m * n, 0.0f, 0.0f);
    fill_float_randomly_seed(mat_bias_cpu, n, -1.0f, 1.0f);

    polaris_memcpy(ctxt, POLARIS_HOST_TO_DEVICE,
                   mat_a_fpga, mat_a_cpu, m * k * sizeof(float));
    polaris_memcpy(ctxt, POLARIS_HOST_TO_DEVICE,
                   mat_b_fpga, mat_b_cpu, n * k * sizeof(float));
    polaris_memcpy(ctxt, POLARIS_HOST_TO_DEVICE,
                   mat_c_fpga, mat_c_cpu, m * n * sizeof(float));
    polaris_memcpy(ctxt, POLARIS_HOST_TO_DEVICE,
                   mat_bias_fpga, mat_bias_cpu, n * sizeof(float));

    polaris_gemm(ctxt, m, n, k,
                 mat_a_fpga, mat_b_fpga, mat_c_fpga, mat_bias_fpga);

    polaris_memcpy(ctxt, POLARIS_DEVICE_TO_HOST,
                   mat_c_cpu, mat_c_fpga, m * n * sizeof(float));

    polaris_free(ctxt, mat_a_fpga);
    polaris_free(ctxt, mat_b_fpga);
    polaris_free(ctxt, mat_c_fpga);
    polaris_free(ctxt, mat_bias_fpga);

    free(mat_a_cpu);
    free(mat_b_cpu);
    free(mat_c_cpu);
    free(mat_bias_cpu);

    return ret;
}
