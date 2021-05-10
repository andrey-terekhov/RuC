/*
 *	Copyright 2014 Andrey Terekhov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include "global_vars.h"


//extern void fprintf_char(FILE *f, int wchar);


void printf_char(int wchar)
{
    if (wchar<128)
        printf("%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;
        
        printf("%c%c", first, second);
    }
}

void fprintf_char(FILE *f, int wchar)
{    if (wchar<128)
    fprintf(f, "%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;
        
        fprintf(f, "%c%c", first, second);
    }
}

int getf_char()
{
    // reads UTF-8
    
    unsigned char firstchar, secondchar;
    
    if (scanf(" %c", &firstchar) == EOF)
        return EOF;
    else
        if ((firstchar & /*0b11100000*/0xE0) == /*0b11000000*/0xC0)
        {
            scanf("%c", &secondchar);
            return ((int)(firstchar & /*0b11111*/0x1F)) << 6 | (secondchar & /*0b111111*/0x3F);
        }
        else
            return firstchar;
}


void tablesandtree()
{
    int i=0, j, n;
    double d;
    
    fprintf(output, "\n%s\n", "source");
    for (i=1; i<line; i++)
    {
        fprintf(output, "line %i) ", i);
        for (j=lines[i]; j<lines[i+1]; j++)
        {
            fprintf_char(output, source[j]);
        }
    }
    fprintf(output, "\n");
    
    fprintf(output, "\n%s\n", "identab");
    i = 2;
    while (i < id)
    {
        for (j=0; j<4; j++)
            fprintf(output, "id %i) %i\n", i +j, identab[i+j]);
        fprintf(output, "\n");
        i +=4;
    }
/*
	fprintf(output, "\n%s\n", "repr");
	for (i = 1206; i <= rp; i++)
		fprintf(output, "rp %i) %i\n", i, reprtab[i]);
 */
    fprintf(output, "\n%s\n", "modetab");
    for (i=0; i<md; i++)
        fprintf(output, "md %i) %i\n", i, modetab[i]);
/*
    fprintf(output, "\n%s\n", "tree");
    for (i=0; i<=tc; i++)
        fprintf(output, "tc %i) %i\n", i, tree[i]);
*/
    fprintf(output, "\n");
    i = 0;
    while (i < tc)
    {
        int t; ;
        fprintf(output, "tc %i) ", i);
        t = tree[i++];
        switch (t > 10000 ? t-1000 : t)
        {
            case TFuncdef:
                fprintf(output, "TFuncdef funcn= %i maxdispl= %i\n", tree[i], tree[i+1]);
				i += 2;
                break;
            case TDeclarr:
                fprintf(output, "TDeclarr N= %i\n", tree[i++]);
                break;
            case TDeclid:
                fprintf(output, "TDeclid ident= %i eltype= %i N= %i all= %i iniproc= %i,"
                        "usual= %i instuct= %i\n",
                    tree[i], tree[i+1], tree[i+2], tree[i+3], tree[i+4], tree[i+5], tree[i+6]);
				i += 7;
                break;
            case TStringform:
                fprintf(output, "TStringform n= %i\n", tree[i++]);
                break;
            case TString:
                fprintf(output, "TString n= %i\n", tree[i++]);
                break;
            case TStringc:
                fprintf(output, "TStringc n= %i\n", tree[i++]);
                break;
            case TStringf:
                fprintf(output, "TStringf n= %i\n", n = tree[i++]);
                for (j=0; j<n; ++j)
                {
                    memcpy(&d, &tree[i], sizeof(double));
                    i += 2;
                    fprintf(output, "%f\n", d);
                }
                break;
            case TCondexpr:
                fprintf(output, "TCondexpr\n");
                break;
            case TBegin:
                fprintf(output, "TBegin\n");
                break;
            case TEnd:
                fprintf(output, "TEnd\n");
                break;
            case TBeginit:
                fprintf(output, "TBeginit n= %i\n", tree[i++]);
                break;
            case TStructinit:
                fprintf(output, "TStructinit n= %i\n", tree[i++]);
                break;
             case TIf:
                fprintf(output, "TIf %i\n", tree[i++]);
                break;
            case TWhile:
                fprintf(output, "TWhile\n");
                break;
            case TDo:
                fprintf(output, "TDo\n");
                break;
            case TFor:
				if (check_nested_for)
				{
					fprintf(output, "TFor %i %i %i %i %i\n", tree[i], tree[i+1], tree[i+2], tree[i+3], tree[i+4]);
					i += 5;
				}
				else
				{
					fprintf(output, "TFor %i %i %i %i\n", tree[i], tree[i+1], tree[i+2], tree[i+3]);
					i += 4;
				}
				break;
			case TForEnd:
				fprintf(output, "TForEnd\n");
				break;
			case TIndVar:
				fprintf(output, "TIndVar number = %i\n", tree[i++]);
				break;
			case TSliceInd:
				fprintf(output, "TSliceInd %i\n", tree[i++]);
				break;
            case TSwitch:
                fprintf(output, "TSwitch\n");
                break;
            case TCase:
                fprintf(output, "TCase\n");
                break;
            case TDefault:
                fprintf(output, "TDefault\n");
                break;
            case TBreak:
                fprintf(output, "TBreak\n");
                break;
            case TContinue:
                fprintf(output, "TContinue\n");
                break;
            case TReturnvoid:
                fprintf(output, "TReturn\n");
                break;
            case TReturnval:
                fprintf(output, "TReturnval %i\n", tree[i++]);
                break;
            case TGoto:
                fprintf(output, "TGoto %i\n", tree[i++]);
                break;
            case TIdent:
                fprintf(output, "TIdent %i\n", tree[i++]);
                break;
            case TIdenttoval:
                fprintf(output, "TIdenttoval %i\n", tree[i++]);
                break;
            case TIdenttovalc:
                fprintf(output, "TIdenttovalc %i\n", tree[i++]);
                break;
            case TIdenttovalf:
                fprintf(output, "TIdenttovalf %i\n", tree[i++]);
                break;
            case TSelect:
                fprintf(output, "TSelect %i %i\n", tree[i], tree[i+1]);
                i += 2;
                break;

            case TFunidtoval:
                fprintf(output, "TFunidtoval %i\n", tree[i++]);
                break;
            case TIdenttoaddr:
                fprintf(output, "TIdenttoaddr %i\n", tree[i++]);
                break;
            case TAddrtoval:
                fprintf(output, "TAddrtoval\n");
                break;
            case TAddrtovalc:
                fprintf(output, "TAddrtovalc\n");
                break;
            case TAddrtovalf:
                fprintf(output, "TAddrtovalf\n");
                break;
            case TExprend:
                fprintf(output, "TExprend\n");
                break;
            case TConst:
                fprintf(output, "TConst %i\n", tree[i++]);
                break;
            case TConstc:
                fprintf(output, "TConstc %c\n", tree[i++]);
                break;
            case TConstf:
                memcpy(&numf, &tree[i++], sizeof(float));
                fprintf(output, "TConstf %f\n", numf);
                break;
            case TConstd:
                memcpy(&numdouble, &tree[i], sizeof(double));
                i += 2;
                fprintf(output, "TConstd %f\n", numdouble);
                break;
            case TSliceident:
                fprintf(output, "TSliceident displ= %i type= %i\n", tree[i], tree[i+1]);
                i += 2;
                break;
            case TSlice:
                fprintf(output, "TSlice elem_type= %i\n", tree[i++]);
                break;
			case TDYNSelect:
				fprintf(output, "TDYNSelect displ= %i\n", tree[i++]);
				break;
            case NOP:
                fprintf(output, "NOP\n");
                break;
            case ADLOGAND:
                fprintf(output, "ADLOGAND addr= %i\n", tree[i++]);
                break;
            case ADLOGOR:
                fprintf(output, "ADLOGOR addr= %i\n", tree[i++]);
                break;
            case COPY00:
                fprintf(output, "COPY00 %i ", tree[i++]);     // displleft
                fprintf(output, "%i ", tree[i++]);            // displright
                fprintf(output, "(%i)\n", tree[i++]);         // type
                break;
            case COPY01:
                fprintf(output, "COPY01 %i ", tree[i++]);     // displleft
                fprintf(output, "(%i)\n", tree[i++]);         // type
                break;
            case COPY10:
                fprintf(output, "COPY10 %i ", tree[i++]);     // displright
                fprintf(output, "(%i)\n", tree[i++]);         // type
                break;
            case COPY11:
                fprintf(output, "COPY11 %i\n", tree[i++]);     // type
                break;
            case COPYST:
                fprintf(output, "COPYST %i ", tree[i++]);       // displ
                fprintf(output, "(%i)", tree[i++]);             // length
                fprintf(output, "(%i)\n", tree[i++]);           // length1
                break;

            case TCall1:
                fprintf(output, "TCall1 %i\n", tree[i++]);
                break;
            case TCall2:
                fprintf(output, "TCall2 %i\n", tree[i++]);
                break;
            case TLabel:
                fprintf(output, "TLabel %i\n", tree[i++]);
                break;
            case TStructbeg:
                fprintf(output, "TStructbeg %i\n", tree[i++]);
                break;
            case TStructend:
                fprintf(output, "TStructend %i\n", tree[i++]);
                break;
            case TPrint:
                fprintf(output, "TPrint %i\n", tree[i++]);
                break;
            case TPrintid:
                fprintf(output, "TPrintid %i\n", tree[i++]);
                break;
            case TPrintf:
                fprintf(output, "TPrintf %i\n", tree[i++]);
                break;
			case TGet:
				fprintf(output, "TGet %i\n", tree[i++]);
				break;
            case TGetid:
                fprintf(output, "TGetid %i\n", tree[i++]);
                break;
            case SETMOTORC:
                fprintf(output, "Setmotor\n");
                break;
            case CREATEC:
                fprintf(output, "TCREATE\n");
                break;
            case CREATEDIRECTC:
                fprintf(output, "TCREATEDIRECT\n");
                break;
            case EXITC:
                fprintf(output, "TEXIT\n");
                break;
            case EXITDIRECTC:
                fprintf(output, "TEXITDIRECT\n");
                break;
            case MSGSENDC:
                fprintf(output, "TMSGSEND\n");
                break;
            case MSGRECEIVEC:
                fprintf(output, "TMSGRECEIVE\n");
                break;
            case JOINC:
                fprintf(output, "TJOIN\n");
                break;
            case SLEEPC:
                fprintf(output, "TSLEEP\n");
                break;
            case SEMCREATEC:
                fprintf(output, "TSEMCREATE\n");
                break;
            case SEMWAITC:
                fprintf(output, "TSEMWAIT\n");
                break;
            case SEMPOSTC:
                fprintf(output, "TSEMPOST\n");
                break;
            case INITC:
                fprintf(output, "INITC\n");
                break;
            case DESTROYC:
                fprintf(output, "DESTROYC\n");
                break;
            case GETNUMC:
                fprintf(output, "GETNUMC\n");
                break;



            default:
                fprintf(output, "TOper %i\n", tree[i-1]);
        }
    }
}
