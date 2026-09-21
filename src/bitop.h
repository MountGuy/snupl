#ifndef BITOP_H
#define BITOP_H 1

#define BYTE_SIZE 8
#define SZLIB (sizeof(unsigned long long int) * BYTE_SIZE)
#define OFFSET(s, c, e, sn, cn) ((e) + (sn) * ((c) + (cn) * (s)))
#define READ_OFFSET(p, o) ((p)[(o) / SZLIB] & ((ulli) 1 << ((o) % SZLIB)))
#define WRITE_OFFSET(p, o) ((p)[(o) / SZLIB] |= ((ulli) 1 << ((o) % SZLIB)))

#endif
