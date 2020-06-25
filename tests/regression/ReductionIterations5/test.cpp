#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void computeSum (long long int *a, long long int SIZE_Z, long long int SIZE_X, long long int SIZE_Y){
	int x, y,  z;

  x = SIZE_X;
  y = SIZE_Y;
  for(z = 0; z < SIZE_Z; z++){
    if (z == 0){
      for (int j=0; j < 1000000; j++){
        rand();
      }
    }

    (*a) += x + y + z;
  }

  return ;
}

int main (int argc, char *argv[]){

  /*
   * Check the inputs.
   */
  if (argc < 2){
    fprintf(stderr, "USAGE: %s LOOP_ITERATIONS\n", argv[0]);
    return -1;
  }
  auto iterations = atoll(argv[1]);
  if (iterations == 0) return 0;

  iterations *= 100;
  long long int *array = (long long int *) calloc(iterations, sizeof(long long int));

  computeSum(&(array[0]), iterations, 2, 2);
  auto s = array[0];

  printf("%lld %lld\n", s, array[iterations/2]);

  return 0;
}