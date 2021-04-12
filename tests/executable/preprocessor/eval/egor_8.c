#define A 10
#define sum(a,b) a + b
#define s #eval( sum(A, A) / 3) 

void main()
{
 assert(s == 13, "fail1");
}