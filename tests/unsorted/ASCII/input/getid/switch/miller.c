int a,b,i;
int c1[5];
char z;
void main()
{
for (i=0; i<5;i++)
 {
  c1[i]=0;
 }
print("Введите строку");
for (i=0;i<10;i++)
 {
  getid(z);
  switch (z)
   {
    case 'a':
           c1[0]++;
           break;
    case 'b':  c1[1]++; break;
    case 'c':  c1[2]++; break;
    case 'd':  c1[3]++; break;
    case 'e': c1[4]++; break;
    default: break;
   }
 }
print("Частота a,b,c,d,e\n");
for (i=0; i<5;i++) 
 {          								 
 print(c1[i]);
 }
}
//Miller A.
