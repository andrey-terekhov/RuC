struct{int a; float b;}c = {13, 3.14}, d; // = {{1,2}, {1.1, 2.2, 3.3}};

void main()
{
    d = c;
    print(c);
    print(d);
    print(c.a);
    print(c.b);
}
