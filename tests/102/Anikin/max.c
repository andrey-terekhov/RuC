void main() 
  { 
	int i ; 
	int A[10], max; 
	getid (A); 
	max = A[0]; 
	for (i = 0; i < 10; i++) 
		if (A[i] > max) 
			max = A[i]; 
	printid (max); 
   }