#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// https://stackoverflow.com/a/3437433
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// (1)
float* mse_ma(int **x, int q, int m, int n) {
    float* p = (float*)malloc(n * sizeof(float));
    int i, k, col;
    float sum_eq01, pj, sum_eq02, maiq;
    
    for (col = 0; col < n; col++) {                      // (1) for each column of X (this is an index of x)
        sum_eq01 = 0;                                    // summation part of pj

        for (i = q; i < m; i++) {                        // (3) get the MSE (no more +1 since it's 0 indexing)
            sum_eq02 = 0;                                // summation part of MAi(q)

            for (k = i - q; k < i; k++) {                // (2) get the moving averages (no more -1 since it's 0 indexing)
                sum_eq02 += (float)x[col][k];            // sum up the range k to i
               //  printf("%f\t", (float)x[col][k]);
            }
            maiq = sum_eq02/q;                           // moving average of the range
            // printf("\nsum: %f\nmaiq %d.%d: %f\n", sum_eq02, col+1, i+1, maiq);
            sum_eq01 += pow((float)x[col][i] - maiq, 2);
        }
        p[col] = sqrt(sum_eq01)/(float)(m-q);
    }
    return p;
}

// (2)
int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("<program_name> <size_of_square_matrix>\n");
        return 0;
    }
    // (1)
    int n = atoi(argv[1]); // size of square matrix
    
    // (2)
    int **matrix_x = (int**)malloc(n * sizeof(int*));
    int i, j;
    int min = 1, max = 1000;
    srand(time(NULL));
    
    for (i = 0; i < n; i++) {
        int *temp = (int*)malloc(n * sizeof(int));
        for (j = 0; j < n; j++) {
            temp[j] = rand() % (max - min + 1) + min; // https://www.geeksforgeeks.org/generating-random-number-range-c/
        }
        matrix_x[i] = temp; // this corresponds to a column
    }

   if (n <= 25){
      printf("Generated Matrix\n");
      for (i = 0; i < n; i++) {
          for (j = 0; j < n; j++) {
              printf("%d\t", matrix_x[j][i]);
          }
          printf("\n");
      }
   }


    // (3)
    // my student number: 2019 - 65131
    int integer_q = MAX(n/2, MIN((10*(6+5)) * n/100, 3*n/4));
    printf("q: %d\n", integer_q); 

    // (4)
    float *vector_p = (float*)malloc(n * sizeof(float)); // holds the MSEs of respective Order Q 
                   // Moving Average of the Columns of X

    // (5)
    clock_t start, end;
    start = clock();
    
    // (6)
    vector_p = mse_ma(matrix_x, integer_q, n, n);
    
    // (7)
    end = clock();
   //  if (n <= 100){
   //    for (i = 0; i < n; i++) {
   //       printf("%f\t", vector_p[i]);
   //    }
   //    printf("\n");
   // }
    // (8)
    double time_elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    // (9)
    printf("Time Elapsed: %f\n", time_elapsed);
    return 0;
}