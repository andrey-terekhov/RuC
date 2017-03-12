   int main()
{
   int a=2, b=3, c;
   int d[2]={4,2};
   float k;
   float m[2]={2.6,1.3};
   c= a/b;
   print ("c  0");
   printid(c);

   c = d[0]/d[1];
   print ("c  2");
   printid(c);

   k = m[0]/d[1];
   print ("k  1.300000");
   printid(k);

   k = m[0]/m[1];
   print ("k  2.000000");
   printid(k);

   return 0;
}