float b = 3.14;
int a = 3; 

void MAIN()
{
    b = a + 4 - b;

    assert(b == 3.86, "b must be 3.86");
}
