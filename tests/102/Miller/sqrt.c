void main()
{
 float m,n;
 print("¬ведите число");
 getid(n);
 m=n;
 while (abs(n*n-m)>1e-6)
     n=(n+m/n)/2;
 print(" вадратный корень из числа\n");
 print(n);
}
//Miller A.
