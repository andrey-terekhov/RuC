void main()
{
    float x[1];
    double y = .01, z;
    x[0] = 3.14;

    assert(abs(x[0] + y - 3.15) < 0.000001, "x[0] + y must be 3.15");
    assert(abs(x[0] - y - 3.13) < 0.000001, "x[0] - y must be 3.13");
    assert(abs(x[0] * y - 0.0314) < 0.000001, "x[0] * y must be 0.0314");
    assert(abs(x[0] / y - 314) < 0.000001, "x[0] / y must be 314");
    
    x[0] += y;       // 3.15

    assert(abs(x[0] - 3.15) < 0.000001, "x[0] must be 3.15");

    x[0] -= y;       // 3.14

    assert(abs(x[0] - 3.14) < 0.000001, "x[0] must be 3.14");

    x[0] *= y;       // 0.0314

    assert(abs(x[0] - 0.0314) < 0.000001, "x[0] must be 0.0314");

    x[0] /= y;       // 3/14

    assert(abs(x[0] - 3.14) < 0.000001, "x[0] must be 3.14");
    
    z = x[0] += y;   // 3.15

    assert(abs(z - 3.15) < 0.000001, "z must be 3.15");

    z = x[0] -= y;   // 3.14

    assert(abs(z - 3.14) < 0.000001, "z must be 3.14");

    z = x[0] *= y;   // 0.0314

    assert(abs(z - 0.0314) < 0.000001, "z must be 0.0314");

    z = x[0] /= y;   // 3.14

    assert(abs(z - 3.14) < 0.000001, "z must be 3.14");
}
