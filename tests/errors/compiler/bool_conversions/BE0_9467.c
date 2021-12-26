void main()
{
  char b;
  b = 1 || 2 || 3 || 4;
  if (b)
  {
    assert(b != 0, "b must not be 0");
  }
}
