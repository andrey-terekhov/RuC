#define A 10
#define s #eval(A / 3.0) 

void main()
{
 assert((s - 3.33)*(s - 3.33) < 0.01, "fail1");
}