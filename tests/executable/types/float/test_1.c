float a;
int b;

void main()
{
    a = 1.;

    assert(a == 1., "a must be 1.");

    a = .2;

    assert(a == .2, "a must be .2");

    a = 1.e2;

    assert(a == 1.e2, "a must be 1.e2");

    a = .2e2;

    assert(a == .2e2, "a must be .2e2");

    a = 12e-2;

    assert(a == 12e-2, "a must be 12e-2");  
}
