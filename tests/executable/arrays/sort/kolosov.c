float A[10] = {1, 23.3, 32.2, -13.1, 2, 2, 3.14, 2.71, 2.31, 1.32}, c;
int i, j;

void main()
{
    for (i = 0; i < 9; i++)
        for (j = 0; j < 9 - i; j++)
            if (A[j] > A[j + 1])
            {
                c = A[j];
                A[j] = A[j + 1];
                A[j + 1] = c;
            }

    assert(A[0] == -13.1, "A[0] must be -13.1");
    assert(A[1] == 1, "A[1] must be 1");
    assert(A[2] == 1.32, "A[2] must be 1.32");
    assert(A[3] == 2, "A[3] must be 2");
    assert(A[4] == 2, "A[4] must be 2");
    assert(A[5] == 2.31, "A[5] must be 2.31");
    assert(A[6] == 2.71, "A[6] must be 2.71");
    assert(A[7] == 3.14, "A[7] must be 3.14");
    assert(A[8] == 23.3, "A[8] must be 23.3");
    assert(A[9] == 32.2, "A[9] must be 32.2");   
}