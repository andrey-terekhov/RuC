#define s #eval(1.25 * 1.3)

void main()
{
 assert((s - 1.625)*(s - 1.625) < 0.01, "fail1");
}


