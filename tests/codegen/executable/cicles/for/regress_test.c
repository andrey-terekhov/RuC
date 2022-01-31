int main()
{
   int i = 0;
   for (; ; )
   {
      i++;
      if (i > 1) break;
   }

   assert(i == 2, "i must be 2");

   for (i = 0; ; )
   {
      i++;
      if (i > 2) break;
   }

   assert(i == 3, "i must be 3");

   i=0;
   for (; i < 4; )
   {
      i++;
   }

   assert(i == 4, "i must be 4");

   i = 0;
   for (; ; i++)
   {
	   if (i > 4) break;
   }

   assert(i == 5, "i must be 5");
   
   for (i = 0; i < 6; )
	   i++;

   assert(i == 6, "i must be 6");

   i = 0;
   for (; i < 7; i++)
   {
   }

   assert(i == 7, "i must be 7");

   for (i = 0; ; i++)
	   if (i > 7) break;

   assert(i == 8, "i must be 8");

   i = 1;
   for (i = 0; i < 9; i++);

   assert(i == 9, "i must be 9");

   return 0;
}