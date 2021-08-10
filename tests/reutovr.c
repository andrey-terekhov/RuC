//  reutovr
//
//  Created by Andrey Terekhov on 12/5/12.
//  Copyright (c) 2012 Andrey Terekhov. All rights reserved.
//

/*AXM
#include <stdio.h>
#include <time.h>
#include <math.h>
*/
#define m 128
#define n 128
#define mn m*n
#define M 512
#define N 512

int main()
{

    DOUBLE input;
/*
    clock_t start_time, end_time;
    start_time = clock();
*/
    int maxk, maxl;
	float Fr, maxFr, ar[m][n];
    char nxt, z[m][N];    
    int s[m][N], s2[m][N], z2[m][N];    
    float zvr, zzr, valr, sqrzzr, aux = 0, aux1;
    int i, ii, iii, i4, jj, k, l, kk, ll, flagend, calca;
    k=0; l=0; i=0; flagend=0; calca=1;    
    maxFr=-1000000;
    input = fopen("/Users/ant/Desktop/Reutov/Reutov/frame3.bmp");
	for (ii=0; ii<1078; ++ii)
        nxt = fgetc(input);
	;
    while (flagend == 0)
    {        
        nxt = fgetc(input);
        z[i][l]=nxt;        
        z2[i][l]=nxt*nxt;        
        if (l == 0)
        {
            s[i][l]=nxt;  s2[i][l]=z2[i][l];
        }
        else
        {
			if(l >= n)
			{
				s[i][l]=s[i][l-1] - z[i][l-n] + nxt;
				s2[i][l]=s2[i][l-1] - z2[i][l-n] + z2[i][l];  // s 12 bit, s2 20
			}
			else
			{
				s[i][l]=s[i][l-1] + nxt;
				s2[i][l]=s2[i][l-1] + z2[i][l];  // s 12 bit, s2 20
			}
		}
        if (k >= m-1 && l >= n-1)
        {
//			aux1 = cos(k);

            zvr=s[0][l]; zzr=s2[0][l];
            
            for (ii=1; ii<m; ++ii)
            {
                zvr=zvr+s[ii][l];  zzr=zzr+s2[ii][l];
            }            
            zvr/=mn;  zzr/=mn; zzr-=zvr*zvr;
			zzr = abs(zzr);
            if (zzr < 0.001) zzr=1;
            sqrzzr = sqrt(zzr);
            kk=k-m+1; ll=l-n+1; Fr=0;
			if (i+1 == m)
				iii = 0;
			else iii = i+1;
            for (ii=0; ii<m; ++ii)
            {
                i4=iii + ii;
				if(i4 >= m)
					i4 = i4 - m;
                for (jj=0; jj<n; ++jj)
                {
                    valr=(z[i4][ll+jj]-zvr)/sqrzzr;
					if ( calca == 1)
						ar[ii][jj]=valr;
                    else
                        Fr += ar[ii][jj] * valr;
                    
                }                
            }
//			aux = sin(i); aux = sin(l);
			aux = sin(ll);

            if (calca == 0)
            {
				printid(kk, ll);
				if(Fr >= maxFr)
				{
					maxFr=Fr; maxk=kk; maxl=ll;
//					printid(maxk, maxl, Fr);
					
				}
            }
        }
//		aux1 = cos(k);
        if (calca == 1 && k == m-1 && l == n-1)
        {
            calca=0; k=0; l=0; i=0;
            
            fclose(input);
            input = fopen("/Users/ant/Desktop/Reutov/Reutov/image3.bmp");
            for (ii=0; ii<1078; ++ii)
				nxt = fgetc(input);
			printid(ii);
        }
        else if (++l == N && k == M-1)
			flagend=1;
             else if ((calca == 1 && l == n) || ( calca == 0 && l == N))
             {
				 //aux1 = cos(k);
                 ++k; l=0;                
                 if (++i == m)                    
                     i=0;
             }
    };
    fclose(input);
	printid(maxk, maxl, maxFr);
//    printf("maxk =%i maxl=%i maxF=%f\n", maxk, maxl, maxFr);
	/*
	end_time = clock();
   clock_t diffTime = end_time - start_time;
    printf("%li\n", diffTime);
    */
    return 0;
}


