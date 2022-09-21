int main()
{
   int a = 2, b = 3, c = -1;
   int d[2] = {4, 2};
   float m[2] = {1.5, 2.5};
   float k;
   a *= b;

   assert(a == 6, "a must be 6");

   c *= b;

   assert(c == -3, "c must be -3");

   d[0] *= d[1];
   b = d[0];

   assert(b == 8, "b must be 8");

   m[0] *= d[1];
   k = m[0];

   assert(k == 3.000000, "k must be 3.000000");

   m[0] *= m[1];
   k = m[0];

   assert(k == 7.500000, "k must be 7.500000");

   return 0;
}