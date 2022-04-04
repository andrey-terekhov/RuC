void main()
{
    char s1[] = "abc";
    char s2[] = "defg";
    printid(s1);
    printid(s2);
    STRCPY(&s1, s2);
    print(s1);
}
