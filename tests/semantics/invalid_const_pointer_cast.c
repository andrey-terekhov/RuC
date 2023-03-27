// Cannot make pointer to variable out of pointer to const
void main()
{
    int a;
    const int *b = &a;
    int *c = b;
}