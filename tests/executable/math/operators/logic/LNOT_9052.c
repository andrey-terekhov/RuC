void main()
{
  int b;
  b = 1 || 2 || 3 || 4;
  if (~b)
  {
    assert(b == 25, "b must be 25");
  }
}
