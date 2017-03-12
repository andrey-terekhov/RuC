
void main()
{
    int a=9, *b;
    float c = 3.14;
    c &= 1;
    b = &a;
    print(*b);
    (*b)++;
    print(*b);
    ++*b;
    print(*b);

}


