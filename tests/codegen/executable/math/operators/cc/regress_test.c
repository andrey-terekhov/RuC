int main()
{
   int a = 4, b = 36, c;
   
   c = a << b; 

   assert(c == 64, "c must be 64");

   c = a >> b;

   assert(c == 0, "c must be 0");

   return 0;
}