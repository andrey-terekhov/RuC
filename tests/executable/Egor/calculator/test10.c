#define s #eval(1.25 * (1.3 - 1) + 31/5) 

void main()
{
 assert((s - 6.575)*(s - 6.575) < 0.01, "fail1");
}


