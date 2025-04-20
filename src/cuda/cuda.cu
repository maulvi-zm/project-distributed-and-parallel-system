#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cuda_runtime.h>

const double _INFINITY = DBL_MAX;
const int _MAX_DISTANCE = 5;
const int BLOCK_SIZE = 64;

void cudaCheckError() {
    cudaError_t e = cudaGetLastError();
    if (e != cudaSuccess) { \
        printf("CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
        exit(EXIT_FAILURE); \
    } \
}

void printDeviceInfo() {
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("CUDA Device: %s\n", prop.name);
    printf("Compute Capability: %d.%d\n", prop.major, prop.minor);
    printf("Max threads per block: %d\n", prop.maxThreadsPerBlock);
    printf("Max threads dim: (%d, %d, %d)\n", prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
    printf("Max grid size: (%d, %d, %d)\n", prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
}

__device__ double atomicMinDouble(double* address, double val) {
    unsigned long long int* address_as_ull = (unsigned long long int*)address;
    unsigned long long int old = *address_as_ull, assumed;

    do {
        assumed = old;
        old = atomicCAS(address_as_ull, assumed, __double_as_longlong(min(val, __longlong_as_double(assumed))));
    } while (assumed != old);

    return __longlong_as_double(old);
}

__global__ void cosine_similarity_kernel(double *graph, double *D, int n) {
    const double _INFINITY = DBL_MAX;
    int i = blockIdx.x;
    int j = threadIdx.x + blockIdx.y * blockDim.x;

    if (j <= i || j >= n)
        return;

    double dot = 0.0, norm_i = 0.0, norm_j = 0.0;

    for (int k = 0; k < n; k++) {
        dot += graph[i * n + k] * graph[j * n + k];
        norm_i += graph[i * n + k] * graph[i * n + k];
        norm_j += graph[j * n + k] * graph[j * n + k];
    }

    norm_i = sqrt(norm_i);
    norm_j = sqrt(norm_j);

    double similarity;
    if (norm_i == 0 || norm_j == 0)
        similarity = 0;
    else
        similarity = dot / (norm_i * norm_j);

    double inverse_similarity = (similarity == 0) ? _INFINITY : 1 - similarity;

    D[i * n + j] = inverse_similarity;
    D[j * n + i] = inverse_similarity;
}

__global__ void fw_kernel(double *D, int n, int k, double r) {
    int i = blockIdx.x;
    int j = threadIdx.x + blockIdx.y * blockDim.x;

    if (i >= n || j >= n || i == j)
        return;

    double a = D[i * n + k];
    double b = D[k * n + j];

    if (a == DBL_MAX || b == DBL_MAX)
        return;

    double t;
    if (r == 1.0) {
        t = a + b;
    } else if (r == 2.0) {
        t = sqrt(a*a + b*b);
    } else if (r == DBL_MAX) {
        t = fmax(a, b);
    } else {
        t = pow((pow(a, r) + pow(b, r)), (1.0 / r));
    }

    if (t < D[i * n + j]) {
        atomicMinDouble(&D[i * n + j], t);
    }
}

void floyd_warshall_cuda(double *D, int n, double r) {
    double *d_D;
    size_t size = n * n * sizeof(double);

    cudaMalloc(&d_D, size);
    cudaMemcpy(d_D, D, size, cudaMemcpyHostToDevice);
    cudaCheckError();

    dim3 blockDim(BLOCK_SIZE);
    dim3 gridDim(n, (n + BLOCK_SIZE - 1) / BLOCK_SIZE);

    for (int k = 0; k < n; k++) {
        fw_kernel<<<gridDim, blockDim>>>(d_D, n, k, r);
        cudaDeviceSynchronize();
        cudaCheckError();
    }

    cudaMemcpy(D, d_D, size, cudaMemcpyDeviceToHost);
    cudaCheckError();

    cudaFree(d_D);
    cudaCheckError();
}

int find_index(char **array, int size, const char *word) {
    for (int i = 0; i < size; i++) {
        if (strcmp(array[i], word) == 0) {
            return i;
        }
    }
    return -1;
}

int word_exists(char **array, int size, const char *word) {
    return find_index(array, size, word) != -1;
}

int add_word(char ***array, int *size, const char *word) {
    if (!word_exists(*array, *size, word)) {
        *array = (char **)realloc(*array, (*size + 1) * sizeof(char *));
        (*array)[*size] = strdup(word);
        (*size)++;
        return 1;
    }
    return 0;
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main() {
    printf("===============================================\n");
    printf("PATHFINDER NETWORK (CUDA Naive Implementation)\n");
    printf("===============================================\n");

    printDeviceInfo();

    char buffer[1024];
    char **text = NULL;
    int text_size = 0;

    while (scanf("%s", buffer) == 1) {
        text = (char **)realloc(text, (text_size + 1) * sizeof(char *));
        text[text_size] = strdup(buffer);
        text_size++;
    }

    printf("Text size:\t%d\n", text_size);

    clock_t start = clock();

    char **wordSet = NULL;
    int wordSetSize = 0;

    for (int i = 0; i < text_size; i++) {
        add_word(&wordSet, &wordSetSize, text[i]);
    }

    qsort(wordSet, wordSetSize, sizeof(char *), compare_strings);

    printf("Unique words:\t%d\n", wordSetSize);

    clock_t wordSetEnd = clock();
    printf("Word Set:\t%ld ms\n", (wordSetEnd - start) * 1000 / CLOCKS_PER_SEC);

    int n = wordSetSize;

    double *graph = (double *)calloc(n * n, sizeof(double));
    for (int i = 0; i < text_size; i++) {
        int token_i = find_index(wordSet, wordSetSize, text[i]);
        int max_neighbor = (i + 1 + _MAX_DISTANCE < text_size) ? i + 1 + _MAX_DISTANCE : text_size;
        for (int j = i + 1; j < max_neighbor; j++) {
            int token_j = find_index(wordSet, wordSetSize, text[j]);
            if (token_i != token_j) {
                graph[token_i * n + token_j]++;
                graph[token_j * n + token_i]++;
            }
        }
    }

    clock_t graphInitEnd = clock();
    printf("Graph Init:\t%ld ms\n", (graphInitEnd - wordSetEnd) * 1000 / CLOCKS_PER_SEC);

    double *D = (double *)malloc(n * n * sizeof(double));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                D[i * n + j] = 0;
            } else {
                D[i * n + j] = _INFINITY;
            }
        }
    }

    double *d_graph, *d_D;
    size_t size = n * n * sizeof(double);

    cudaMalloc(&d_graph, size);
    cudaMalloc(&d_D, size);

    cudaMemcpy(d_graph, graph, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_D, D, size, cudaMemcpyHostToDevice);
    cudaCheckError();

    dim3 gridDim(n, (n + BLOCK_SIZE - 1) / BLOCK_SIZE);
    dim3 blockDim(BLOCK_SIZE);

    cosine_similarity_kernel<<<gridDim, blockDim>>>(d_graph, d_D, n);
    cudaDeviceSynchronize();
    cudaCheckError();

    cudaMemcpy(D, d_D, size, cudaMemcpyDeviceToHost);
    cudaFree(d_graph);
    cudaFree(d_D);
    cudaCheckError();

    clock_t similarityEnd = clock();
    printf("Similarity:\t%ld ms\n", (similarityEnd - graphInitEnd) * 1000 / CLOCKS_PER_SEC);

    // const double r = 1;
    // const double r = 2;
    const double r = _INFINITY;

    floyd_warshall_cuda(D, n, r);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j && graph[i * n + j] > 0 && graph[i * n + j] < D[i * n + j]) {
                D[i * n + j] = graph[i * n + j];
            }
        }
    }

    clock_t pfEnd = clock();
    printf("Pathfinder:\t%ld ms\n", (pfEnd - similarityEnd) * 1000 / CLOCKS_PER_SEC);
    printf("Total:\t%ld ms\n", (pfEnd - start) * 1000 / CLOCKS_PER_SEC);
    printf("===============================================\n");
    printf("RESULT\n");
    printf("===============================================\n");

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (D[i * n + j] == _INFINITY) {
                printf("%s %s inf\n", wordSet[i], wordSet[j]);
            } else {
                printf("%s %s %f\n", wordSet[i], wordSet[j], D[i * n + j]);
            }
        }
    }

    for (int i = 0; i < text_size; i++) {
        free(text[i]);
    }
    free(text);

    for (int i = 0; i < wordSetSize; i++) {
        free(wordSet[i]);
    }
    free(wordSet);

    free(graph);
    free(D);

    return 0;
}