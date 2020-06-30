 int main()
{
   float a[2]={1,2};
   
   a[0]= a[1] ^= a[1]^a[0]^a[1];
   print ("a 1.000000 1.000000");
   printid(a);

   return 0;
}
