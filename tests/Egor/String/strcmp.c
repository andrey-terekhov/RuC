void main()
{
    char s1[] = "qwertyuia";
    char s2[] = "qweratyui";
    int n;
    printid(s1);
    printid(s2);
    n = STRCMP(&s1, s2);
    print(n);

}