#define s #eval(1.25 - 1.3)

void main()
{
  assert((s + 0.5)*(s + 0.5) < 0.01, "fail1");
}


