int main()
{
   int a = 4, b = 36, c;
   
   c = a << b;

   assert(c == 64, "c must be 64");

   return 0;
}