   int main()
{
   int a=7, b=3, c;
   int d[2] = {4, 2};
   c = a % b;
   print ("c  1");
   printid(c);

   c = b % d[0];
   print ("c  3");
   printid(c);

   c = d[0] % d[1];
   print ("c  0");
   printid(c);

   return 0;
}