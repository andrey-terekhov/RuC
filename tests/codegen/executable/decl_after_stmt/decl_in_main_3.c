int main()
{
   int a=32767;

   assert(a == 32767, "a must be 32767");

   a=-32767;

   assert(a == -32767, "a must be -32767");

   int i=2;

   assert(i == 2, "i must be 2");

   char m2[2] = { 'a', 'b' };
   char c = m2[0];

   assert(c == 'a', "c must be a");

   float f = 2.0;

   assert(f == 2.000000, "f must be 2.000000");

   return 0;
}