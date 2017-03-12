   int main()
{
   int a=1, b=0, c=1, d=0;
   
   a&=b;
   print ("a  0");
   printid(a);

   a|=c;
   print ("a  1");
   printid(a);

   a|=c;
   print ("a  1");
   printid(a);

   a^=c;
   print ("a  0");
   printid(a);

   a&=b;
   print ("a  0");
   printid(a);

   c&=d;
   print ("c  1");
   printid(c);

   b^=c;
   print ("b  0");
   printid(b);

   b|=c;
   print ("b  0");
   printid(b);

   return 0;
}