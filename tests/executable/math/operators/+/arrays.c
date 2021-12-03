int a[5] = {2,3,6,8,9}, b[5] = {3,4,6,9,11}, c[5];
int i;
void main()
{
    for (i=0; i<5; i++)
    {
       c[i] = a[i] + b[i];
    }

    assert(c[0] == 5, "c[0] must be 5");
    assert(c[1] == 7, "c[1] must be 7");
    assert(c[2] == 12, "c[2] must be 12");
    assert(c[3] == 17, "c[3] must be 17");
    assert(c[4] == 20, "c[4] must be 20");
}
