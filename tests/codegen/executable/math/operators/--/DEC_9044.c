int  b[3] = {4, 5, 6};
int i = 2, j = 1, p;

void main()
{
    --b[1]; 
    --i;
    p = --b[j]; 
    p = --i;

    assert(b[0] == 4, "b[0] must be 4");
    assert(b[1] == 3, "b[1] must be 3");
    assert(b[2] == 6, "b[2] must be 6");

    assert(p == 0, "p must be 0");  
}
