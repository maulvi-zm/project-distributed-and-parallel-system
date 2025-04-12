#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cuda_runtime.h>

const double _INFINITY = DBL_MAX;
const int _MAX_DISTANCE = 5;

void update_row(double **D, const int i, const int n, const int k,
                const double r) {
  for (int j = 0; j < n; j++) {
    if (i == j)
      continue;

    double a = D[i][k];
    double b = D[k][j];
    double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

    if (t < D[i][j]) {
      D[i][j] = t;
    }
  }
}

__global__ void update_row_kernel(double *D, int n, int i, int k, double r){
  int j = blockIdx.x * blockDim.x + threadIdx.x;

  if (j < n && i != j){
    double a = D[i * n + k];
    double b = D[k * n + j];
    double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));
    if (t < D[i * n + j]) {
      D[i * n + j] = t;
    }
  }
}

void floyd_warshall(double **D, int q, int r){
  int n = q + 1;
  double *d_D;
  cudaMalloc((void **)&d_D, n * n * sizeof(double));

  double *D_flat = (double *) malloc (n * n * sizeof(double));

  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++) {
      D_flat[i * n + j] = D[i][j];
    }
  }

  for (int i = 0; i < n; i++){ // isunya sama kek mpi, klo ga di-copy per row, ngecrash
    cudaMemcpy(d_D + i * n, D_flat + i * n, n * sizeof(double), cudaMemcpyHostToDevice);
  }

  for (int k = 0; k < n; k++) {
    for (int i = 0; i < n; i++) {
      int blockSize = 256;
      int numBlocks = (n + blockSize - 1) / blockSize;
      update_row_kernel<<<numBlocks, blockSize>>>(d_D, n, i, k, r);
    }
    cudaDeviceSynchronize();
  }

  
  for (int i = 0; i < n; i++){
    cudaMemcpy(D[i], d_D + i * n, n * sizeof(double), cudaMemcpyDeviceToHost);
  }
  cudaFree(d_D);
}

double **pathfinder_network(double **graph, int n, int q, int r) {
  double **D = (double **)malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    D[i] = (double *)malloc(n * sizeof(double));
    for (int j = 0; j < n; j++) {
      D[i][j] = graph[i][j];
    }
  }

  floyd_warshall(D, q, r);

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (graph[i][j] < D[i][j]) {
        D[i][j] = graph[i][j];
      }
    }
  }

  return D;
}

double cosine_similarity(double *a, double *b, int n) {
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
    *array = (char**) realloc(*array, (*size + 1) * sizeof(char *));
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
  printf("PATHFINDER NETWORK\n");
  printf("===============================================\n");

  char buffer[1024];
  char **text = NULL;
  int text_size = 0;

  while (scanf("%s", buffer) == 1) {
    text = (char**) realloc(text, (text_size + 1) * sizeof(char *));
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
  printf("Word Set:\t%ld s\n", (wordSetEnd - start) / CLOCKS_PER_SEC);

  int n = wordSetSize;

  double **graph = (double **)malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    graph[i] = (double *)calloc(n, sizeof(double));
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

  clock_t graphInitEnd = clock();
  printf("Graph Init:\t%ld s\n", (graphInitEnd - wordSetEnd) / CLOCKS_PER_SEC);

  double **D = (double **)malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    D[i] = (double *)malloc(n * sizeof(double));
  }

  for (int i = 0; i < n; i++) {
    D[i][i] = 0;
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

  clock_t similarityEnd = clock();
  printf("Similarity:\t%ld s\n",
         (similarityEnd - graphInitEnd) / CLOCKS_PER_SEC);

  const int q = n - 1;
  const double r = 1;

  double **pf_net = pathfinder_network(D, n, q, r);

  clock_t pfEnd = clock();
  printf("Pathfinder:\t%ld s\n", (pfEnd - similarityEnd) / CLOCKS_PER_SEC);
  printf("Total:\t%ld s\n", (pfEnd - start) / CLOCKS_PER_SEC);
  printf("===============================================\n");
  printf("RESULT\n");
  printf("===============================================\n");

  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (pf_net[i][j] == _INFINITY) {
        printf("%s %s inf\n", wordSet[i], wordSet[j]);
      } else {
        printf("%s %s %f\n", wordSet[i], wordSet[j], pf_net[i][j]);
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
