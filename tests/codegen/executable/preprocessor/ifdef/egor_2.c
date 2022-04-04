//#define a 1
#ifdef a
  #define abc -7
#else
  #define abc -4
#endif

void main()
{
  int n = abc;
  assert(n == -4, "Must be -4");
}


