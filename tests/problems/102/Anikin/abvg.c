void main ()
{
   char  A [10];
   int B [5] ={0,0,0,0,0};
   int i;
   getid (A);
   for (i=0; i<10; i++)
         switch (A[i])
        {
         case 'а':
               B[0]++;
               break;
          case 'б':
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
        break;
      }
         print(B);
}