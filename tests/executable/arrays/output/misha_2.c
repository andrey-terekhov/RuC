void main()
{
    int A[2][3][2] = 
    { 
        {{1, 2}, {3, 4}, {5, 6}}, 
        {{7, 8}, {9, 10}, {11, 12}} 
    };

    assert(A[0][0][0] == 1, "A[0][0][0] must be 1");
    assert(A[0][0][1] == 2, "A[0][0][1] must be 2");
    assert(A[0][1][0] == 3, "A[0][1][0] must be 3");
    assert(A[0][1][1] == 4, "A[0][1][1] must be 4");
    assert(A[0][2][0] == 5, "A[0][2][0] must be 5");
    assert(A[0][2][1] == 6, "A[0][2][1] must be 6");
    assert(A[1][0][0] == 7, "A[1][0][0] must be 7");
    assert(A[1][0][1] == 8, "A[1][0][1] must be 8");
    assert(A[1][1][0] == 9, "A[1][1][0] must be 9");
    assert(A[1][1][1] == 10, "A[1][1][1] must be 10");
    assert(A[1][2][0] == 11, "A[1][2][0] must be 11");
    assert(A[1][2][1] == 12, "A[1][2][1] must be 12");
}

