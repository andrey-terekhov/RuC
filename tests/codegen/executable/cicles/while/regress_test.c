   int main()
{
   int i = 0, j = 10;

   while (i != 0)
   {
       i--;
       j--;
   }

   assert(j == 10, "j must be 10");

   while (i == 0)
   {
       i--;
       j--;
   }

   assert(j == 9, "j must be 9");

   while (j!= 0)
   {
       i++;
       j--;
   }

   assert(j == 0, "j must be 0");
   assert(i == 8, "i must be 8");

   while (j >= 0)
   {
	i--;
	if (i % 2 == 0) continue;
	j++;
	if (i == 1) break;
   }

   assert(j == 4, "j must be 4");
   assert(i == 1, "i must be 1");

   return 0;
}