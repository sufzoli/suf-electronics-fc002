/*
 * Frequency Counter project - 002B
 * Copyright by Zoltan Gomori
 * License:
 * Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0)
 * textconv.c
 */

void LongToBCD(unsigned int len, unsigned char str[], unsigned long src, unsigned char leadingzero)
{
	unsigned long tmp;

	unsigned char *i;
	for(i=str; i < str+len;i++)
	{
		if(src > 0)
		{
			tmp = src % 10;
			*i = (unsigned char)tmp;
			src = src / 10;
		}
		else
		{
			*i = leadingzero ? 0 : 0x10;
		}
	}
	return;
}
