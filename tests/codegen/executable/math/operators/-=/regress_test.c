int main()
{
   int a = 4, b = 3;
   int d[2] = {4, 2};
   float m[2] = {2.5, 0.1};
   float k;

   a -= b;

   assert(a == 1, "a must be 1");

   d[0] -= d[1];
   b = d[0];

   assert(b == 2, "b must be 2");

   m[0] -= d[1];
   k = m[0];

   assert(k == 0.500000, "k must be 0.500000");

   m[0] -= m[1];
   k = m[0];

   assert(k == 0.400000, "k must be 0.400000");

   return 0;
}