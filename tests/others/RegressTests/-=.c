   int main()
{
   int a=4, b=3;
   int d[2]={4,2};
   float m[2]={2.5,0.1};
   float k;
   a-=b;
   print ("a  1");
   printid(a);

   d[0]-=d[1];
   b = d[0];
   print ("b  2");
   printid(b);

   m[0]-=d[1];
   k = m[0];
   print ("k  0.500000");
   printid(k);

   m[0]-=m[1];
   k = m[0];
   print ("k  0.400000");
   printid(k);

   return 0;
}