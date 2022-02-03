int main()
{
   int a = 64, b = 36, c;
   
   c = a >> b;

   assert(c == 4, "c must be 4");

   return 0;
}
