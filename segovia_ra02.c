#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h> //for threads

// https://stackoverflow.com/a/3437433
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// (1)
// float *mse_ma(int **x, int q, int m, int n)
/*
float *mse_ma(int **x, int q, int m, int n, int col_start, int col_end)
{
   float *p = (float *)malloc(n * sizeof(float));
   int i, k, col;
   float sum_eq01, pj, sum_eq02, maiq;

   for (col = col_start; col < col_end; col++)
   {                // (1) for each column of X (this is an index of x)
      sum_eq01 = 0; // summation part of pj

      for (i = q; i < m; i++)
      {                // (3) get the MSE (no more +1 since it's 0 indexing)
         sum_eq02 = 0; // summation part of MAi(q)

         for (k = i - q; k < i; k++)
         {                                // (2) get the moving averages (no more -1 since it's 0 indexing)
            sum_eq02 += (float)x[col][k]; // sum up the range k to i
            //  printf("%f\t", (float)x[col][k]);
         }
         maiq = sum_eq02 / q; // moving average of the range
         // printf("\nsum: %f\nmaiq %d.%d: %f\n", sum_eq02, col+1, i+1, maiq);
         sum_eq01 += pow((float)x[col][i] - maiq, 2);
      }
      p[col] = sqrt(sum_eq01) / (float)(m - q);
   }
   return p;
}

*/

typedef struct ARG
{
   int col_start;
   int col_end;
} args;

// global variables
int **matrix_x;
float *vector_p;
int q;
int m;
int n;
int t;

void *mse_ma(void *arguments)
{
   args *temp;
   temp = (args *)arguments;

   int col_start = temp->col_start;
   int col_end = temp->col_end;
   // if(n < 100) printf("This thread has %d columns; from %d to %d\n", col_end-col_start, col_start, col_end);

   float *p = (float *)malloc((col_end - col_start) * sizeof(float));
   // float **return_value = (float **)malloc(sizeof(float *));

   int i, k, col;
   float sum_eq01, pj, sum_eq02, maiq;
   // if (n <= 10) printf("q: %d\n",q);
   int curr_index = 0;
   for (col = col_start; col < col_end; col++)
   {                // (1) for each column of X (this is an index of x)
      sum_eq01 = 0; // summation part of pj

      for (i = q; i < m; i++)
      {                // (3) get the MSE (no more +1 since it's 0 indexing)
         sum_eq02 = 0; // summation part of MAi(q)

         for (k = i - q; k < i; k++)
         {                                // (2) get the moving averages (no more -1 since it's 0 indexing)
            sum_eq02 += (float)matrix_x[col][k]; // sum up the range k to i
            //  printf("%f\t", (float)x[col][k]);
         }
         maiq = sum_eq02 / q; // moving average of the range
         // printf("\nsum: %f\nmaiq %d.%d: %f\n", sum_eq02, col+1, i+1, maiq);
         sum_eq01 += pow((float)matrix_x[col][i] - maiq, 2);
      }
      // p[col] = sqrt(sum_eq01) / (float)(m - q);
      p[curr_index] = sqrt(sum_eq01) / (float)(m - q);
      // if (n <= 10) printf("%f\t",p[curr_index]);
      curr_index++;
   }
   // if (n <= 10) printf("\n");
   // *return_value = p;

   // return p;
   pthread_exit((void *)p);
}

// (2)
int main(int argc, char *argv[])
{

   if (argc != 3)
   {
      printf("<program_name> <size_of_square_matrix> <#_of_threads>\n");
      return 0;
   }
   // (1)
   n = atoi(argv[1]); // size of square matrix
   m = n;
   t = atoi(argv[2]);

   if (t > n)
   {
      printf("should be n >= t\n");
      return 0;
   }

   // (2)
   matrix_x = (int **)malloc(n * sizeof(int *));
   int i, j;
   int min = 1, max = 1000;
   srand(time(NULL));

   for (i = 0; i < n; i++)
   {
      int *temp = (int *)malloc(n * sizeof(int));
      for (j = 0; j < n; j++)
      {
         temp[j] = rand() % (max - min + 1) + min; // https://www.geeksforgeeks.org/generating-random-number-range-c/
      }
      matrix_x[i] = temp; // this corresponds to a column
   }

   // if (n <= 25)
   // {
   //    printf("Generated Matrix\n");
   //    for (i = 0; i < n; i++)
   //    {
   //       for (j = 0; j < n; j++)
   //       {
   //          printf("%d\t", matrix_x[j][i]);
   //       }
   //       printf("\n");
   //    }
   // }

   // (3)
   // split matrix into t submatrices;
   pthread_t *tid = (pthread_t *)malloc(t * sizeof(pthread_t));
   if (tid == NULL)
   {
      printf("Error! memory not allocated.");
      exit(0);
   }

   // my student number: 2019 - 65131
   q = MAX(n / 2, MIN((10 * (6 + 5)) * n / 100, 3 * n / 4));
   printf("q: %d\n", q);

   vector_p = (float *)malloc(n * sizeof(float)); // holds the MSEs of respective Order Q
                                                  // Moving Average of the Columns of X

   int num_submatrices;
   int divisible;
   printf("%d mod %d = %d ;\n %d / %d = %d\n",n,t,n%t, n,t,n/t);
   if (n % t == 0)
   {
      divisible = 1;
   }
   else
   {
      divisible = 0;
   }

   // create arguments
   args *arguments;
   arguments = (args *)malloc(t * sizeof(args));

   // temp to hold results
   float **temp = (float **)malloc(t * sizeof(float *));
   // (5)
   clock_t start, end;
   start = clock();

   // (6)
   // vector_p = mse_ma(matrix_x, integer_q, n, n);
   // vector_p = mse_ma(matrix_x, integer_q, n, n, 0, n);
   for (i = 0; i < t; i++)
   {
      arguments[i].col_start = i * (n / t);

      if (i == t - 1)
      {
         if (divisible == 0)
         {
            arguments[i].col_end = arguments[i].col_start + (n / t) + (n % t);
            pthread_create(&tid[i], NULL, mse_ma, (void *)&arguments[i]);
            continue;
         }
      }
      arguments[i].col_end = arguments[i].col_start + (n / t);

      pthread_create(&tid[i], NULL, mse_ma, (void *)&arguments[i]);
   }

   for (i = 0; i < t; i++)
   {
      pthread_join(tid[i], (void **)&temp[i]);
   }
   // (7)
   end = clock();
   // (8)
   double time_elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;

   // (9)
   printf("Time Elapsed: %f\n", time_elapsed);

   // recover the results of each thread
   int ans = 0; // index for current vector_p
   for (i = 0; i < t; i++)
   {
      // if(n <= 10) printf("Thread %d\n", i);
      float *temp2 = temp[i];
      
      int end = n/t;
      if (i == t - 1)
      {
         if (divisible == 0)
         {
            end = n / t + n % t;
         }
      }

      for (j = 0; j < end; j++)
      {
         vector_p[ans] = temp2[j];
         // if(n <= 10) printf("Thread %d\n", i);printf("%f\t",vector_p[ans]);
         ans++;
      }
      // if(n <= 10) printf("Thread %d\n", i);printf("\n");

      free(temp2);
   }

   // if (n <= 25)
   // {
   //    for (i = 0; i < n; i++)
   //    {
   //       printf("%f\t", vector_p[i]);
   //    }
   //    printf("\n");
   // }
   
   return 0;
}