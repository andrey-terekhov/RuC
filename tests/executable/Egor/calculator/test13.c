#define A 10
#define sum(a,b) a + b
#define s #eval( sum(A, A) / 3.0) 

void main()
{
 assert((s - 13.33)*(s - 13.33) < 0.01, "fail1");
}