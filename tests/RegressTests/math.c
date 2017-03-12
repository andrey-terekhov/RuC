   int main()
{
   int a=7, b=-3;
   a = abs(a);
   b = abs(b);
   print ("a  7");
   printid(a);
   print ("b  3");
   printid(b);

   a=sqrt(a);
   print ("a  2");
   printid(a);
   float c = sqrt(45);
   print ("c  6.708204");
   printid(c);

   b = exp(b);
   print ("b  20");
   printid(b);
   c = exp(3);
   print ("c  20.085537");
   printid(c);

   c = log(12);
   print ("c  2.484907");
   printid(c);

   c = log10(12);
   print ("c  1.079181");
   printid(c);

   c = cos(1.23);
   print ("c  0.334238");
   printid(c);

   c = sin(1.23);
   print ("c  0.942489");
   printid(c);

   c = asin(0.43);
   print ("c  0.444493");
   printid(c);
   
   return 0;
}