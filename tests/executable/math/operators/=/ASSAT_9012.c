void main()
{
  char  b[4];
  char a;
  a = b[1] = '1';
  a = '1';

  assert(b == '1', "b must be '1'");
}