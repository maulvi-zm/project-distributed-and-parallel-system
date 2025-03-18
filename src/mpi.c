#include <float.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

void floyd_warshall_parallel(double **D, int q, int r) {
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int n = q + 1;

  double *k_row = (double *)malloc(n * sizeof(double));

  for (int k = 0; k < n; k++) {
    if (rank == 0) {
      for (int j = 0; j < n; j++) {
        k_row[j] = D[k][j];
      }
    }

    MPI_Bcast(k_row, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int rows_per_proc = n / size;
    int remainder = n % size;

    int start_row =
        rank * rows_per_proc + (rank < remainder ? rank : remainder);
    int end_row = start_row + rows_per_proc + (rank < remainder ? 1 : 0);

    for (int i = start_row; i < end_row; i++) {
      for (int j = 0; j < n; j++) {
        if (i == j)
          continue;

        double a = D[i][k];
        double b = k_row[j];
        double t = pow((pow(a, r) + pow(b, r)), (1.0 / r));

        if (t < D[i][j]) {
          D[i][j] = t;
        }
      }
    }

    for (int i = 0; i < size; i++) {
      int proc_start = i * rows_per_proc + (i < remainder ? i : remainder);
      int proc_end = proc_start + rows_per_proc + (i < remainder ? 1 : 0);
      int proc_rows = proc_end - proc_start;

      if (rank == 0 && i != 0) {
        for (int row = proc_start; row < proc_end; row++) {
          MPI_Recv(D[row], n, MPI_DOUBLE, i, 0, MPI_COMM_WORLD,
                   MPI_STATUS_IGNORE);
        }
      } else if (rank == i && i != 0) {
        for (int row = start_row; row < end_row; row++) {
          MPI_Send(D[row], n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
      }

      MPI_Barrier(MPI_COMM_WORLD);
    }
  }

  free(k_row);
}

double **pathfinder_network(double **graph, int n, int q, int r, int rank) {
  double **D = NULL;

  if (rank == 0) {
    D = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
      D[i] = (double *)malloc(n * sizeof(double));
      for (int j = 0; j < n; j++) {
        D[i][j] = graph[i][j];
      }
    }
  } else {
    D = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
      D[i] = (double *)malloc(n * sizeof(double));
    }
  }

  for (int i = 0; i < n; i++) {
    MPI_Bcast(D[i], n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  }

  floyd_warshall_parallel(D, q, r);

  if (rank == 0) {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        if (graph[i][j] < D[i][j]) {
          D[i][j] = graph[i][j];
        }
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

int main(int argc, char **argv) {
  // Initialize MPI
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  double start_time = MPI_Wtime();

  if (rank == 0) {
    printf("===============================================\n");
    printf("PATHFINDER NETWORK (MPI Parallel Version)\n");
    printf("Number of processes: %d\n", size);
    printf("===============================================\n");
  }

  char **wordSet = NULL;
  int wordSetSize = 0;
  char **text = NULL;
  int text_size = 0;
  double **graph = NULL;
  double **D = NULL;
  double **pf_net = NULL;

  if (rank == 0) {
    // Only process 0 reads input
    char buffer[1024];

    while (scanf("%s", buffer) == 1) {
      text = realloc(text, (text_size + 1) * sizeof(char *));
      text[text_size] = strdup(buffer);
      text_size++;
    }

    printf("Text size:\t%d\n", text_size);

    for (int i = 0; i < text_size; i++) {
      add_word(&wordSet, &wordSetSize, text[i]);
    }

    qsort(wordSet, wordSetSize, sizeof(char *), compare_strings);

    printf("Unique words:\t%d\n", wordSetSize);
    printf("Word Set:\t%.2f s\n", MPI_Wtime() - start_time);

    double wordset_time = MPI_Wtime();

    int n = wordSetSize;

    // Broadcast the size of the matrix to all processes
    MPI_Bcast(&wordSetSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    graph = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
      graph[i] = (double *)calloc(n, sizeof(double));
    }

    for (int i = 0; i < text_size; i++) {
      int token_i = find_index(wordSet, wordSetSize, text[i]);

      int max_neighbor = (i + 1 + _MAX_DISTANCE < text_size)
                             ? i + 1 + _MAX_DISTANCE
                             : text_size;
      for (int j = i + 1; j < max_neighbor; j++) {
        int token_j = find_index(wordSet, wordSetSize, text[j]);
        if (token_i != token_j) {
          graph[token_i][token_j]++;
          graph[token_j][token_i]++;
        }
      }
    }

    printf("Graph Init:\t%.2f s\n", MPI_Wtime() - wordset_time);
    double graph_time = MPI_Wtime();

    D = (double **)malloc(n * sizeof(double *));
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

    printf("Similarity:\t%.2f s\n", MPI_Wtime() - graph_time);
  } else {
    MPI_Bcast(&wordSetSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }

  double pathfinder_start = MPI_Wtime();

  const int q = wordSetSize - 1;
  const double r = 1;

  pf_net = pathfinder_network(D, wordSetSize, q, r, rank);

  if (rank == 0) {
    printf("Pathfinder:\t%.2f s\n", MPI_Wtime() - pathfinder_start);
    printf("Total:\t%.2f s\n", MPI_Wtime() - start_time);
    printf("===============================================\n");
    printf("RESULT\n");
    printf("===============================================\n");

    for (int i = 0; i < wordSetSize; i++) {
      for (int j = i + 1; j < wordSetSize; j++) {
        printf("%s %s %f\n", wordSet[i], wordSet[j], pf_net[i][j]);
      }
    }

    for (int i = 0; i < text_size; i++) {
      free(text[i]);
    }
    free(text);

    for (int i = 0; i < wordSetSize; i++) {
      free(wordSet[i]);
      free(graph[i]);
      free(D[i]);
      free(pf_net[i]);
    }
    free(wordSet);
    free(graph);
    free(D);
    free(pf_net);
  } else {
    for (int i = 0; i < wordSetSize; i++) {
      free(pf_net[i]);
    }
    free(pf_net);
  }

  MPI_Finalize();
  return 0;
}