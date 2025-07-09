#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#define FLAG_1HY_HELP 'h'
#define FLAG_1HY_LIST 'l'

#define FLAG_2HY_HELP "help"
#define FLAG_2HY_LIST "list"

typedef enum {
        FT_LIST = 1 << 0,
} flag_type;


#endif // FLAGS_H_INCLUDED
