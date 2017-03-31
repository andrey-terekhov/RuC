
void main()
{
    int a=9, *b, e = 1;
    float c = 3.14, *d;
    b = &a;
    print(*b); // 9
    (*b)++;
    print(*b); // 10
    ++*b;
    print(*b); // 11
    
    d = &c;
    print(*d); // 3.14
    (*d)++;
    print(*d); // 4.14
    ++*d;
    print(*d); // 5.14

    a = ++e + ++e;
    printid(a);
    printid(e); // 3
    
    a = e++ + ++e;
    printid(a); // 8
    printid(e); // 5
}


