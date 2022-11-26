int main()
{
   int a = 1, b = 0, c;
   bool d;
   
   c = a & b;

   assert(c == 0, "c must be 0");

   c = a | a;

   assert(c == 1, "c must be 1");

   c = a | b;

   assert(c == 1, "c must be 1");

   c = a ^ a;

   assert(c == 0, "c must be 0");

   d = (a ^ a) || (a | b);

   assert(d, "d must be true");

   d = (a ^ a) && (a | b);

   assert(!d, "d must be false");

   return 0;
}
