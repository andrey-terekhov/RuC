int  b[3], c, d;
void main()
{
    b[1]++; 
    d++;
    d = b[1]++; 
    c = d++;
    print("b 0 2 0");
    printid(b);
    print("c 1");
    printid(c);
    print("d 2");
    printid(d);
}