int b[3];

void main()
{
   b[1] += 2;

   assert(b[0] == 0, "b[0] must be 0");
   assert(b[1] == 2, "b[1] must be 2");
   assert(b[2] == 0, "b[2] must be 0");
}