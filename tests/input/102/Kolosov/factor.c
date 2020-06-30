int n;

int f (int n)
{
  if (n)
      return n * f(n-1);
  return 1;
}

void main()
{
  getid (n);
  print (f(n));
}