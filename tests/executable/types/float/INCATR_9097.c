float  b[3], c, d;

void main()
{
  d = ++b[1];

  assert(d == 1, "d must be 1");
  
  d = ++c;

  assert(d == 1, "d must be 1");
}