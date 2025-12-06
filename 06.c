#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>

#define ALLOC_BATCH 32

uint64_t add_op( const uint64_t a, const uint64_t b )
{
    return a + b;
}

uint64_t mult_op( const uint64_t a, const uint64_t b )
{
    return a * b;
}

int data_load( char ***data, int *lines, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;

    char *line = NULL;
    char **tmp, **nums_lines = NULL;
    int idx = 0;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( idx % ALLOC_BATCH == 0 )
        {
            int new_n = idx + ALLOC_BATCH;
            tmp = realloc( nums_lines, new_n * sizeof( uint64_t ) );
            if ( tmp == NULL )
                goto final;
            nums_lines = tmp;
        }

        nums_lines[ idx ] = line;
        line = NULL;
        n = 0;

        ++idx;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        goto final;
    }

    rv = 0;
    --idx;

    *data = nums_lines;
    *lines = idx;

    final:
        if ( line != NULL )
            free( line );

    return rv;
}

ssize_t part_one( char **lines, int lines_n )
{
    int greatest_end_idx = 0;

    uint64_t ( *operator )( const uint64_t, const uint64_t );
    uint64_t num, grand_total = 0;
    char *end_p;

    while ( lines[ 0 ][ greatest_end_idx ] != '\0' )
    {
        operator = lines[ lines_n ][ greatest_end_idx ] == '*' ? mult_op : add_op;
        num = operator == mult_op ? 1 : 0;

        int dif_end = 0;
        for ( int line_idx = 0; line_idx < lines_n; ++line_idx )
        {
            errno = 0;
            uint64_t read_num = strtoull( &lines[ line_idx ][ greatest_end_idx ], &end_p, 10 );
            if ( errno == EINVAL )
            {
                warn( "reading a number" );
                return -1;
            }

            num = operator( num, read_num );

            if ( end_p - &lines[ line_idx ][ greatest_end_idx ] > dif_end )
                dif_end = ( int ) ( end_p - &lines[ line_idx ][ greatest_end_idx ] );
        }

        grand_total += num;
        greatest_end_idx += dif_end + 1;
    }

    return ( ssize_t ) grand_total;
}

ssize_t part_two( char **lines, int lines_n )
{
    char num_buf[ 20 ] = { 0 };
    uint64_t ( *operator )( const uint64_t, const uint64_t ) = add_op;
    uint64_t num_part = 0, grand_total = 0;

    for ( int column = 0; lines[ 0 ][ column ] != '\0'; ++column )
    {
        if ( lines[ lines_n ][ column ] != ' ' )
        {
            grand_total += num_part;

            operator = lines[ lines_n ][ column ] == '*' ? mult_op : add_op;
            num_part = operator == mult_op ? 1 : 0;
        }

        for ( int line_idx = 0; line_idx < lines_n ; ++line_idx )
            num_buf[ line_idx ] = lines[ line_idx ][ column ];

        char *num_buf_p = num_buf;
        while ( *num_buf_p == ' ' || *num_buf_p == '\n' )
            ++num_buf_p;

        if ( *num_buf_p == '\0' )
            continue;

        errno = 0;
        uint64_t num = strtoull( num_buf, NULL, 10 );
        if ( errno == EINVAL )
        {
            warn( "reading a number" );
            return -1;
        }

        num_part = operator( num_part, num );
    }

    return ( ssize_t ) grand_total;
}

ssize_t process_data( FILE *in_file, ssize_t ( *part_fn )( char**, int ) )
{
    ssize_t rv = -1;

    int lines_n = 0;
    char **lines = NULL;

    if ( data_load( &lines, &lines_n, in_file ) < 0 )
        goto finalize;

    rv = part_fn( lines, lines_n );

finalize:
    if ( lines != NULL )
    {
        for ( int i = 0; i < lines_n + 1; ++i )
            if ( lines[ i ] != NULL )
                free( lines[ i ] );
        free( lines );
    }

    return rv;
}

int main( int argc, char *argv[ ] )
{
    ssize_t ( *part_fn )( char**, int ) = part_one;
    int rv = 0;
    FILE *in_file = stdin;
    size_t sum = 0;

    if ( argc == 2 )
    {
        in_file = fopen( argv[ 1 ], "r" );
        if ( in_file == NULL )
            err( 1, "opening file" );
    }

    if ( ( sum = process_data( in_file, part_fn ) ) < 0 )
        rv = 1;

    if ( in_file != stdin )
        if ( fclose( in_file ) == EOF )
        {
            warn( "closing a file" );
            rv = 1;
        }

    if ( rv == 0 )
        printf( "%lu\n", sum );

    return rv;
}
