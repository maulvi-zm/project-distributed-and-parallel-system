#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define _MAX_DISTANCE 5
#define _INFINITY DBL_MAX

void blocked_floyd_warshall(double **D, int n, int block_size)
{
    int n_blocks = n / block_size;
    
    int num_threads;
    #pragma omp parallel
    {
        #pragma omp master
        num_threads = omp_get_num_threads();
    }
    
    double *A = (double *)malloc(block_size * block_size * sizeof(double));
    
    double **block_C = (double **)malloc(num_threads * sizeof(double *));
    double **block_A = (double **)malloc(num_threads * sizeof(double *));
    double **block_B = (double **)malloc(num_threads * sizeof(double *));
    
    for (int t = 0; t < num_threads; t++) {
        block_C[t] = (double *)malloc(block_size * block_size * sizeof(double));
        block_A[t] = (double *)malloc(block_size * block_size * sizeof(double));
        block_B[t] = (double *)malloc(block_size * block_size * sizeof(double));
    }
    
    for (int k_block = 0; k_block < n_blocks; k_block++) {
        // Phase 1: Dependent phase
        for (int i = 0; i < block_size; i++) {
            for (int j = 0; j < block_size; j++) {
                A[i * block_size + j] = D[k_block * block_size + i][k_block * block_size + j];
            }
        }
        
        for (int k = 0; k < block_size; k++) {
            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    A[i * block_size + j] = min(A[i * block_size + j], 
                                               A[i * block_size + k] + A[k * block_size + j]);
                }
            }
        }
        
        for (int i = 0; i < block_size; i++) {
            for (int j = 0; j < block_size; j++) {
                D[k_block * block_size + i][k_block * block_size + j] = A[i * block_size + j];
            }
        }
        
        // Phase 2: Partially dependent phase
        #pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            double *thread_C = block_C[thread_id];
            double *thread_B = block_B[thread_id];
            
            #pragma omp for schedule(dynamic)
            for (int j_block = 0; j_block < n_blocks; j_block++) {
                if (j_block == k_block) continue;
                
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        thread_B[i * block_size + j] = D[k_block * block_size + i][j_block * block_size + j];
                    }
                }
                
                for (int k = 0; k < block_size; k++) {
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            thread_B[i * block_size + j] = min(thread_B[i * block_size + j], 
                                                            A[i * block_size + k] + thread_B[k * block_size + j]);
                        }
                    }
                }
                
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        D[k_block * block_size + i][j_block * block_size + j] = thread_B[i * block_size + j];
                    }
                }
            }
            
            #pragma omp for schedule(dynamic)
            for (int i_block = 0; i_block < n_blocks; i_block++) {
                if (i_block == k_block) continue;
                
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        thread_C[i * block_size + j] = D[i_block * block_size + i][k_block * block_size + j];
                    }
                }
                
                for (int k = 0; k < block_size; k++) {
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            thread_C[i * block_size + j] = min(thread_C[i * block_size + j], 
                                                            thread_C[i * block_size + k] + A[k * block_size + j]);
                        }
                    }
                }
                
                for (int i = 0; i < block_size; i++) {
                    for (int j = 0; j < block_size; j++) {
                        D[i_block * block_size + i][k_block * block_size + j] = thread_C[i * block_size + j];
                    }
                }
            }
        }
        #pragma omp barrier
        
        // Phase 3: Independent phase
        #pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            double *thread_C = block_C[thread_id];
            double *thread_A = block_A[thread_id];
            double *thread_B = block_B[thread_id];
            
            #pragma omp for collapse(2) schedule(dynamic)
            for (int i_block = 0; i_block < n_blocks; i_block++) {
                for (int j_block = 0; j_block < n_blocks; j_block++) {
                    if (i_block == k_block || j_block == k_block) continue;
                    
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            thread_C[i * block_size + j] = D[i_block * block_size + i][j_block * block_size + j];
                        }
                    }
                    
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            thread_A[i * block_size + j] = D[i_block * block_size + i][k_block * block_size + j];
                        }
                    }
                    
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            thread_B[i * block_size + j] = D[k_block * block_size + i][j_block * block_size + j];
                        }
                    }
                    
                    for (int k = 0; k < block_size; k++) {
                        for (int i = 0; i < block_size; i++) {
                            for (int j = 0; j < block_size; j++) {
                                thread_C[i * block_size + j] = min(thread_C[i * block_size + j], 
                                                               thread_A[i * block_size + k] + thread_B[k * block_size + j]);
                            }
                        }
                    }
                    
                    for (int i = 0; i < block_size; i++) {
                        for (int j = 0; j < block_size; j++) {
                            D[i_block * block_size + i][j_block * block_size + j] = thread_C[i * block_size + j];
                        }
                    }
                }
            }
        }
    }
    
    free(A);
    
    for (int t = 0; t < num_threads; t++) {
        free(block_C[t]);
        free(block_A[t]);
        free(block_B[t]);
    }
    free(block_C);
    free(block_A);
    free(block_B);
}

void floyd_warshall(double **D, int n)
{
    for (int k = 0; k < n; k++) {
        #pragma omp parallel for collapse(2) schedule(dynamic, 32)
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                D[i][j] = min(D[i][j], D[i][k] + D[k][j]);
            }
        }
    }
}

double **pathfinder_network(double **graph, int n, int q, int r)
{
    double **D = (double **)malloc(n * sizeof(double *));
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        D[i] = (double *)malloc(n * sizeof(double));
        memcpy(D[i], graph[i], n * sizeof(double));
    }
    
    const int L1_CACHE_SIZE = 384 * 1024; // 32KB
    int block_size = sqrt(L1_CACHE_SIZE / (3 * sizeof(double)));
    

    block_size = (block_size / 4) * 4;
    if (block_size < 4) block_size = 4;
    
    if (n % block_size != 0) {
        for (int i = block_size; i >= 1; i--) {
            if (n % i == 0) {
                block_size = i;
                break;
            }
        }
        
        if (n % block_size != 0) {
            floyd_warshall(D, n);
        } else {
            blocked_floyd_warshall(D, n, block_size);
        }
    } else {
        blocked_floyd_warshall(D, n, block_size);
    }
    
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (graph[i][j] < D[i][j]) {
                D[i][j] = graph[i][j];
            }
        }
    }
    
    return D;
}

double cosine_similarity(double *a, double *b, int n)
{
    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;

    for (int i = 0; i < n; i++) {
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

int find_index(char **array, int size, const char *word)
{
    for (int i = 0; i < size; i++) {
        if (strcmp(array[i], word) == 0) {
            return i;
        }
    }
    return -1;
}

int word_exists(char **array, int size, const char *word)
{
    return find_index(array, size, word) != -1;
}

int add_word(char ***array, int *size, const char *word)
{
    if (!word_exists(*array, *size, word)) {
        *array = realloc(*array, (*size + 1) * sizeof(char *));
        (*array)[*size] = strdup(word);
        (*size)++;
        return 1;
    }
    return 0;
}

int compare_strings(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

int main()
{
    printf("===============================================\n");
    printf("PATHFINDER NETWORK\n");
    printf("===============================================\n");

    int num_threads = omp_get_num_procs();
    omp_set_num_threads(num_threads);
    printf("Using %d OpenMP threads\n", num_threads);

    char buffer[1024];
    char **text = NULL;
    int text_size = 0;

    while (scanf("%s", buffer) == 1) {
        text = realloc(text, (text_size + 1) * sizeof(char *));
        text[text_size] = strdup(buffer);
        text_size++;
    }

    printf("Text size:\t%d\n", text_size);

    double wtime = omp_get_wtime();

    char **wordSet = NULL;
    int wordSetSize = 0;

    for (int i = 0; i < text_size; i++) {
        add_word(&wordSet, &wordSetSize, text[i]);
    }

    qsort(wordSet, wordSetSize, sizeof(char *), compare_strings);

    printf("Unique words:\t%d\n", wordSetSize);

    double wtime_wordset = omp_get_wtime();
    printf("Word Set:\t%.2f s\n", 
           wtime_wordset - wtime);

    int n = wordSetSize;

    double **graph = (double **)malloc(n * sizeof(double *));
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        graph[i] = (double *)calloc(n, sizeof(double));
    }

    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic, 100)
        for (int i = 0; i < text_size; i++) {
            int token_i = find_index(wordSet, wordSetSize, text[i]);

            int max_neighbor =
                (i + 1 + _MAX_DISTANCE < text_size) ? i + 1 + _MAX_DISTANCE : text_size;
            for (int j = i + 1; j < max_neighbor; j++) {
                int token_j = find_index(wordSet, wordSetSize, text[j]);
                if (token_i != token_j) {
                    #pragma omp atomic
                    graph[token_i][token_j]++;
                    #pragma omp atomic
                    graph[token_j][token_i]++;
                }
            }
        }
    }

    double wtime_graph = omp_get_wtime();
    printf("Graph Init:\t%.2f s\n", 
           wtime_graph - wtime_wordset);

    double **D = (double **)malloc(n * sizeof(double *));
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        D[i] = (double *)malloc(n * sizeof(double));
        D[i][i] = 0;
    }

    #pragma omp parallel for schedule(dynamic)
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

    double wtime_similarity = omp_get_wtime();
    printf("Similarity:\t%.2f s\n",
           wtime_similarity - wtime_graph);

    const int q = n - 1;
    const double r = 1;

    double **pf_net = pathfinder_network(D, n, q, r);

    double wtime_pf = omp_get_wtime();
    printf("Pathfinder:\t%.2f s\n", 
           wtime_pf - wtime_similarity);
    printf("Total:\t%.2f s\n", 
           wtime_pf - wtime);
    printf("===============================================\n");
    printf("RESULT\n");
    printf("===============================================\n");

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            printf("%s %s %f\n", wordSet[i], wordSet[j], pf_net[i][j]);
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < text_size; i++) {
        free(text[i]);
    }
    free(text);

    #pragma omp parallel for
    for (int i = 0; i < wordSetSize; i++) {
        free(wordSet[i]);
    }
    free(wordSet);

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        free(graph[i]);
        free(D[i]);
        free(pf_net[i]);
    }
    free(graph);
    free(D);
    free(pf_net);

    return 0;
}