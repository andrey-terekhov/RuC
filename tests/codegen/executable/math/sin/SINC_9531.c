float a;
int b;
void main()
{
    a = sin(3.14 / 2);

    assert(abs(a - 1) < 0.000001, "a must be 1");

    a = cos(0);

    assert(abs(a - 1) < 0.000001, "a must be 1");

    a = sqrt(1.e2);

    assert(abs(a - 10) < 0.000001, "a must be 10.000000");

    a = abs(-2.72);

    assert(abs(a - 2.72) < 0.000001, "a must be 2.720000");

    a = abs(-3);

    assert(abs(a - 3) < 0.000001, "a must be 3.000000");
    /* sin(3.14/2)=1, cos(0)=1, sqrt(1.e2)=10, abs(-2.72)=2.72, abs(-3)=3 */ 
}

