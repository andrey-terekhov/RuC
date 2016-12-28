float a[3], b[3], c, d;
int i,j;
void main()
{
    ++b[i]; ++c;
    d = ++b[j]; d = ++c;
    --b[1]; --i;    
    c = --b[j]; c = --d;
    b[i]++; d++;
    d = b[j]++; c = d++;
    b[1]+=i; --d;
    d = b[j]--; d = c--;
}