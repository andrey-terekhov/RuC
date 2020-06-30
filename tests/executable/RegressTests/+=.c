   int main()
{
   int a=2, b=3;
   int d[2]={4,2};
   float m[2]={1.5,2.5};
   float k;
   a+=b;
   print ("a  5");
   printid(a);

   d[0]+=d[1];
   b = d[0];
   print ("b  6");
   printid(b);

   m[0]+=d[1];
   k = m[0];
   print ("k  3.500000");
   printid(k);

   m[0]+=m[1];
   k = m[0];
   print ("k  6.000000");
   printid(k);

   return 0;
}