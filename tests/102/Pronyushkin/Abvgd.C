void main ()
{
char A[10]; 
int i = 0; 
float a; 
int B[6] = {0, 0, 0, 0, 0, 0}; 
getid (A); 
print (A); 
while (i < 10)
{
	switch (A[i])
	{
    		case 'а':
		B[0]++;  
		break; 
		case   'б': 
        		B[1]++; 	
		break; 
		case 'в': 
		B[2]++; 
		break; 
		case 'г':
		B[3]++; 
		break; 
		case 'д': 
		B[4]++; 
		break; 
		default:
		B[5]++; 		 // Прочие буквы
		break; 
	}
	i ++; 
}
print (B); 

} 