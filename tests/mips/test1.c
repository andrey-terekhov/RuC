
void main()
{
    int a = 0, b = 2, c = 3, d = 4;
//    a = b = c = d;
    a = b += c-= d;      //  1
    printf("%i\n", a);
    a = (b += 3) + (c -= 5 + 6);  // -8
    printf("%i\n", a);
}


