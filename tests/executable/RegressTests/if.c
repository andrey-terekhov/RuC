   int main()
{
   int i = 0, j=10;

   if (i>j) printid(i);
   else printid(j);
   print ("j  10");

   if (11!=10) {
   i=1;
   }
   print ("i  1");
   printid(i);

   j = 9;
   if (14>15) j=15;
   print ("j  9");
   printid(j);

   if (11==11) j=2;
   else {
   i=2+1;
   j=i;
   }
   print ("j  2");
   printid(j);

   return 0;
}