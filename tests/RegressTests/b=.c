   int main()
{
   int a = 2, b = 3, c;
   int d[2] = {4, 2};
   float m[2] = {1.5, 2.5};
    char m2[2] = { 'a', 'b' };

   c = a>=b;
   print ("c  0");
   printid(c);

   c = d[0]>=d[1];
   print ("c  1");
   printid(c);

   c = d[0]>=m[0];
   print ("c  1");
   printid(c);

   c = m[0]>=m[1];
   print ("c  0");
   printid(c);

   m[1] = 2.0;
   c = m[1]>=d[1];
   print ("c  1");
   printid(c);

   c = m2[1] >= m2[0];
   print ("c  1");
   printid(c);

   c = m2[0] >= m2[1];
   print ("c  0");
   printid(c);

   m2[0]=m2[1];
   c = m2[1] >= m2[0];
   print ("c  1");
   printid(c);

   return 0;
}