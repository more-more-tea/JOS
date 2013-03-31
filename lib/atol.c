// string to integer
#include <inc/atol.h>

// convert ascii decimal into long
long
atol(char *str)
{
	int base = 10;	// decimal by default
        int sign = 1;   // positive number by default

        int idx  = 0;   // pointer to ascii string
        if (str[idx] == '-') {
		sign = -1;
		idx++;
	}

	switch (str[idx]) {
	case '0':
		idx++;
		if (LOWER_CASE(str[idx]) == 'x') {
			base = 16;	// hex-digit
			idx++;
		} else
			base = 8;	// oct-digit
		break;
	default:
		base = 10;
		break;
	}

	long value = 0;
	int  digit = 0;
	char c = str[idx];
	while (c) {
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else {
			c = LOWER_CASE(c);
			if (c >= 'a' && c <= 'f')
				digit = c - 'a' + 10;
			else		// invalid digit
				digit = -1;
		}

		if ((digit < 0) || (base < digit))
			break;
		else {
			value = value * base + digit;
			c = str[++idx];
		}
	}

	return sign * value;
}
