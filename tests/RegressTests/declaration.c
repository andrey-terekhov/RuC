   int main()
{
   int i = 0, j=10;
    int k;

   print ("j  10");
   printid(j);
   print ("i  0");
   printid(i);

   k = (10>=9);
   print ("k  1");
   printid(k);
    {
       int l = 11*3 - j;
       int m[3] = {1,2,3};

       print ("l  23");
       printid(l);

       i = m[2];
       print ("i  3");
       printid(i);
    }

   return 0;
}