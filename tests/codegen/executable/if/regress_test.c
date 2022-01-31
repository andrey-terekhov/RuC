   int main()
{
   int i = 0, j = 10;

   if (i > j)
      assert(i == 0, "i must be 0");
   else
      assert(j == 10, "j must be 10");
   
   assert(j == 10, "j must be 10");

   if (11 != 10) 
   {
      i = 1;
   }

   assert(i == 1, "i must be 1");

   j = 9;
   if (14 > 15) 
      j = 15;

   assert(j == 9, "j must be 9");

   if (11 == 11) 
      j=2;
   else 
   {
      i = 2 + 1;
      j = i;
   }

   assert(j == 2, "j must be 2");

   return 0;
}