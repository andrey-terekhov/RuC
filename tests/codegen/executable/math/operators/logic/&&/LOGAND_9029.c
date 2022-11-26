int i, j;
bool a;
void main()
{
    a = ( i < 10 && j > -i);

    assert(!a, "a must be false");
}
