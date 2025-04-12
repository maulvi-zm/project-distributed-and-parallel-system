#include <float.h>
#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const double _INFINITY = DBL_MAX;
const int _MAX_DISTANCE = 5;

double horizontal_sum(__m256d v) {
  double sum[4];
  _mm256_storeu_pd(sum, v);
  return sum[0] + sum[1] + sum[2] + sum[3];
}

void update_row(double **D, const int i, const int n, const int k,
                const double r) {
  if (r == _INFINITY) {
    __m256d a_vec = _mm256_set1_pd(D[i][k]);
    int j = 0;

    for (; j <= n - 4; j += 4) {
      __m256d j_indices = _mm256_set_pd((double)(j + 3), (double)(j + 2),
                                        (double)(j + 1), (double)j);
      __m256d i_vec = _mm256_set1_pd((double)i);
      __m256d mask = _mm256_cmp_pd(j_indices, i_vec, _CMP_NEQ_OQ);

      __m256d b_vec = _mm256_loadu_pd(&D[k][j]);
      __m256d t_vec = _mm256_max_pd(a_vec, b_vec);

      __m256d current_d_vec = _mm256_loadu_pd(&D[i][j]);
      __m256d new_d_vec = _mm256_min_pd(current_d_vec, t_vec);

      __m256d final_d_vec = _mm256_blendv_pd(current_d_vec, new_d_vec, mask);

      _mm256_storeu_pd(&D[i][j], final_d_vec);
    }

    for (; j < n; j++) {
      if (i == j)
        continue;

      double a = D[i][k];
      double b = D[k][j];
      double t = fmax(a, b);

      if (t < D[i][j]) {
        D[i][j] = t;
      }
    }
  } else {
    for (int j = 0; j < n; j++) {
      if (i == j)
        continue;

      double a = D[i][k];
      double b = D[k][j];
      if (a < 0 || b < 0) {
        if (a < 0)
          a = 0;
        if (b < 0)
          b = 0;
      }

      double t;
      if (isinf(r) || r > 700) {
        t = fmax(a, b);
      } else {
        double pow_a = (a == 0 && r > 0) ? 0 : pow(a, r);
        double pow_b = (b == 0 && r > 0) ? 0 : pow(b, r);
        t = pow(pow_a + pow_b, 1.0 / r);
      }

      if (t < D[i][j]) {
        D[i][j] = t;
      }
    }
  }
}

void floyd_warshall(double **D, int q, int r) {
  int n = q + 1;
  for (int k = 0; k < n; k++) {
    for (int i = 0; i < n; i++) {
      update_row(D, i, n, k, r);
    }
  }
}

double **pathfinder_network(double **graph, int n, int q, int r) {
  double **D = (double **)malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    D[i] = (double *)malloc(n * sizeof(double));
    memcpy(D[i], graph[i], n * sizeof(double));
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
  __m256d dot_vec = _mm256_setzero_pd();
  __m256d norm_a_vec = _mm256_setzero_pd();
  __m256d norm_b_vec = _mm256_setzero_pd();

  int i = 0;
  for (; i <= n - 4; i += 4) {
    __m256d a_vec = _mm256_loadu_pd(&a[i]);
    __m256d b_vec = _mm256_loadu_pd(&b[i]);

    dot_vec = _mm256_fmadd_pd(a_vec, b_vec, dot_vec);

    norm_a_vec = _mm256_fmadd_pd(a_vec, a_vec, norm_a_vec);

    norm_b_vec = _mm256_fmadd_pd(b_vec, b_vec, norm_b_vec);
  }

  double dot = horizontal_sum(dot_vec);
  double norm_a_sq = horizontal_sum(norm_a_vec);
  double norm_b_sq = horizontal_sum(norm_b_vec);

  for (; i < n; i++) {
    dot += a[i] * b[i];
    norm_a_sq += a[i] * a[i];
    norm_b_sq += b[i] * b[i];
  }

  double norm_a = sqrt(norm_a_sq);
  double norm_b = sqrt(norm_b_sq);

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
    char **new_array = (char **)realloc(*array, (*size + 1) * sizeof(char *));
    if (!new_array) {
      perror("Failed to realloc memory for word array");
      exit(EXIT_FAILURE);
    }
    *array = new_array;

    (*array)[*size] = strdup(word);
    if (!(*array)[*size]) {
      perror("Failed to duplicate word string");
      exit(EXIT_FAILURE);
    }
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
  printf("PATHFINDER NETWORK (AVX2 Version)\n"); // Judul diubah
  printf("===============================================\n");

  char buffer[1024];
  char **text = NULL;
  int text_size = 0;

  while (scanf("%1023s", buffer) == 1) {
    char **new_text = (char **)realloc(text, (text_size + 1) * sizeof(char *));
    if (!new_text) {
      perror("Failed to realloc memory for text");
      for (int i = 0; i < text_size; ++i)
        free(text[i]);
      free(text);
      return 1;
    }
    text = new_text;
    text[text_size] = strdup(buffer);
    if (!text[text_size]) {
      perror("Failed to duplicate text word");
      for (int i = 0; i < text_size; ++i)
        free(text[i]);
      free(text);
      return 1;
    }
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
  printf("Word Set:\t%.2f s\n", (double)(wordSetEnd - start) / CLOCKS_PER_SEC);

  int n = wordSetSize;

  double **graph = (double **)malloc(n * sizeof(double *));
  if (!graph) {
    perror("Malloc failed for graph rows");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n; i++) {
    graph[i] = (double *)calloc(n, sizeof(double));
    if (!graph[i]) {
      perror("Calloc failed for graph columns");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < text_size; i++) {
    int token_i = find_index(wordSet, wordSetSize, text[i]);
    if (token_i == -1)
      continue;

    int max_neighbor =
        (i + _MAX_DISTANCE + 1 < text_size) ? i + _MAX_DISTANCE + 1 : text_size;
    for (int j = i + 1; j < max_neighbor; j++) {
      int token_j = find_index(wordSet, wordSetSize, text[j]);
      if (token_j == -1)
        continue;

      if (token_i != token_j) {
        graph[token_i][token_j]++;
        graph[token_j][token_i]++;
      }
    }
  }

  clock_t graphInitEnd = clock();
  printf("Graph Init:\t%.2f s\n",
         (double)(graphInitEnd - wordSetEnd) / CLOCKS_PER_SEC);

  double **D = (double **)malloc(n * sizeof(double *));
  if (!D) {
    perror("Malloc failed for D rows");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n; i++) {
    D[i] = (double *)malloc(n * sizeof(double));
    if (!D[i]) {
      perror("Malloc failed for D columns");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < n; i++) {
    D[i][i] = 0.0;
    for (int j = i + 1; j < n; j++) {
      double similarity = cosine_similarity(graph[i], graph[j], n);
      double inverse_similarity =
          (similarity <= 0) ? _INFINITY : (1.0 - similarity);
      D[i][j] = inverse_similarity;
      D[j][i] = inverse_similarity;
    }
  }

  clock_t similarityEnd = clock();
  printf("Similarity:\t%.2f s\n",
         (double)(similarityEnd - graphInitEnd) / CLOCKS_PER_SEC);

  const int q = n - 1;
  const double r = _INFINITY;

  double **pf_net = pathfinder_network(D, n, q, r);

  clock_t pfEnd = clock();
  printf("Pathfinder:\t%.2f s\n",
         (double)(pfEnd - similarityEnd) / CLOCKS_PER_SEC);
  printf("Total:\t\t%.2f s\n", (double)(pfEnd - start) / CLOCKS_PER_SEC);
  printf("===============================================\n");
  printf("RESULT\n");
  printf("===============================================\n");

  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (pf_net[i][j] >= _INFINITY) {
        printf("%s %s inf\n", wordSet[i], wordSet[j]);
      } else {
        printf("%s %s %f\n", wordSet[i], wordSet[j], pf_net[i][j]);
      }
    }
  }

  printf("Cleaning up memory...\n");

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

  printf("Cleanup complete.\n");

  return 0;
}
