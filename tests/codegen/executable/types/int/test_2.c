int a, b[3] = {7, 8, 9};
int i = 2;

void main()
{
    b[i] -= b[1] += i;

    assert(b[0] == 7, "b[0] must be 7");
    assert(b[1] == 10, "b[1] must be 10");
    assert(b[2] == -1, "b[2] must be -1");
}
