int main()
{
   int a = 2, b = 3, c;
   int d[2] = {4,2};
   float k;
   float m[2] = {2.6, 1.3};
   c = a / b;

   assert(c == 0, "c must be 0");

   c = d[0] / d[1];

   assert(c == 2, "c must be 2");

   k = m[0] / d[1];

   assert(k == 1.300000, "k must be 1.300000");

   k = m[0] / m[1];

   assert(k == 2.000000, "k must be 2.000000");

   return 0;
}