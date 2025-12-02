#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

typedef struct state
{
    int64_t pointer;
} state;

// normalization function
uint64_t part_one( int64_t *pointer, int64_t steps )
{
    int64_t pointer_l = *pointer;
    pointer_l += steps;
    pointer_l %= 100;
    *pointer = ( int ) pointer_l;

    return ( pointer_l == 0 ) ? 1 : 0;
}

int64_t actual_cycle( int64_t num )
{
    if ( num < 0 )
        num -= 100;

    return num / 100;
}

uint64_t part_two( int64_t *pointer, int64_t steps )
{
    int64_t pointer_l = *pointer;

    // not to count zero twice when going left
    if ( steps < 0 )
        --pointer_l;

    int64_t pointer_l_next = pointer_l + steps;

    int64_t clicks = llabs( actual_cycle( pointer_l ) - actual_cycle( pointer_l_next ) );

    if ( steps < 0 )
        ++pointer_l_next;

    *pointer = pointer_l_next;

    return clicks;
}

uint64_t process_rotation( char *rotation, state *st, uint64_t ( *part_fn ) ( int64_t*, int64_t ) )
{
    int direction   = 0;

    int64_t new_pointer = st->pointer;

    if ( rotation[ 0 ] == 'L' )
        direction = -1;
    if ( rotation[ 0 ] == 'R' )
        direction = 1;

    if ( direction == 0 )
        return 0;

    errno = 0;
    int64_t steps = strtoll( rotation + 1, NULL, 10 );

    if ( errno == EINVAL
         || steps < 0 )
        return 0;

    steps = direction * steps;

    uint64_t additional_clicks = part_fn( &new_pointer, steps );

    st->pointer = new_pointer;


    return additional_clicks;
}

int main( int argc, char *argv[ ] )
{
    uint64_t ( *part_fn ) ( int64_t*, int64_t ) = part_two;

    FILE *in_file = stdin;
    char *line = NULL;
    size_t n = 0, count = 0;
    ssize_t bytes_r;
    state st = { .pointer = 50 };

    if ( argc == 2 )
    {
        in_file = fopen( argv[ 1 ], "r" );
        if ( in_file == NULL )
            err( 1, "opening file" );
    }

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        count += process_rotation( line, &st, part_fn );
    }

    if ( line != NULL )
        free( line );

    if ( in_file != stdin )
        fclose( in_file );

    if ( bytes_r < 0 && errno != 0 )
        err( 1, "reading from a file" );

    printf( "%lu\n", count );

    return 0;
}
