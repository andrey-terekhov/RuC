float  b[3], c, d;
void main()
{
    b[1]++; 
    d++;
    d = b[1]++; 
    c = d++;
    print ("b 0.000000 2.000000 0.000000");
    printid(b);
    print ("c 1.000000");
    printid(c);
    print ("d 2.000000");
    printid(d);
}
