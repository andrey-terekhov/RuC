int main()
{
   int a = 2, b = 3;
   int d[2] = {4, 2};
   float m[2] = {2.6, 1.3};
   float k;

   assert(d[0] == 4, "d[0] must be 4");
   assert(d[1] == 2, "d[1] must be 2");

   assert(m[0] == 2.6, "m[0] must be 2.6");
   assert(m[1] == 1.3, "m[1] must be 1.3");

   a /= b;

   assert(a == 0, "a must be 0");

   d[0] /= d[1];
   b = d[0];

   assert(b == 2, "b must be 2");
    
   m[0] /= d[1];
   k = m[0];

   assert(k == 1.300000, "k must be 1.300000");

   m[0] /= m[1];
   k = m[0];

   assert(k == 1.000000, "k must be 1.000000");

   return 0;
}
