int main()
{
  int  c;
  float mm[2] = {1, 2};
  c = mm[1] > mm[0];

  assert(c == 1, "c must be 1");

  return 0;
}