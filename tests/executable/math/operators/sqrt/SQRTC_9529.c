float a;
int b;
void main()
{
    a = sin(3.14 / 2);

    assert(a == 1, "a must be 1");

    a = cos(0);

    assert(a == 1, "a must be 1");

    a = sqrt(1.e2);

    assert(a == 10.000000, "a must be 10.000000");

    a = abs(-2.72);

    assert(a == 2.720000, "a must be 2.720000");

    a = abs(-3);

    assert(a == 3.000000, "a must be 3.000000");
    /* sin(3.14/2)=1, cos(0)=1, sqrt(1.e2)=10, abs(-2.72)=2.72, abs(-3)=3 */
}

