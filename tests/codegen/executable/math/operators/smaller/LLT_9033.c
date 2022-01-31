int main()
{
  bool c;
  char mm[2] = { 'a', 'b'};
  c = mm[1] < mm[0];

  assert(!c, "c must be false");

  return 0;
}
