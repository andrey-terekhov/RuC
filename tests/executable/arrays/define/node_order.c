void main()
{
	float a = 3.14, b = 2.72, a1 = 5.0, b2 = 3.6;
	float c[2] = { (a + b) * 7.6 + a1 / b2 - 3.0, (a + b) * 7.6 + a1 / b2 - 3.0 };

	assert(abs(c[0] - 42.924) < 0.001, "c[0] must be 42.924(8)");
    assert(abs(c[1] - 42.924) < 0.001, "c[1] must be 42.924(8)");
}
