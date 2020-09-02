int f (int n)
{
    if (n == 0)
    return 1; 
    else
    return f(n - 1) * n; 
}
void main ()
{
int n; 
getid (n); 
print (f(n)); 
}