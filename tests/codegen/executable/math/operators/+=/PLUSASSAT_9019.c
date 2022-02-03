int b[3];

void main()
{
   int a = 0;	
   a = b[1] += 2;

   assert(a == 2, "a must be 2");
}
