void func3 (int i)
{
	printid (i);  
}

   int main()
{
   int a=32767;
   print ("a  32767");
   printid(a);

   a=-32767;
   print ("a  -32767");
   printid(a);

   int i=2;
   print ("i  2");
   func3 (i);

   char m2[2] = { 'a', 'b' };
   char c = m2[0];
   print ("c  a");
   printid(c);

   float f = 2.0;
   print ("f  2.000000");
   printid(f);

   return 0;
}