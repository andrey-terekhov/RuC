int  b[3] = {4, 5, 6};
void main()
{
    int r;
    r = b[2]--; 
    r = r--;

    assert(r == 6, "r must be 6");
}
