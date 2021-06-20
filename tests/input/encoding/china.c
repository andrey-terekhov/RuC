int a[10];

void main()
{
	int n, i;
	print("¢夨袢");
	getid(a);

	n = a[0];
	for (i = 0; i < 10; i++)
		if (a[i] > n)
			n = a[i];

	print("̠걨쳬\n");
	print(n);
}
