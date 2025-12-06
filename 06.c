#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>

#define ALLOC_BATCH 16

int process_numlist( uint64_t **items, int *items_n, char *line )
{
    char *end_p = line;
    int items_l = 0;

    if ( items_n != NULL )
        items_l = *items_n;


    while ( *end_p != '\n' && strlen( end_p ) != 0 )
    {
        if ( items_n != NULL && items_l % ALLOC_BATCH == 0 )
        {
            int new_n = items_l + ALLOC_BATCH;
            uint64_t *tmp = realloc( *items, new_n * sizeof( uint64_t ) );
            if ( tmp == NULL )
                return -1;
            *items = tmp;
        }

        errno = 0;
        uint64_t num = strtoull( end_p, &end_p, 10 );
        if ( errno == EINVAL )
        {
            warn( "reading a number" );
            return -1;
        }
        while ( *end_p == ' ' )
            ++end_p;

        ( *items )[ items_l ] = num;
        ++items_l;
    }

    if ( items_n != NULL )
        *items_n = items_l;

    return 0;
}

int data_load_one( uint64_t **nums, int *lines, int *line_size, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;
    char *line = NULL;
    int idx = 0;

    uint64_t *nums_line = NULL;
    int nums_line_n = 0;

    bytes_r = getdelim( &line, &n, '\n', in_file );
    if ( ( bytes_r < 0 && errno != 0 ) || line == NULL )
    {
        warn( "reading from a file" );
        goto final;
    }

    if ( process_numlist( &nums_line, &nums_line_n, line ) < 0 )
        goto final;

    uint64_t *tmp = realloc( nums_line, ALLOC_BATCH * sizeof( uint64_t ) * nums_line_n );
    if ( tmp == NULL )
        goto final;
    nums_line = tmp;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        ++idx;
        if ( idx % ALLOC_BATCH == 0 )
        {
            int new_n = idx + ALLOC_BATCH;
            tmp = realloc( nums_line, new_n * sizeof( uint64_t ) * nums_line_n );
            if ( tmp == NULL )
                goto final;
            nums_line = tmp;
        }
        if ( *line == '*' || *line == '+' )
            break;

        uint64_t *actual_nums_line = &nums_line[ idx * nums_line_n ];
        if ( process_numlist( &actual_nums_line, NULL, line ) < 0 )
            goto final;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        goto final;
    }

    char *line_p = line;
    int op_idx = 0;
    for ( ; *line_p != '\0'; ++line_p )
    {
        if ( *line_p == '*' )
            nums_line[ ( idx * nums_line_n ) + op_idx++ ] = 1;

        if ( *line_p == '+' )
            nums_line[ ( idx * nums_line_n ) + op_idx++ ] = 2;
    }

    rv = 0;
final:
    *nums = nums_line;
    *line_size = nums_line_n;
    *lines = idx;

    if ( line != NULL )
        free( line );

    return rv;
}

uint64_t add_op( const uint64_t a, const uint64_t b )
{
    return a + b;
}

uint64_t mult_op( const uint64_t a, const uint64_t b )
{
    return a * b;
}

ssize_t part_one( FILE *in_file )
{
    ssize_t rv = -1;

    int lines_n = 0, lines_size = 0;
    uint64_t *lines = NULL;

    if ( data_load_one( &lines, &lines_n, &lines_size, in_file ) < 0 )
        goto finalize;

    uint64_t ( *operator )( const uint64_t, const uint64_t );
    uint64_t num, grand_total = 0;

    for ( int column = 0; column < lines_size; ++column )
    {
        operator = lines[ lines_size * lines_n + column ] == 1 ? mult_op : add_op;
        num = operator == mult_op ? 1 : 0;

        for ( int line_idx = 0; line_idx < lines_n; ++line_idx )
            num = operator( num, lines[ lines_size * line_idx + column ] );

        grand_total += num;
    }

    rv = ( ssize_t ) grand_total;

finalize:

    if ( lines != NULL )
        free( lines );

    return rv;
}

int data_load_two( char ***data, int *lines, FILE *in_file )
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

ssize_t part_two( FILE *in_file )
{
    ssize_t rv = -1;

    int lines_n = 0;
    char **lines = NULL;

    if ( data_load_two( &lines, &lines_n, in_file ) < 0 )
        goto finalize2;

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
            goto finalize2;
        }

        num_part = operator( num_part, num );
    }

    rv = ( ssize_t ) grand_total;

finalize2:
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
    ssize_t ( *part_fn )( FILE* ) = part_two;
    int rv = 0;
    FILE *in_file = stdin;
    size_t sum = 0;

    if ( argc == 2 )
    {
        in_file = fopen( argv[ 1 ], "r" );
        if ( in_file == NULL )
            err( 1, "opening file" );
    }

    if ( ( sum = part_fn( in_file ) ) < 0 )
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
