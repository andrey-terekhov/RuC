int a[2] = {2,2}, b[3] = {3,3,3}, c = 4, d = 5;
int i = 0, j = 1;
void main()
{
    ++b[i];
    print(b[0]);        // 4
    ++c;
    printid(c);         // 5
    d = ++b[j];
    printid(d);         // 4
    d = ++c;
    printid(d);         // 6
    --b[1];
    print(b[1]);        // 3
    --i;
    printid(i);         // -1
    c = --b[j];
    printid(c);         // 2
    i = 1; j = 2;
    c = --d;
    printid(c);         // 5
    b[i]++;
    print(b[1]);        // 3
    d++;
    printid(d);         // 6
    d = b[j]++;
    printid(d);         // 3
    c = d++;
    printid(c);         // 3
    b[1]+=i;
    print(b[1]);        // 4
    --d;
    printid(d);         // 3
    d = b[j]--;
    printid(d);         // 4
    d = c--;
    printid(d);         // 3
    printid(c);         // 2
}
