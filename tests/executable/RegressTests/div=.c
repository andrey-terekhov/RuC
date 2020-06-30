   int main()
{
   int a=2, b=3;
   int d[2]={4,2};
   float m[2]={2.6,1.3};
   float k;
    printid(d);
    printid(m);
    print(m[1]);

   a/=b;
    print(m[1]);

   print ("a 0");
   printid(a);
    print(m[1]);

   d[0]/=d[1];
   b = d[0];
   print ("b 2");
   printid(b);
    
   m[0]/=d[1];
   k = m[0];
   print ("k 1.300000");
   printid(k);
   m[0]/=m[1];
   k = m[0];
   print ("k 1.000000");
   printid(k);

   return 0;
}
