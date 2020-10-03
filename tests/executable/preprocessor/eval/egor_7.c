#define s #eval(1.25 - 1.3)

void main()
{
  assert((s + 0.05)*(s + 0.05) < 0.01, "fail1");
}


