#define s #eval(1.25 + 1.3)

void main()
{
 assert((s - 2.55)*(s - 2.55) < 0.01, "fail1");
}


