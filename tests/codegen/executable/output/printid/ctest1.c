int a = 11, b = 22, c = 0;
void main()
{
    int d = 1<c ? a : c;
	int s[2][2], l = 1, n = 0, i = 0, nxt = 5;
	s[0][0] = 1;
	s[0][1] = 2;
	s[0][1] = s[0][l-1] + ((l >= n) ? s[0][l-n] : 0) + nxt;
	printid(s);            // 1
    printid(d);            // 0
    b = (14 > a ? a+4 : a-5+10) + (c <= a? a : c);
    printid(b);            // 26
}
