   int main()
{
   int a=2, b=3, c;
   float k;
   int d[2]={4,2};
   float m[2]={1.5,2.5};
   c=a+b;
   print ("c  5");
   printid(c);

   c = d[0]+d[1];
   print ("c  6");
   printid(c);

   k = m[0]+d[1];
   print ("k  3.500000");
   printid(k);

   k = m[0]+m[1];
   print ("k  4.000000");
   printid(k);

   return 0;
}