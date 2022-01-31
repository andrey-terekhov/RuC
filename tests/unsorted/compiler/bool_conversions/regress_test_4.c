int main()
{
   int a = 2, b = 3, c;
   int d[2] = {4, 2};
   float m[2] = {1.5, 2.5};
   char m2[2] = { 'a', 'b' };

   c = a > b;

   assert(c == 0, "c must be 0");

   c = d[0] > d[1];

   assert(c == 1, "c must be 1");

   c = d[0] > m[0];

   assert(c == 1, "c must be 1");

   c = m[0] > m[1];

   assert(c == 0, "c must be 0");

   m[1] = 2.0;
   c = m[1] > d[1];

   assert(c == 0, "c must be 0");

   c = m2[1] > m2[0];

   assert(c == 1, "c must be 1");

   c = m2[0] > m2[1];

   assert(c == 0, "c must be 0");

   m2[0] = m2[1];
   c = m2[1] > m2[0];

   assert(c == 0, "c must be 0");

   return 0;
}