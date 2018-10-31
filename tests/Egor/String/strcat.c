void main()
{
    char s1[] = "abc";
    char s2[] = "defg";
    printid(s1);
    printid(s2);
    STRCAT(&s1, s2);
    printid(s1);
}