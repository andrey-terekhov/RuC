   int main()
{
   int i = 0;
   for (;;)
   {
	i++;
	if (i > 1) break;
   }
   print ("i  2");
   printid(i);

   for (i=0;;)
   {
	i++;
	if (i > 2) break;
   }
   print ("i  3");
   printid(i);

   i=0;
   for (;i<4;)
   {
	i++;
   }
   print ("i  4");
   printid(i);

   i=0;
   for (;;i++)
   {
	if (i > 4) break;
   }
   print ("i  5");
   printid(i);
   
   for (i=0;i<6;)
	i++;
   print ("i  6");
   printid(i);

   i=0;
   for (;i<7;i++)
   {
   }
   print ("i  7");
   printid(i);

   for (i=0;;i++)
	if (i > 7) break;
   print ("i  8");
   printid(i);

   i = 1;
   for (i = 0; i < 9; i++);
   print ("i  9");
   printid(i);

   return 0;
}