int i, j;

void main()
{
   i = 8; j = 0;
   do {
      j++;
      i--;
      if (i % 2 == 0) 
         continue;
      j++;
      if (i == 1) 
         break;
   } while (i != 10);

   assert(i == 1, "i must be 1");
}
