void main()
{
    struct somestr {int a; int b;} *pointstr;

    struct somestr  str = {1, 2};

    pointstr = &str;
    
    assert(pointstr->a == 1, "pointstr->a must be 1");
    assert(pointstr->b == 2, "pointstr->b must be 2");       
}
