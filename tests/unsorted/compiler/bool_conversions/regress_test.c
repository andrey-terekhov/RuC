int main()
{
   int i = 0, j = 10;
   int k;

   assert(j == 10, "j must be 10");
   assert(i == 0, "i must be 0");

   k = (10 >= 9);

   assert(k == 1, "k must be 1");

   {
      int l = 11 * 3 - j;
      int m[3] = {1, 2, 3};

      assert(l == 23, "l must be 23");

      i = m[2];

      assert(i == 3, "i must be 3");
   }

   return 0;
}