int sizes[4] = {3, 2, 2, 3};
int  a[3] = {1, 2, 3};
int  b[2][3] =  {a, {4, 5, 6}};
int  c[2][2][3] = {{{7, 8, 9}, {10, 11, 12}}, b};
int  d[3][2][2][] = {c, c, c};
int  numOfLowestDimensions[3] = {12, 6, 3};

int find_d(int val, int first_start_index)
{
	for (int i = first_start_index; i < sizes[0]; i++)
	{
		for (int j = 0; j < sizes[1]; j++)
		{
			for (int k = 0; k < sizes[2]; k++)
			{
				for (int l = 0; l < sizes[3]; l++)
				{
					if (d[i][j][k][l] == val)
					{
						return i * numOfLowestDimensions[0] + j * numOfLowestDimensions[1] + k * numOfLowestDimensions[2] + l;
					}
				}
			}
		}
	}

	return 0;
}

void main()
{
	assert(find_d(5, 0) == 10, "considering only first d subarray 5 is on index 10 in d array");
	assert(find_d(5, 1) == 22, "considering only second d subarray 5 is on index 22 in d array");
	assert(find_d(5, 2) == 34, "considering only third d subarray  5 is on index 34 in d array");
	assert(find_d(11, 1) == 16, "considering only second d subarray  11 is on index 16 in d array");
	assert(find_d(7, 0) == 0, "considering only first d subarray  7 is on index 0 in d array");
	assert(find_d(6, 2) == 35, "considering only third d subarray  6 is on index 35 in d array");
}