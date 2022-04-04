void main()
{
  char b;
  b = 1 || 2 || 3 || 4;

  assert(b == 1, "b must be 1");
}