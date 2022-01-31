int main()
{
   int a = 4, b = 3;
   
   a <<= b;

   assert(a == 32, "a must be 32");

   a >>= b;

   assert(a == 4, "a must be 4");

   return 0;
}
