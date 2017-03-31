int a[3]={1,2,3}, b[3]={4,5,6};
int i=0,j=1, k;
void main()
{
    ++b[i];
    printid(b); // 5 5 6
    ++i;
    printid(i); // 1
    k = ++b[j];
    printid(k); // 6
    printid(b); // 5 6 6
    k = ++i;
    printid(k); // 2
    
    --b[1];
    printid(b); // 5 5 6
    --i;
    printid(i); // 1
    k = --b[j];
    printid(k); // 4
    printid(b); // 5 4 6
    k = --i;
    printid(k); // 0
    
    b[i]++;
    printid(b); // 6 4 6
    k++;
    printid(k); // 1
    k = b[j]++;
    printid(k); // 4
    printid(b); // 6 5 6
    k = j++;
    printid(k); // 1
    printid(j); // 2
    
    b[1]--;
    printid(b); // 6 4 6
    --i;
    printid(i); // -1
    i = b[j]--;
    printid(i); // 6
    printid(b); // 6 4 5
    j = i--;
    printid(j); // 6
    printid(i); // 5
}
