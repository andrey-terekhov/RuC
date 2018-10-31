void main()
{
    char s1[] = "qwerwertyui";
    char s2[] = "werty";
    int n;
    printid(s1);
    printid(s2);
    n = STRSTR(s1, s2);
    printid(n);
}
