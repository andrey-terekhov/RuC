#define a 1
#set a #eval(a+1)
#set a #eval(a+1)
#set a #eval(a+1)
void main()
{
  assert(a == 4, "fail1");
}