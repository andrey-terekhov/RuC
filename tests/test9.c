int a[3]={1,2,3}, b[3]={4,5,6};
int i=0,j=1, k;
void main()
{
    ++b[i]; ++i;
    printid(b);
    k = ++b[j]; k = ++i;
    
    --b[1]; --i;
    
    k = --b[j]; k = --i;
    
    b[i]++; k++;
    
    k = b[j]++; k = j++;
    
    b[1]+=i; --i;
    
    i = b[j]--; j = i--;
}