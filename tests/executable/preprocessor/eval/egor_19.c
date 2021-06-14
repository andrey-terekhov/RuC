#define s #eval(10 / 3.0) 

void main()
{
 assert((s - 3.33)*(s - 3.33) < 0.01, "fail1");
}