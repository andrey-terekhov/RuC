float  b[3], c, d;
void main()
{
   d = b[2]++;
    c = d++;
    b[1]+=7; 
    print("0.000000 7.000000 1.000000");
    print(b);
    print("0.000000");
    print(c);
    print("1.000000");
    print(d);
}
