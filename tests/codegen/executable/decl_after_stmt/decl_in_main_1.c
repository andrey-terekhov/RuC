int main()
{
   int a = 1, b = 0, c;
   
   c = (a + b) * a;

   assert(c == 1, "c must be 1");

   c = b * (a + b);

   assert(c == 0, "c must be 0");

   int d[2] = {4, 2};
   c = d [a + b];

   assert(c == 2, "c must be 2");

   return 0;
}