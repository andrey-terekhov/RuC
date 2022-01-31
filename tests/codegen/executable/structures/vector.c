struct vector
{
    int arr[5];
    int size;
};

int main()
{
    struct vector v;
    v.arr[0] = 1;

    assert(v.arr[0] == 1, "v.arr[0] must be 1");

    return 0;
}