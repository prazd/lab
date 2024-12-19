#include <iostream>
#include <limits>
#include "gmp.h"

void fact(int n){
  int i;
  mpz_t p;

  mpz_init_set_ui(p,1); /* p = 1 */
  for (i=1; i <= n ; ++i){
    mpz_mul_ui(p,p,i); /* p = p * i */
  }
  printf ("%d!  =  ", n);
  mpz_out_str(stdout,10,p);
  mpz_clear(p);
}

int main() {
    fact(1000);
}
