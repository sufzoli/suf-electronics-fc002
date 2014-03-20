/*
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
*/

const char segments[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
/*
void LongToText(unsigned int len, unsigned char str[], unsigned long src, unsigned char leadingzero)
{
	unsigned long tmp;

	unsigned char *i;
	for(i=str; i < str+len;i++)
	{
		if(src > 0)
		{
			tmp = src % 10;
			*i = segments[(unsigned char)tmp];
			src = src / 10;
		}
		else
		{
			*i = leadingzero || i == str ? segments[0] : 0;
		}
	}
	return;
}
*/
void LongToText(unsigned int len, unsigned char str[], unsigned long src, unsigned char leadingzero, unsigned char dp)
{
	unsigned long tmp;

	unsigned char *i;
	for(i=str; i < str+len;i++)
	{
		if(src > 0)
		{
			tmp = src % 10;
			*i = segments[(unsigned char)tmp] + ((i-str+1 == dp) ? 0x80:0);
			src = src / 10;
		}
		else
		{
			*i = leadingzero || i == str || (i-str) <= dp ? segments[0] : 0;
		}
	}
	return;
}

