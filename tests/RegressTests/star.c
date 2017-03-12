   int main()
{
   int a=2, b=3, c=-1, k;
   int d[2]={4,2};
   float l;
   float m[2]={1.5,2.5};
   k = a*b;
   print ("k  6");
   printid(k);

   k = c*b;
   print ("k  -3");
   printid(k);

   k = d[0]*d[1];
   print ("k  8");
   printid(k);

   l = m[0]*d[1];
   print ("l  3.000000");
   printid(l);

   l = m[0]*5;
   print ("l  7.500000");
   printid(l);

   return 0;
}