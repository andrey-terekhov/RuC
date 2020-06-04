void main() 
{ 
    int i ; 
    int A[5], max; 
    getid (A); 
    max = A[0]; 
    for (i = 1; i < 5; i++)
        if (A[i] > max) 
        max = A[i]; 
    printid (max); 
}
