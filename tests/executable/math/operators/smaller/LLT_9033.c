int main()
{
  int  c;
  char mm[2] = { 'a', 'b'};
  c = mm[1] < mm[0];

  assert(c == 0, "c must be 0");

  return 0;
}
