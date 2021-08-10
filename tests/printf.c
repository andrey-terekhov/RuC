// #include <stdio.h>
// #include <stddef.h>


// void func()
// {
// 	printf("func\n");
// }

// int func1()
// {
// 	int i = 3;

// 	return i;
// }

// double func1d()
// {
// 	double i = 3.5;

// 	return i;
// }

// int func2(int i, int j)
// {
// 	return i + j;
// }

// double func2d(double i, double j)
// {
// 	return i + j;
// }

// int func_arr(int arr[])
// {
// 	return arr[0];
// }

// int func_arr1(int arr[])
// {
// 	return arr[0];
// }


int main()
{
	// struct point
	// {
	// 	int	x;
	// 	int y;
	// };

	int i = 5, j = 6;
	// char c = 'x';
	// long li = 13;
	double x = 3.14, y = 2.72, z = 5.1;
	// int *pointer; 
	// pointer = &i;
	// *pointer = 1;
	// (*pointer)++;
	// *pointer = -1;
	// j += 3 + *pointer;
	// struct point a, b, c;
	// char image_file[] = "i0001.bmp";
	// double f[4] = {1.1, 2.2, 3.3, 4.4};
	// int f1[] = {1, 2, 3};
	// double a[5][3];
	int a[5];
	// int a1[5][6];						
	// double c[j][i];
	// double b[5][4];
	// int d[i][j];

	// if (i >= j || i > 1) // True
	// if (i >= j && i > 1) // False
	// if (i >= j || i > 1 || i >= j && i > 1) // True
	// if ((i >= j || i <= 1) && i >= j && i > 1) // False
	// if (!(i == 5)) // False
	// if (!(i == 5) || j == 6) // True
	// if (i == 5 && !(j == 6)) // False
	// if (!(i == 5 || j == 6) && i > 2) // False
	// if (3.14 > 2.5)
	// if (0)
	// {
	// 	printf("True\n");
	// }
	// else
	// {
	// 	printf("False\n");
	// }

	// if (i != 5)
	// {
	// 	if (j == 6)
	// 	{
	// 		printf("1\n");
	// 	}
	// 	else
	// 	{
	// 		printf("2\n");
	// 	}	
	// }
	// else
	// {
	// 	if (j == 6)
	// 	{
	// 		printf("3\n");
	// 	}
	// 	else
	// 	{
	// 		printf("4\n");
	// 	}
	// }

	// while (i)
	// {
	// 	printf("i = %i\n", i);
	// 	i--;
	// 	while (j != 0)
	// 	{
	// 		printf("j = %i\n", j);
	// 		j--;
	// 	}
	// 	j = i;
	// }

	// do
	// {
	// 	printf("%i\n", i);
	// 	i--;
	// 	while (j != 0)
	// 	{
	// 		printf("j = %i\n", j);
	// 		j--;
	// 	}
	// 	j = i;
	// } while (i);
	
	// for (i = 0; i < 10; i++)
	// {
	// 	printf("i = %i\n", i);

	// 	for (j = 2; j != 0; --j)
	// 	{
	// 		printf("j = %i\n", j);
	// 	}
	// }
	
	// while (i > 0)
	// {
	// 	i--;
	// 	if (i == 3)
	// 		continue;
	// 	printf("i = %i\n", i);
	// }
	
	// goto a;


	
	// j += -(i > 2 > 3) * (i != 2); // почему-то не сработало, надо разобраться
	// j = 3 > 2;
	// i = j * i + 3 / (j - 3); // 31
	// j++;
	// i += ++j;
	// func();
	// i += func1() / 3 - 2;
	// i = func2(j, 3) + 1;
	a[3] = 0;
	// i = a[3] + 3;

	// for (i = 0; i < 5; i++)
	// 	a[i] = i + 1;
	// for (i = 0; i < 5; i++)
	// 	printf("a[%i] = %i\n", i, a[i]);

	// for (i = 0; i < 5; i++)
	// {
	// 	for (j = 0; j < 6; j++)
	// 	{
	// 		a1[i][j] = i + j;
	// 		a1[i][j] = i * j + a1[i][j];
	// 	}
	// }
	// for (i = 0; i < 5; i++)
	// {
	// 	for (j = 0; j < 6; j++)
	// 	{
	// 		printf("a1[%i][%i] = %i\n", i, j, a1[i][j]);
	// 	}
	// }

	// x = 3.6 - 2.1 + --y / 3.1;
	// i += 1 + j;
	// i = ~j;
	// x /= 2.3 * --z + y - 1.5;
	// x = func2d(1.0, z);
	// x -= i - y;
	// b[0][1] = b[0][1] * 3.1; // ОШИБКА!!!
	// a.x = 0;
	// a.y = -1;
	// i = a.y;
	// func_arr(a);
	// if (i != 5)
	// 	exit(0);
	printf("%i %i %f\n", i, j, x);
	// printf("hello world!\n");
	// a:

	// wchar_t c = L'щ';
	// printf("АБОБА\n");

	// wchar_t ru[] = L"Привет"; //Russian language
	// wprintf(ru);

	return 0;
}
