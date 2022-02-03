int main()
{
   int i = 0, j = 9;
   do 
   {
      i++;
   } while (i!=10);

   assert(i == 10, "i must be 10");

   do
      j++;
   while (i!=10);

   assert(j == 10, "j must be 10");

   i = 8; j = 0;
   do 
   {
      j++;
      i--;
      if (i % 2 == 0) continue;
      j++;
      if (i == 1) break;
   } while (i!=10);

   assert(i == 1, "i must be 1");
   assert(j == 11, "j must be 11");

   return 0;
}