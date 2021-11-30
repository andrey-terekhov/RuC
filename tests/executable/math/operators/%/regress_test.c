int main()
{
   int a = 7, b = 3, c;
   int d[2] = {4, 2};
   c = a % b;

   assert(c == 1, "c must be 1");

   c = b % d[0];

   assert(c == 3, "c must be 3");

   c = d[0] % d[1];

   assert(c == 0, "c must be 0");

   return 0;
}