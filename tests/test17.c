int i, a, j;
void main()
{
    for ((i=0, j=9); i<10 && j>-i; (i++, j--))
    {
        a+=i+j;
    }
}
