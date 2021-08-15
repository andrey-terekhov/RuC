#define a 0
#while a < 5
  #set a #eval(a + 2)
#endw

void main()
{
  assert(a == 6, "fail1");
}


