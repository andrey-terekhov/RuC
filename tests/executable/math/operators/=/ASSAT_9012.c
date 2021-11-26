void main()
{
  char  b[4];
  char a;
  a = b[1] = '1';
  a = '1';

  assert(b[1] == '1', "b[1] must be '1'");
  assert(a == '1', "a must be '1'");
}