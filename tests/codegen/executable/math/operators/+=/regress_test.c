int main()
{
   int a = 2, b = 3;
   int d[2] = {4,2};
   float m[2] = {1.5, 2.5};
   float k;
   a += b;

   assert(a == 5, "a must be 5");

   d[0] += d[1];
   b = d[0];

   assert(b == 6, "b must be 6");

   m[0] += d[1];
   k = m[0];

   assert(k == 3.500000, "k must be 3.500000");

   m[0] += m[1];
   k = m[0];

   assert(k == 6.000000, "k must be 6.000000");

   return 0;
}