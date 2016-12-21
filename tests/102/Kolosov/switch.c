int B[5] = {0, 0, 0, 0 ,0} , i, N;
char ch;

void main ()
{
    getid (N);
    for (i = 0; i < N; i++)
        {
            getid (ch);
            switch (ch)
            {
                case 'а' :
                    B[ 0 ] ++;
                    break;
                case 'б' :
                    B[ 1 ] ++;
                    break;
                case 'в' :
                    B[ 2 ] ++;
                    break;
                case 'г' :
                    B[ 3 ] ++;
                    break;
                case 'д' :
                    B[ 4 ] ++;
                    break;
            }
        }
    printid (B);
}
