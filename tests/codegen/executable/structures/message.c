void main()
{
    struct message{int numTh; int data;} m = {1, 2};

    assert(m.numTh == 1, "m.numTh must be 1");
    assert(m.data == 2, "m.data must be 2");
}
