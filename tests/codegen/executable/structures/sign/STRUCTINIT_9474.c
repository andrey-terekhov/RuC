void main()
{
    struct somestr {int a; int b;} *pointstr;

    struct somestr  str = {2, 2};

    pointstr = &str;

    assert(pointstr->a == 2, "pointstr->a must be 2");
    assert(pointstr->b == 2, "pointstr->b must be 2");     
}
