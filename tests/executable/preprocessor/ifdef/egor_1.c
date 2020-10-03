#define a 1
#ifdef a
  #define abc -7
#endif

void main()
{
  assert(abc == -7, "fail1");
}


