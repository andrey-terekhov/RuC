void main()
{
float numt;
int z=1;
int i,k=0,p=-1,er=0,t;
int num=0,fin;
char c[5000];
for (i=0;i<5000;i++) c[i]=='0';	
	print("Press number");
	getid(c);
for (i=0;i<5000;i++) 
{ 
	if ( (c[i]!='0')&(c [i]!='1')&(c [i]!='2')&(c [i]!='3')&(c [i]!='4')&(c [i]!='5')&(c [i]!='6')&(c [i]!='7')&(c [i]!='8')&(c [i]!='9')&(c [i]!='.')) 
				 { er=1;}
	if (k>1) 
				{  er=1;}
	if (c [i]=='.')  
				{ k++; p=i;}
} 
//—читывание и определение ошибок и позиции точки, если есть

t=0;
for (i=4999;i>-1;i--) 
		{ if ((t==0)&(c[i]!='0'))  
		{ t=1; fin=i;	}
		}

//ќпределение конца ненулевого хвоста




if (p=0) {er=1;} //≈сли точек более одной- ошибка
if (er=0) 
	{
	if (p=-1) //“очек не было, число целое
		{
num=0;
			for (i=0;i<5000;i++) 
					{				
						if (c[i]=='0') { num=10*num+0;}
						if (c[i]=='1') { num=10*num+1;}
						if (c[i]=='2') { num=10*num+2;}
						if (c[i]=='3') { num=10*num+3;}
						if (c[i]=='4') { num=10*num+4;}
						if (c[i]=='5') { num=10*num+5;}
						if (c[i]=='6') { num=10*num+6;}
						if (c[i]=='7') { num=10*num+7;}
						if (c[i]=='8') { num=10*num+8;}
						if (c[i]=='9') { num=10*num+9;}
					} 
					print(num);
		}	
	else 	//“очка одна в p-позиции
		{
			for (i=0;i<5000;i++)
					{	
					if (i!=p) {					//записываем число без точки
						if (c[i]=='0') { numt=10*numt+0;}
						if (c[i]=='1') { numt=10*numt+1;}
						if (c[i]=='2') { numt=10*numt+2;}
						if (c[i]=='3') { numt=10*numt+3;}
						if (c[i]=='4') { numt=10*numt+4;}
						if (c[i]=='5') { numt=10*numt+5;}
						if (c[i]=='6') { numt=10*numt+6;}
						if (c[i]=='7') { numt=10*numt+7;}
						if (c[i]=='8') { numt=10*numt+8;}
						if (c[i]=='9') { numt=10*numt+9;}
						}
					}
			for (i=1;i<(fin-p+1);i++) {z=z*10;}	//определ€ем степень дес€тки дл€ делени€
			numt=numt/(z);
			print(numt);
		}

						
	}						
						
	
if (er==1) print("ERROR");
	
}