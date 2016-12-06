struct{
    struct{int a[2]; float b[3];}c[4][5];
    struct{char d[6];}e[7];
}f[8];

void main()
{
    int r = f[0].c[0][0].a[0] = 11;
    print(r);
}