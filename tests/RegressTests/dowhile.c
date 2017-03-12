   int main()
{
   int i = 0, j=9;
   do {
   i++;
   } while (i!=10);
   print ("i  10");
   printid(i);

   do
   j++;
   while (i!=10);
   print ("j  10");
   printid(j);

   i=8; j=0;
   do {
   j++;
   i--;
   if (i % 2 == 0) continue;
   j++;
   if (i == 1) break;
   } while (i!=10);
   print ("j  11");
   printid(j);
   print ("i  1");
   printid(i);

   return 0;
}