int b[3] = {4, 5, 6};
int j = 1, k;
void main()
{
    k = --b[j];

    assert(k == 4, "k must be 4");
}
