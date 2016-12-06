void main ()
{
int a[5];  
int i, j, t;
getid (a);  
for (i = 0; i < 4; i++)
	for (j = i + 1; j < 5; j++)
		if (a[i] > a[j])
		{
		t = a[i]; 
		a[i] = a[j]; 
		a[j] = t; 
		}
	
print (a); 
}