int main()
{
   int a = 1, b = 0, c;
   
   c = a & b;

   assert(c == 0, "c must be 0");

   c = a | a;

   assert(c == 1, "c must be 1");

   c = a | b;

   assert(c == 1, "c must be 1");

   c = a ^ a;

   assert(c == 0, "c must be 0");

   c = (a ^ a) || (a | b);

   assert(c == 1, "c must be 1");

   c = (a ^ a) && (a | b);

   assert(c == 0, "c must be 0");

   return 0;
}