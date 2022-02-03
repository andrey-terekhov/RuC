int main()
{
   int a=2, b=3;
   bool c;
   int d[2]={4,2};
   float m[2]={1.5,2.5};
   c=a<=b;

   assert(c, "c must be true");

   c=d[0]<=d[1];

   assert(!c, "c must be false");

   c=d[0]<=m[0];

   assert(!c, "c must be false");

   c=m[0]<=m[1];

   assert(c, "c must be true");

   m[1]=2.0;
   c=m[1]<=d[1];

   assert(c, "c must be true");

   char m2[2] = { 'a', 'b' };
   c = m2[1] <= m2[0];

   assert(!c, "c must be false");

   c = m2[0] <= m2[1];

   assert(c, "c must be true");

   m2[0]=m2[1];
   c = m2[1] <= m2[0];

   assert(c, "c must be true");

   return 0;
}
