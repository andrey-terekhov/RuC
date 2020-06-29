   int main()
{
   int a=1, b=0, c;
   
   c = (a+b)*a;
   print ("c  1");
   printid(c);

   c = b*(a+b);
   print ("c  0");
   printid(c);

   int d[2]={4,2};
   c = d [a+b];
   print ("c  2");
   printid(c);

   return 0;
}