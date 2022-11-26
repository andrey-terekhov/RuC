int main()
{
   int a = 2;

   assert(a == 2, "a must be 2");

   a = a + 5;

   assert(a == 7, "a must be 7");

   return 0;
}