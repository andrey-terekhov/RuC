   int main()
{
   int a=2, b=3, c;
   float k;
   int d[2]={4,2};
   float m[2]={1.5,2.5};
   c=a-b;
   print ("c  -1");
   printid(c);

   c = d[0]-d[1];
   print ("c  2");
   printid(c);

   k = m[0]-d[1];
   print ("k  -0.500000");
   printid(k);

   k = m[1]-m[0];
   print ("k  1.000000");
   printid(k);

   return 0;
}