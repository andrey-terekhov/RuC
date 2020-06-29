   int func ()
{
   return 1;
}

   int func2 (int a)
{
   return a*a;
}

void func3 (int i, int *j)
{
	if (i > 0)
	{
		(*j)++;
		return;
	}
	else (*j)--;
	return;
}

   int main()
{
   int i=2;
   int j = func();
   print ("j  1");
   printid (j);
   j = func2 (i);
   print ("j  4");
   printid (j);
   func(i, &j);
   print ("j  5");
   printid (j);  

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

   for (i=0; i<10; i++)
   {
   if (i==5) break;
   }
   print ("i  5");
   printid(5);

   return 0;
}