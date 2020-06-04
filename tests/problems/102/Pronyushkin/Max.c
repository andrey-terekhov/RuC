void main()
{
int i, k; 
float A[5], max; 
getid (A); 
max = A[0];  
for (i = 0; i < 5; i++)
{
	if (A[i] > max)
	{
	max = A[i]; 
	k = i + 1; 
	}
}
printid (max); 
printid (k); 
}