   int main()
{
   int a = 1, b = 0, c;
   
   c = a & b;

   assert(c == 0, "c must be 0");

   return 0;
}