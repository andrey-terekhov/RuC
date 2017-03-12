   int main()
{
   int i = 0, j=10;

   while (i!=0)
   {
       i--;
       j--;
   }
   print ("j  10");
   printid(j);

   while (i==0)
   {
       i--;
       j--;
   }
   print ("j  9");
   printid(j);

   while (j!=0)
   {
       i++;
       j--;
   }
   print ("j  0");
   printid(j);
   print ("i  8");
   printid(i);

   while (j>=0)
   {
	i--;
	if (i % 2 == 0) continue;
	j++;
	if (i == 1) break;
   }
   print ("j  4");
   printid(j);
   print ("i  1");
   printid(i);

   return 0;
}