void main()
{
  int b;
  b = 1 || 2 || 3 || 4;
  if (~b)
  {
    assert(b == 1, "b must be 1");
  }
}
