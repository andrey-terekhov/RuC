void main()
{
    char s1[] = "abc";
    char s2[] = "defg";
    printid(s1);
    printid(s2);
    STRNCAT(&s1, s2, 3);
    printid(s1);
}