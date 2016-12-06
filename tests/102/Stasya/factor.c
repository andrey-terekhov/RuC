int n;
int F(int n)
{
	if (n==0)
	return 1;
	else
	return F(n-1)*n;
}
void main ()
{
getid(n);
printid(n);
print(F(n));
}