#define a 1

#macro l(b)
#set b #eval(b + 1)
#set b #eval(b * 2)
#endm

l(a)
l(a)


void main()
{
  a;
}


