#define a 1

#macro l()
  #set a #eval(a + 1)
  #set a #eval(a * 2)
#endm

l
l


void main()
{
  assert(a == 10, "fail1");
}


