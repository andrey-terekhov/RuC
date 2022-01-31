int main()
{
   int a = 2, b = 3, c =- 1, k;
   int d[2] = {4, 2};
   float l;
   float m[2] = {1.5, 2.5};
   k = a * b;

   assert(k == 6, "k must be 6");

   k = c * b;

   assert(k == -3, "k must be -3");

   k = d[0] * d[1];

   assert(k == 8, "k must be 8");

   l = m[0] * d[1];

   assert(l == 3.000000, "l must be 3.000000");

   l = m[0] * 5;

   assert(l == 7.500000, "l must be 7.500000");

   return 0;
}