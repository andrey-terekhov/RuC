int main()
{
   int a = 1, b = 0, c = 1, d = 0;
   
   a &= b;

   assert(a == 0, "a must be 0");

   a |= c;

   assert(a == 1, "a must be 1");

   a |= c;

   assert(a == 1, "a must be 1");

   a ^= c;

   assert(a == 0, "a must be 0");

   a &= b;

   assert(a == 0, "a must be 0");

   c &= d;

   assert(c == 0, "c must be 0");

   b ^= c;

   assert(b == 0, "b must be 0");

   b |= c;

   assert(b == 0, "b must be 0");

   return 0;
}