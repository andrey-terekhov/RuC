int  b[3], c, d;
void main()
{
   d = b[2]++;
    c = d++;
    b[1]+=7; 
    print("0 7 1");
    print(b);
    print("0");
    print(c);
    print("1");
    print(d);
}
