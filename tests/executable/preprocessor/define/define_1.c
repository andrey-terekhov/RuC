#define bplusc b + c
void main()
{
    int a, b = 1, c = 2;
    a = bplusc;

    assert(a == 3, "a must be 3");
}


