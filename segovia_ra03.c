#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h> // for threads
#include <sched.h>   // for cpu affinity

#include <stdatomic.h> // just for a loading bar to see how the threads are doing

// https://stackoverflow.com/a/3437433
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX_CORES 4
#define BAR_WIDTH 50 // for loading bar

typedef struct ARG
{
   int num_cols; // number of columns of the submatrix; n is still how many rows the columns has
   int core;
   int **submatrix;
} args;

// global variables
// int **matrix_x;
float *vector_p;
int q;
int m;
int n;
int t; // threads

// for incrementing the bar
int completed_columns = 0;
int completed_threads = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to update and display the loading bar
void update_loading_bar()
{
   float progress = (float)completed_columns / n;
   int bar_position = (int)(progress * BAR_WIDTH);

   printf("\r[");
   for (int i = 0; i < BAR_WIDTH; i++)
   {
      if (i < bar_position)
         printf("="); // Filled part
      else if (i == bar_position)
         printf(">"); // Leading edge
      else
         printf(" "); // Empty part
   }
   printf("] %d%% ( %d / %d columns )( %d / %d threads )", (int)(progress * 100), completed_columns, n, completed_threads, t);
   fflush(stdout);
}

void *mse_ma(void *arguments)
{

   args *temp;
   temp = (args *)arguments; // extract arguments

   // Set thread affinity
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(temp->core, &cpuset);
   pthread_t thread = pthread_self(); // Get the thread ID

   int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
   if (n <= 10)
   {
      if (result != 0)
      {
         perror("pthread_setaffinity_np");
      }
      else
      {
         printf("Thread %ld assigned to CPU %d\n", (long)thread, temp->core);
      }
   }

   // initialize holder of results
   float *p = (float *)malloc((temp->num_cols) * sizeof(float));

   int i, k, col;
   float sum_eq01, pj, sum_eq02, maiq;

   for (col = 0; col < temp->num_cols; col++)
   {                // (1) for each column of X (this is an index of x)
      sum_eq01 = 0; // summation part of pj

      for (i = q; i < m; i++)
      {                // (3) get the MSE (no more +1 since it's 0 indexing)
         sum_eq02 = 0; // summation part of MAi(q)

         for (k = i - q; k < i; k++)
         {                                              // (2) get the moving averages (no more -1 since it's 0 indexing)
            sum_eq02 += (float)temp->submatrix[col][k]; // sum up the range k to i
         }
         maiq = sum_eq02 / q; // moving average of the range
         sum_eq01 += pow((float)temp->submatrix[col][i] - maiq, 2);
      }

      p[col] = sqrt(sum_eq01) / (float)(m - q);

      // Update loading bar
      pthread_mutex_lock(&mutex);
      completed_columns++;
      update_loading_bar();
      pthread_mutex_unlock(&mutex);
   }

   // free submatrix of completed thread
   free(temp->submatrix);

   // Update loading bar for threads
   pthread_mutex_lock(&mutex);
   completed_threads++;
   update_loading_bar();
   pthread_mutex_unlock(&mutex);

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

   // (2) // CREATE SUBMATRICES  IN arg.submatrix INSTEAD
   // matrix_x = (int **)malloc(n * sizeof(int *));
   int i, j, k;
   int min = 1, max = 1000;
   srand(time(NULL));

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
   printf("%d mod %d = %d ;\n %d / %d = %d\n", n, t, n % t, n, t, n / t);
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

   // initialize arguments
   for (i = 0; i < t; i++)
   {
      // set core affinity
      arguments[i].core = i % MAX_CORES; // round robin scheduling for affinity

      // find how many columns to evaluate per thread
      if (i == t - 1 && !divisible)
      {
         arguments[i].num_cols = n % t;
      }
      else
      {
         arguments[i].num_cols = n / t;
      }

      // initialize submatrix
      arguments[i].submatrix = (int **)malloc(arguments[i].num_cols * sizeof(int *));
      for (j = 0; j < arguments[i].num_cols; j++)
      {
         arguments[i].submatrix[j] = (int *)malloc(n * sizeof(int));
         for (k = 0; k < n; k++)
         {
            arguments[i].submatrix[j][k] = rand() % (max - min + 1) + min;
         }
      }
   }

   // temp to hold results
   float **temp = (float **)malloc(t * sizeof(float *));

   // rough time when mse_ma calculation starts
   time_t starttime;
   struct tm *timeinfo;
   time(&starttime);
   timeinfo = localtime(&starttime);
   printf("Started: %s", asctime(timeinfo));

   // (5) ========================== MAIN PROCESS =================================
   // (6)
   for (i = 0; i < t; i++)
   {
      pthread_create(&tid[i], NULL, mse_ma, (void *)&arguments[i]);
   }

   for (i = 0; i < t; i++)
   {
      pthread_join(tid[i], (void **)&temp[i]);
   }
   // (7)
   //     ========================== MAIN PROCESS =================================
   // rough time when mse_ma calculation ends
   time_t endtime;
   time(&endtime);
   timeinfo = localtime(&endtime);
   printf("Ended: %s", asctime(timeinfo));

   // (8)
   double time_elapsed =  difftime(endtime,starttime);//((double)(end - start)) / CLOCKS_PER_SEC;

   // (9)
   printf("Time Elapsed: %f\n", time_elapsed);

   // recover the results of each thread
   int ans = 0; // index for current vector_p
   for (i = 0; i < t; i++)
   {
      // if(n <= 10) printf("Thread %d\n", i);
      float *temp2 = temp[i];

      int end = n / t;
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
         ans++;
      }

      free(temp2);
   }

   return 0;
}