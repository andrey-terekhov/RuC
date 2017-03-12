   int main()
{
   int a=1,b=0, c;
   c = ~a;
   print ("c  0");
   printid(0);

   ñ= ~b;
   print ("c  1");
   printid(1);

   ñ= a&b;
   print ("c  0");
   printid(0);

   ñ= a&a;
   print ("c  1");
   printid(1);

   ñ= b&b;
   print ("c  0");
   printid(0);

   ñ=(b!=a);
   print ("c  1");
   printid(1);

   ñ= a!=a;
   print ("c  0");
   printid(0);

   ñ=(a==a);
   print ("c  1");
   printid(1);

   c = b==a;
   print ("c  0");
   printid(0);

   return 0;
}