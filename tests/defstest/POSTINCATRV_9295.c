float  b[3], c, d;
void main()
{
    b[1]++; 
    d++;
    d = b[1]++; 
    c = d++;
    print("0.000000 2.000000 0.000000");
    print(b);
    print("1.000000");
    print(c);
    print("2.000000");
    print(d);
}
