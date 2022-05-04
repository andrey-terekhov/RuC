int a = 1, b[3] = {1, 2, 3};
int i;

void main()
{
    i = 1;
    b[i + 1] = b[a - i];

    assert(b[0] == 1, "b[0] must be 1");
    assert(b[1] == 2, "b[1] must be 2");
    assert(b[2] == 1, "b[2] must be 1");
}