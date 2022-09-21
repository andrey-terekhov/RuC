int main()
{
   int i = 0, j = 1;

   switch (i) 
   {
      case -1:
         j++;
      break;
      case 0:
      {
         int d = 1;
         j += d;
         i = 2;
      }
      case 2:
         j++;
   }

   // assert(j == 3, "j must be 3");

   switch (i) 
   {
      case 1: j--; break;
      case 2: j--; break;
      default: j++;
   }

   // assert(j == 2, "j must be 2");

   return 0;
}