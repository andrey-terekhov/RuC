int f(int res)
{
if ((res-1)>=1) 
  {
	res=res*f(res-1);
	return res;
  }  
 else return 1;
}
void main()
{
int n;
print("Введите число");
getid(n);
print("Факториал числа\n");
if (f(n)>0)
 print(f(n));
else       
 print("Incorrectly");
}
//Miller A.