int a=1,b[3] = {1,2,3};
int i;
void main()
{
    i=1;
    b[i+1] = b[a-i];
    printid(b);
}