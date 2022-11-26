int main()
{
   int a = 2, b = 3, c;
   float k;
   int d[2] = {4, 2};
   float m[2] = {1.5, 2.5};

   c = a - b;

   assert(c == -1, "c must be -1");

   c = d[0] - d[1];

   assert(c == 2, "c must be 2");

   k = m[0] - d[1];

   assert(k == -0.500000, "k must be -0.500000");

   k = m[1] - m[0];

   assert(k == 1.000000, "k must be 1.000000");

   return 0;
}