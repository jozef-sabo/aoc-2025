#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>

#define ALLOC_BATCH 32

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

    *data = nums_lines;
    *lines = idx;

    final:
        if ( line != NULL )
            free( line );

    return rv;
}

uint64_t part_one( char **lines, int lines_n )
{
    uint64_t splits = 0;

    for ( int line_idx = 1; line_idx < lines_n; ++line_idx )
        for ( int column = 0; lines[ line_idx ][ column ] != '\0'; ++column )
        {
            if ( lines[ line_idx - 1 ][ column ] != 'S' && lines[ line_idx - 1 ][ column ] != '|' )
                continue;

            if ( lines[ line_idx ][ column ] == '.' )
                lines[ line_idx ][ column ] = '|';

            if ( lines[ line_idx ][ column ] == '^' )
            {
                ++splits;
                if ( column != 0 )
                    lines[ line_idx ][ column - 1 ] = '|';

                if ( lines[ line_idx ][ column + 1 ] != '\n' && lines[ line_idx ][ column + 1 ] != '\0' )
                    lines[ line_idx ][ column + 1 ] = '|';

            }
        }

    return splits;
}

int alloc_map( void ***map, size_t width, size_t length, size_t elem )
{
    void **map_l = malloc( sizeof( void* ) * length );
    if ( map_l == NULL )
        return -1;

    void *field = malloc( width * length * elem );
    if ( field == NULL )
    {
        free( map_l );
        return -1;
    }

    memset( field, 0, width * length * elem );

    for ( int i = 0; i < length; ++i )
        map_l[ i ] = ( uint8_t* ) field + ( i * width * elem );

    *map = map_l;

    return 0;
}

void map_free( void **map )
{
    free( map[ 0 ] );
    free( map );
}

uint64_t part_two( char **lines, int lines_n )
{
    size_t line_len = strlen( lines[ 0 ] );
    if ( lines[ 0 ][ line_len - 1 ] == '\n' )
        --line_len;

    uint64_t **map = NULL;
    alloc_map( ( void *** ) &map, line_len, lines_n, sizeof( uint64_t ) );

    uint64_t splits = 0;

    int column_s = 0;
    for ( ; lines[ 0 ][ column_s ] != 'S'; ++column_s )
        ;
    map[ 0 ][ column_s ] = 1;

    for ( int line_idx = 1; line_idx < lines_n; ++line_idx )
        for ( int column = 0; lines[ line_idx ][ column ] != '\0'; ++column )
        {
            if ( lines[ line_idx - 1 ][ column ] != 'S' && lines[ line_idx - 1 ][ column ] != '|' )
                continue;

            if ( lines[ line_idx ][ column ] == '.' || lines[ line_idx ][ column ] == '|' )
            {
                lines[ line_idx ][ column ] = '|';
                map[ line_idx ][ column ] += map[ line_idx - 1 ][ column ];
            }

            if ( lines[ line_idx ][ column ] == '^' )
            {
                if ( column != 0 )
                {
                    lines[ line_idx ][ column - 1 ] = '|';
                    map[ line_idx ][ column - 1 ] += map[ line_idx - 1 ][ column ];
                }

                if ( lines[ line_idx ][ column + 1 ] != '\n' && lines[ line_idx ][ column + 1 ] != '\0' )
                {
                    lines[ line_idx ][ column + 1 ] = '|';
                    map[ line_idx ][ column + 1 ] += map[ line_idx - 1 ][ column ];
                }

            }
        }

    for ( size_t idx = 0; idx < line_len; ++idx )
        splits += map[ lines_n - 1 ][ idx ];

    map_free( ( void** ) map );

    return splits;
}

ssize_t process_data( FILE *in_file, uint64_t ( *part_fn )( char**, int ) )
{
    ssize_t rv = -1;

    int lines_n = 0;
    char **lines = NULL;

    if ( data_load( &lines, &lines_n, in_file ) < 0 )
        goto finalize;

    rv =  ( ssize_t ) part_fn( lines, lines_n );

finalize:
    if ( lines != NULL )
    {
        for ( int i = 0; i < lines_n; ++i )
            if ( lines[ i ] != NULL )
                free( lines[ i ] );
        free( lines );
    }

    return rv;
}

int main( int argc, char *argv[ ] )
{
    uint64_t ( *part_fn )( char**, int ) = part_two;
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
