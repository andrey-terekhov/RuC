int main()
{
   int a = 2, b = 3, c;

   c = (a > b) ? a : b;

   assert(c == 3, "c must be 3");

   c = a < b ? a : b;

   assert(c == 2, "c must be 2");

   return 0;
}