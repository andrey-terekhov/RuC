void main ()
{
	int i;
	int b[6]={0, 0, 0, 0, 0, 0};
	char a[10];
	getid(a);
	printid(a);
	for (i=0; i<10; i++)
		switch (a[i])
		{
		case 'а':
 			b[0]++;
 			break;
		case 'б':
 			b[1]++;
 			break;
	 	case 'в':
 			b[2]++;
			break;
		case 'г':
 			b[3]++;
			break;
		case 'д':
			b[4]++;
 			break;
		default:
			b[5]++;
		}
	printid(b);
}