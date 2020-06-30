   int main()
{
   int i = 0, j = 1;

   switch (i) {
   case -1:
   j++;
   break;
   case 0:
   {
   int d=1;
   j+=d;
   i=2;
   }
   case 2:
   j++;
   }
   print ("j  3");
   printid(j);

   switch (i) {
   case 1: j--; break;
   case 2: j--; break;
   default: j++;
   }
   print ("j  4");
   printid(j);

   return 0;
}