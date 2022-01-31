#include "lib1.h"

#define A 5 
#define B 3 
#define C 7 
#define D 1 

void main()
{
  int h = h1() + A;

  assert(h == 8, "h must be 8");
}