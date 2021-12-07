void main()
{
    int AA[2];
    int BB[2][2];
    int A[2] = {1, 2};
    int B[]  = {1, 2, 3};
    int C[2][2] = {{1, 2}, {3, 4}};
    int D[2][]  = {{1, 2}, {3, 4, 5}};

    assert(A[0] == 1, "A[0] must be 1");
    assert(A[1] == 2, "A[1] must be 2");

    assert(B[0] == 1, "B[0] must be 1");
    assert(B[1] == 2, "B[1] must be 2");
    assert(B[2] == 3, "B[2] must be 3");

    assert(C[0][0] == 1, "C[0][0] must be 1");
    assert(C[0][1] == 2, "C[0][1] must be 2");
    assert(C[1][0] == 3, "C[1][0] must be 3");
    assert(C[1][1] == 4, "C[1][1] must be 4");

    assert(D[0][0] == 1, "D[0][0] must be 1");
    assert(D[0][1] == 2, "D[0][1] must be 2");
    assert(D[1][0] == 3, "D[1][0] must be 3");
    assert(D[1][1] == 4, "D[1][1] must be 4");
    assert(D[1][2] == 5, "D[1][2] must be 5");
}
