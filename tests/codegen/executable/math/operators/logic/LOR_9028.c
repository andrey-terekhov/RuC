int main()
{
   int a = 1, b = 0, c;
   
   c = a | a;

   assert(c == 1, "c must be 1");

   c = a | b;

   assert(c == 1, "c must be 1");
   
   return 0;
}
