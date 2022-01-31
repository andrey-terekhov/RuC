float b[3];

void main()
{
   float f = b[1] += 2;

   assert(f == 2.000000, "f must be 2.000000");
}