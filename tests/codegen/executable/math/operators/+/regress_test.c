int main()
{
   int a = 2, b = 3, c;
   float k;
   int d[2] = {4,2};
   float m[2] = {1.5, 2.5};
   c = a + b;

   assert(c == 5, "c must be 5");

   c = d[0] + d[1];

   assert(c == 6, "c must be 6");

   k = m[0] + d[1];

   assert(k == 3.500000, "k must be 3.500000");

   k = m[0] + m[1];

   assert(k == 4.000000, "k must be 4.000000");

   return 0;
}