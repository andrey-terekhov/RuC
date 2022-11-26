int main()
{
  bool c;
  int mm[2] = {11, 2};
  c = mm[1] <= mm[0];

  assert(c, "c must be true");

  return 0;
}
