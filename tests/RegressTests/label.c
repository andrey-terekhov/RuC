   int main()
{
   int i = 0;
lbl: if (i!=10)
{
   i++;
   goto lbl;
}
   print ("i  10");
   printid (i);  

   return 0;
}