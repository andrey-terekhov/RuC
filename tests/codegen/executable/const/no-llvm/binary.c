void main()
{
    const int y = 3;
    int z = 2;
    z = y % z;
    assert(z == 1, "Binary % operator error. z must be 1");
    z = y << y;
    assert(z == 24, "Binary << operator error. z must be 24");
    z = y >> y;
    assert(z == 0, "Binary >> operator error. z must be 0");
    z = y & 2;
    assert(z == 2, "Binary & operator error. z must be 2");
    z = y | 4;
    assert(z == 7, "Binary | operator error. z must be 7");
    z = y ^ y;
    assert(z == 0, "Binary ^ opreation error. z must be 0");

    z = y * y;
    assert(z == 9, "Binary * operation error. z must be 9");
	z = y / (y >> 1);
    assert(z == 3, "Binary / operation error. z must be 3");
    z = y + y;
    assert(z == 6, "Binary + operation error. z must be 6");
	z = y - (y << 1);
    assert(z == -3, "Binary - operation error. z must be -3");
}