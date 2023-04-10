struct k
{
    int &a;
    int b;
};

k f(int &t)
{
	k a = { t, 2 };
	return a;
}

void main()
{
    int c;
    k a = {c, 2};
    k b = f(c);
    b.a = 4;
    assert(c == 4, "c must be 4");
}