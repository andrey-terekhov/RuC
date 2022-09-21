int main()
{
  int  c;
  float mm[2] = {1, 2};
  c = mm[1] < mm[0];

  assert(c == 0, "c must be 0");

  return 0;
}