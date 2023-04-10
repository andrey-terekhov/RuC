void main()
{
    int x = 3;
    int &y = x;
    int z = 2;
    z = y % z;
    assert(z == 1, "Binary % operator error. z must be 1");
    z = y << y;
    assert(z == 24, "Binary << operator error. z must be 24");
    z = y >> y;
    assert(z == 0, "Binary >> operator error. z must be 0");
    y = 3;
    z = y & 2;
    assert(z == 2, "Binary & operator error. z must be 2");
    z = y | 4;
    assert(z == 7, "Binary | operator error. z must be 7");
    z = y ^ y;
    assert(z == 0, "Binary ^ opreation error. z must be 0");

    y = 10;

    z = y * y;
    assert(z == 100, "Binary * operation error. z must be 100");
	z = y / (y >> 1);
    assert(z == 2, "Binary / operation error. z must be 2");
    z = y + y;
    assert(z == 20, "Binary + operation error. z must be 20");
	z = y - (y << 1);
    assert(z == -10, "Binary - operation error. z must be -10");
    bool c;
    c = y > y;
    assert(!c, "Binary > opreation error. c must be false");
    c = y < y;
    assert(!c, "Binary < opreation error. c must be false");
    c = y >= y;
    assert(c, "Binary >= opreation error. c must be true");
    c = y <= y;
    assert(c, "Binary <= opreation error. c must be true");

    c = x == y;
    assert(c, "Binary == opreation error. c must be true");
    c = y != y;
    assert(!c, "Binary != opreation error. c must be false");

    bool a = false;
    bool &b = a;
    c = b && b;
    assert(!c, "Binary && opreation error. c must be false");
    b = true;
    c = b || b;
    assert(c, "Binary || opreation error. c must be true");

	y %= 7;
    assert(x == 3, "Binary %= opreation error. x must be 3");
    y <<= y;
    assert(x == 24, "Binary <<= opreation error. x must be 24");
    y >>= 4;
    assert(x == 1, "Binary >>= opreation error. x must be 1");
    y = 7;
    y &= 5;
    assert(x == 5, "Binary &= opreation error. x must be 5");
    y |= 16;
    assert(x == 21, "Binary |= opreation error. x must be 21");
    y ^= y;
    assert(x == 0, "Binary ^= opreation error. x must be 0");

    y = 10;
    y *= y;
    assert(x == 100, "Binary *= opreation error. x must be 100");
    y /= 11;
    assert(x == 9, "Binary /= operation error. x must be 9");
    y += y;
    assert(x == 18, "Binary += operation error. x must be 18");
    y -= 7;
    assert(x == 11, "Binary -= operation error. x must be 11");

    float i = 3;
    float &j = i;
    float k;
    k = j * j;
    assert(k == 9, "Binary floating * operation error. k must be 9");
    k = j / j;
    assert(k == 1, "Binary floating / operation error. k must be 1");
    k = j + j;
    assert(k == 6, "Binary floating + operation error. k must be 6");
    k = j - j;
    assert(k == 0, "Binary floating - operation error. k must be 0");

    c = j > j;
    assert(!c, "Binary floating > operation error. c must be false");
    c = j < j;
    assert(!c, "Binary floating < operation error. c must be false");
    c = j >= j;
    assert(c, "Binary floating >= operation error. c must be true");
    c = j <= j;
    assert(c, "Binary floating <= operation error. c must be true");

    c = j == j;
    c = j != j;

    j = 10;
    j *= j;
    assert(i == 100, "Binary floating *= opreation error. i must be 100");
    j /= 5;
    assert(i == 20, "Binary floating /= opreation error. i must be 20");
    j += j;
    assert(i == 40, "Binary floating += opreation error. i must be 40");
    j -= 15;
    assert(i == 25, "Binary floating -= opreation error. i must be 15");
}