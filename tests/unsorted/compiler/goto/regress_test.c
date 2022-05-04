int main()
{
   int i = 0;
   lbl: if (i!=10)
   {
      i++;
      goto lbl;
   }
   
   assert(i == 10, "i must be 10"); 

   return 0;
}