void main()
{
	int a, b;
	bool c;
    c = a && b || c;
    c = a || b && c;

    assert(!c, "c must be false");
}
