#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
const int _MAX_DISTANCE = 5;
const double _INFINITY = DBL_MAX;

static inline __m256d avx2_minkowski_distance(__m256d a, __m256d b, double r) {
    if (r == 1.0) {
        return _mm256_add_pd(a, b);
    } else if (r == 2.0) {
        __m256d a_squared = _mm256_mul_pd(a, a);
        __m256d b_squared = _mm256_mul_pd(b, b);
        __m256d sum_squares = _mm256_add_pd(a_squared, b_squared);
        return _mm256_sqrt_pd(sum_squares);
    } else if (r == _INFINITY) {
      return _mm256_max_pd(a, b);
    } else {
        double a_vals[4], b_vals[4], result[4];
        _mm256_storeu_pd(a_vals, a);
        _mm256_storeu_pd(b_vals, b);

        for (int i = 0; i < 4; i++) {
            result[i] = pow((pow(a_vals[i], r) + pow(b_vals[i], r)), (1.0 / r));
        }

        return _mm256_loadu_pd(result);
    }
}

void floyd_warshall(double **D, int n, double r) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            __m256d a_vec = _mm256_set1_pd(D[i][k]);

            int j = 0;
            for (; j <= n - 4; j += 4) {
                __m256d b_vec = _mm256_loadu_pd(&D[k][j]);
                __m256d c_vec = _mm256_loadu_pd(&D[i][j]);

                __m256d t_vec = avx2_minkowski_distance(a_vec, b_vec, r);
                __m256d result_vec = _mm256_min_pd(c_vec, t_vec);

                _mm256_storeu_pd(&D[i][j], result_vec);
            }

            for (; j < n; j++) {
                double a = D[i][k];
                double b = D[k][j];
                double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

                if (t < D[i][j]) {
                    D[i][j] = t;
                }
            }
        }
    }
}

void blocked_floyd_warshall(double **D, int n, int block_size, double r) {
    int n_blocks = n / block_size;

    double *A = (double *)_mm_malloc(block_size * block_size * sizeof(double), 32);
    double *block_B = (double *)_mm_malloc(block_size * block_size * sizeof(double), 32);
    double *block_C = (double *)_mm_malloc(block_size * block_size * sizeof(double), 32);

    for (int k_block = 0; k_block < n_blocks; k_block++) {

        for (int i = 0; i < block_size; i++) {
            for (int j = 0; j < block_size; j++) {
                A[i * block_size + j] = D[k_block * block_size + i][k_block * block_size + j];
            }
        }

        for (int k = 0; k < block_size; k++) {
            for (int i = 0; i < block_size; i++) {
                __m256d a_vec = _mm256_set1_pd(A[i * block_size + k]);

                int j = 0;
                for (; j <= block_size - 4; j += 4) {
                    __m256d b_vec = _mm256_loadu_pd(&A[k * block_size + j]);
                    __m256d c_vec = _mm256_loadu_pd(&A[i * block_size + j]);

                    __m256d t_vec = avx2_minkowski_distance(a_vec, b_vec, r);
                    __m256d result_vec = _mm256_min_pd(c_vec, t_vec);

                    _mm256_storeu_pd(&A[i * block_size + j], result_vec);
                }

                for (; j < block_size; j++) {
                    double a = A[i * block_size + k];
                    double b = A[k * block_size + j];
                    double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

                    if (t < A[i * block_size + j]) {
                        A[i * block_size + j] = t;
                    }
                }
            }
        }

        for (int i = 0; i < block_size; i++) {
            for (int j = 0; j < block_size; j++) {
                D[k_block * block_size + i][k_block * block_size + j] = A[i * block_size + j];
            }
        }

        for (int j_block = 0; j_block < n_blocks; j_block++) {
            if (j_block == k_block) continue;

            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    block_B[i * block_size + j] = D[k_block * block_size + i][j_block * block_size + j];
                }
            }

            for (int k = 0; k < block_size; k++) {
                for (int i = 0; i < block_size; i++) {
                    __m256d a_vec = _mm256_set1_pd(A[i * block_size + k]);

                    int j = 0;
                    for (; j <= block_size - 4; j += 4) {
                        __m256d b_vec = _mm256_loadu_pd(&block_B[k * block_size + j]);
                        __m256d c_vec = _mm256_loadu_pd(&block_B[i * block_size + j]);

                        __m256d t_vec = avx2_minkowski_distance(a_vec, b_vec, r);
                        __m256d result_vec = _mm256_min_pd(c_vec, t_vec);

                        _mm256_storeu_pd(&block_B[i * block_size + j], result_vec);
                    }

                    for (; j < block_size; j++) {
                        double a = A[i * block_size + k];
                        double b = block_B[k * block_size + j];
                        double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

                        if (t < block_B[i * block_size + j]) {
                            block_B[i * block_size + j] = t;
                        }
                    }
                }
            }

            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    D[k_block * block_size + i][j_block * block_size + j] = block_B[i * block_size + j];
                }
            }
        }

        for (int i_block = 0; i_block < n_blocks; i_block++) {
            if (i_block == k_block) continue;

            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    block_C[i * block_size + j] = D[i_block * block_size + i][k_block * block_size + j];
                }
            }

            for (int k = 0; k < block_size; k++) {
                for (int i = 0; i < block_size; i++) {
                    __m256d a_vec = _mm256_set1_pd(block_C[i * block_size + k]);

                    int j = 0;
                    for (; j <= block_size - 4; j += 4) {
                        __m256d b_vec = _mm256_loadu_pd(&A[k * block_size + j]);
                        __m256d c_vec = _mm256_loadu_pd(&block_C[i * block_size + j]);

                        __m256d t_vec = avx2_minkowski_distance(a_vec, b_vec, r);
                        __m256d result_vec = _mm256_min_pd(c_vec, t_vec);

                        _mm256_storeu_pd(&block_C[i * block_size + j], result_vec);
                    }

                    for (; j < block_size; j++) {
                        double a = block_C[i * block_size + k];
                        double b = A[k * block_size + j];
                        double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

                        if (t < block_C[i * block_size + j]) {
                            block_C[i * block_size + j] = t;
                        }
                    }
                }
            }

            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    D[i_block * block_size + i][k_block * block_size + j] = block_C[i * block_size + j];
                }
            }
        }

        for (int i_block = 0; i_block < n_blocks; i_block++) {
            if (i_block == k_block) continue;

            for (int j_block = 0; j_block < n_blocks; j_block++) {
                if (j_block == k_block) continue;

                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        block_C[i * block_size + j] = D[i_block * block_size + i][j_block * block_size + j];
                    }
                }

                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        block_B[i * block_size + j] = D[i_block * block_size + i][k_block * block_size + j];
                    }
                }

                for (int k = 0; k < block_size; k++) {
                    for (int i = 0; i < block_size; i++) {
                        __m256d a_vec = _mm256_set1_pd(block_B[i * block_size + k]);

                        int j = 0;
                        for (; j <= block_size - 4; j += 4) {
                            __m256d b_vec = _mm256_loadu_pd(&D[k_block * block_size + k][j_block * block_size + j]);
                            __m256d c_vec = _mm256_loadu_pd(&block_C[i * block_size + j]);

                            __m256d t_vec = avx2_minkowski_distance(a_vec, b_vec, r);
                            __m256d result_vec = _mm256_min_pd(c_vec, t_vec);

                            _mm256_storeu_pd(&block_C[i * block_size + j], result_vec);
                        }

                        for (; j < block_size; j++) {
                            double a = block_B[i * block_size + k];
                            double b = D[k_block * block_size + k][j_block * block_size + j];
                            double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

                            if (t < block_C[i * block_size + j]) {
                                block_C[i * block_size + j] = t;
                            }
                        }
                    }
                }

                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        D[i_block * block_size + i][j_block * block_size + j] = block_C[i * block_size + j];
                    }
                }
            }
        }
    }

    _mm_free(A);
    _mm_free(block_B);
    _mm_free(block_C);
}

double **pathfinder_network(double **graph, int n, int q, double r) {
    double **D = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        D[i] = (double *)_mm_malloc(n * sizeof(double), 32);
        memcpy(D[i], graph[i], n * sizeof(double));
    }

    const int L1_CACHE_SIZE = 32 * 1024;
    int block_size = sqrt(L1_CACHE_SIZE / (3 * sizeof(double)));

    block_size = (block_size / 4) * 4;
    if (block_size < 4) block_size = 4;

    if (n % block_size != 0) {
        for (int i = block_size; i >= 4; i -= 4) {
            if (n % i == 0) {
                block_size = i;
                break;
            }
        }

        if (n % block_size != 0) {
            floyd_warshall(D, n, r);
        } else {
            blocked_floyd_warshall(D, n, block_size, r);
        }
    } else {
        blocked_floyd_warshall(D, n, block_size, r);
    }

    for (int i = 0; i < n; i++) {
        int j = 0;
        for (; j <= n - 4; j += 4) {
            __m256d graph_vec = _mm256_loadu_pd(&graph[i][j]);
            __m256d d_vec = _mm256_loadu_pd(&D[i][j]);
            __m256d result_vec = _mm256_min_pd(graph_vec, d_vec);
            _mm256_storeu_pd(&D[i][j], result_vec);
        }

        for (; j < n; j++) {
            if (graph[i][j] < D[i][j]) {
                D[i][j] = graph[i][j];
            }
        }
    }

    return D;
}

double cosine_similarity(double *a, double *b, int n) {
    __m256d dot_vec = _mm256_setzero_pd();
    __m256d norm_a_vec = _mm256_setzero_pd();
    __m256d norm_b_vec = _mm256_setzero_pd();

    int i = 0;
    for (; i <= n - 4; i += 4) {
        __m256d a_vec = _mm256_loadu_pd(&a[i]);
        __m256d b_vec = _mm256_loadu_pd(&b[i]);

        __m256d mul_vec = _mm256_mul_pd(a_vec, b_vec);
        dot_vec = _mm256_add_pd(dot_vec, mul_vec);

        __m256d a_squared = _mm256_mul_pd(a_vec, a_vec);
        __m256d b_squared = _mm256_mul_pd(b_vec, b_vec);
        norm_a_vec = _mm256_add_pd(norm_a_vec, a_squared);
        norm_b_vec = _mm256_add_pd(norm_b_vec, b_squared);
    }

    double dot_arr[4], norm_a_arr[4], norm_b_arr[4];
    _mm256_storeu_pd(dot_arr, dot_vec);
    _mm256_storeu_pd(norm_a_arr, norm_a_vec);
    _mm256_storeu_pd(norm_b_arr, norm_b_vec);

    double dot = dot_arr[0] + dot_arr[1] + dot_arr[2] + dot_arr[3];
    double norm_a = norm_a_arr[0] + norm_a_arr[1] + norm_a_arr[2] + norm_a_arr[3];
    double norm_b = norm_b_arr[0] + norm_b_arr[1] + norm_b_arr[2] + norm_b_arr[3];

    for (; i < n; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    norm_a = sqrt(norm_a);
    norm_b = sqrt(norm_b);

    if (norm_a == 0 || norm_b == 0)
        return 0;

    return dot / (norm_a * norm_b);
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
        *array = realloc(*array, (*size + 1) * sizeof(char *));
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
    printf("PATHFINDER NETWORK (AVX2 Only)\n");
    printf("===============================================\n");

    char buffer[1024];
    char **text = NULL;
    int text_size = 0;

    while (scanf("%s", buffer) == 1) {
        text = realloc(text, (text_size + 1) * sizeof(char *));
        text[text_size] = strdup(buffer);
        text_size++;
    }

    printf("Text size:\t%d\n", text_size);

    clock_t start_time = clock();

    char **wordSet = NULL;
    int wordSetSize = 0;

    for (int i = 0; i < text_size; i++) {
        add_word(&wordSet, &wordSetSize, text[i]);
    }

    qsort(wordSet, wordSetSize, sizeof(char *), compare_strings);

    printf("Unique words:\t%d\n", wordSetSize);

    clock_t wordset_time = clock();
    printf("Word Set:\t%.2f s\n",
           (double)(wordset_time - start_time) / CLOCKS_PER_SEC);

    int n = wordSetSize;

    double **graph = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        graph[i] = (double *)_mm_malloc(n * sizeof(double), 32);
        memset(graph[i], 0, n * sizeof(double));
    }

    for (int i = 0; i < text_size; i++) {
        int token_i = find_index(wordSet, wordSetSize, text[i]);

        int max_neighbor =
            (i + 1 + _MAX_DISTANCE < text_size) ? i + 1 + _MAX_DISTANCE : text_size;
        for (int j = i + 1; j < max_neighbor; j++) {
            int token_j = find_index(wordSet, wordSetSize, text[j]);
            if (token_i != token_j) {
                graph[token_i][token_j]++;
                graph[token_j][token_i]++;
            }
        }
    }

    clock_t graph_time = clock();
    printf("Graph Init:\t%.2f s\n",
           (double)(graph_time - wordset_time) / CLOCKS_PER_SEC);

    double **D = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        D[i] = (double *)_mm_malloc(n * sizeof(double), 32);
        for (int j = 0; j < n; j++) {
            D[i][j] = (i == j) ? 0 : _INFINITY;
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double similarity = cosine_similarity(graph[i], graph[j], n);
            double inverse_similarity;
            if (similarity == 0) {
                inverse_similarity = _INFINITY;
            } else {
                inverse_similarity = 1 - similarity;
            }
            D[i][j] = inverse_similarity;
            D[j][i] = inverse_similarity;
        }
    }

    clock_t similarity_time = clock();
    printf("Similarity:\t%.2f s\n",
           (double)(similarity_time - graph_time) / CLOCKS_PER_SEC);

    const int q = n - 1;
    // const double r = 1;
    // const double r = 2;
    const double r = _INFINITY;

    double **pf_net = pathfinder_network(D, n, q, r);

    clock_t pf_time = clock();
    printf("Pathfinder:\t%.2f s\n",
           (double)(pf_time - similarity_time) / CLOCKS_PER_SEC);
    printf("Total:\t%.2f s\n",
           (double)(pf_time - start_time) / CLOCKS_PER_SEC);
    printf("===============================================\n");
    printf("RESULT\n");
    printf("===============================================\n");

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            printf("%s %s %f\n", wordSet[i], wordSet[j], pf_net[i][j]);
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

    for (int i = 0; i < n; i++) {
        _mm_free(graph[i]);
        _mm_free(D[i]);
        _mm_free(pf_net[i]);
    }
    free(graph);
    free(D);
    free(pf_net);

    return 0;
}