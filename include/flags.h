#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#define FLAG_1HY_HELP 'h'
#define FLAG_1HY_LIST 'l'

#define FLAG_2HY_HELP "help"
#define FLAG_2HY_LIST "list"
#define FLAG_2HY_COPYING "copying"

typedef enum {
        FT_LIST = 1 << 0,
} flag_type;

void usage(void);
void copying(void);

#endif // FLAGS_H_INCLUDED
