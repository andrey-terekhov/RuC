void main () 
{ 
	int i,j,n,x; 
	float str,c; 
	getid (n);  
	{
	float A[n][n], B[n], X[n]; 
	getid (A); 
	getid (B);  
	for(i=0;i<n;i++) 
		for (j=i+1;j<n;j++) 
		{ 
			c=A[j][i]/A[i][i]; 
			for(x=0;x<n;x++) 
				A[j][x]=A[j][x]-A[i][x]*c; 
			B[j]=B[j]-B[i]*c; 
		} 
	for(i=n-1;i>-1;i--) 
	{ 
		str=B[i]; 
		for (j=i+1;j<n;j++) 
			str=str-A[i][j]*X[j]; 
		X[i]=str/A[i][i]; 
	} 
	printid(X); 
	}
}
