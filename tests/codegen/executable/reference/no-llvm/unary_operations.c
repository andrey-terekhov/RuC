void main()
{
    int x = 0;
    int &y = x;
    int z;
    z = y++;
    assert(z == 0, "Unary post increment operation error. z must be 0");
    assert(x == 1, "Unary post increment operation error. x must be 1");
    z = y--;
    assert(z == 1, "Unary post decrement operation error. z must be 1");
    assert(x == 0, "Unary post decrement operation error. x must be 0");
    z = ++y;
    assert(z == 1, "Unary pre increment operation error. z must be 1");
    assert(x == 1, "Unary pre increment operation error. x must be 1");
    z = --y;
    assert(z == 0, "Unary pre decrement operation error. z must be 0");
    assert(x == 0, "Unary pre decrement operation error. x must be 0");
    y = -5;
    y = abs(y);
    assert(x == 5, "Unary abs operation error. x must be 5");
    z = -y;
    assert(z == -5, "Unary - operation error. z must be -5");
    bool a = false;
    bool &b = a;
    bool c = !b;

    float i = 0;
    float &j = i;
    float k;
    k = j++;
    assert(k == 0, "Unary floating post increment operation error. k must be 0");
    assert(i == 1, "Unary floating post increment operation error. i must be 1");
    k = j--;
    assert(k == 1, "Unary floating post decrement operation error. k must be 1");
    assert(i == 0, "Unary floating post decrement operation error. i must be 0");
    k = ++j;
    assert(k == 1, "Unary floating pre increment operation error. k must be 1");
    assert(i == 1, "Unary floating pre increment operation error. i must be 1");
    k = --j;
    assert(k == 0, "Unary floating pre decrement operation error. k must be 0");
    assert(i == 0, "Unary floating pre decrement operation error. i must be 0");
    j = -5;
    j = abs(j);
    assert(i == 5, "Unary floating abs opreation error. i must be 5");
    k = -j;
    assert(k == -5, "Unary floating - operation error. k must be -5");
}
