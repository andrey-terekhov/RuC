void main()
{
  char b;
  b = 1 || 2 || 3 || 4;
  if (!b)
  assert(b == '#', "b must be '#'");
}