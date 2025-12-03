#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>

uint64_t slice_highest( const char* num_buf, const int count )
{
    // max number of digits in 64-bit number
    if ( count > 19 || count < 0 )
        return 0;

    // VLA here is OK as the size value is sanitized
    char highest_num_buf[ count + 1 ];
    highest_num_buf[ count ] = '\0';

    int highest_index = 0;
    char highest_num = '0' - 1;

    for ( int char_idx = 0; char_idx < count; ++char_idx )
    {
        for ( int idx = highest_index; num_buf[ idx + count - char_idx - 1 ] != '\n'; ++idx )
            if ( num_buf[ idx ] > highest_num )
            {
                highest_num = num_buf[ idx ];
                highest_index = idx;
            }

        highest_num_buf[ char_idx ] = highest_num;
        highest_num = '0' - 1;
        ++highest_index;
    }

    errno = 0;
    uint64_t result = strtoull( highest_num_buf, NULL, 10 );
    if ( errno == EINVAL )
    {
        warn( "parsing sliced number" );
        return 0;
    }

    return result;
}

uint64_t part_one( const char* bank )
{
    return slice_highest( bank, 2 );
}

uint64_t part_two( const char* bank )
{
    return slice_highest( bank, 12 );
}

uint64_t process_bank( const char *bank, uint64_t ( *part_fn ) ( const char* ) )
{
    uint64_t rv = 0;

    if ( bank == NULL )
        return 0;

    rv = part_fn( bank );

    return rv;
}

int main( int argc, char *argv[ ] )
{
    uint64_t ( *part_fn ) ( const char* ) = part_two;

    int rv = 0;
    FILE *in_file = stdin;
    char *line = NULL;
    size_t n = 0, sum = 0;
    ssize_t bytes_r;

    if ( argc == 2 )
    {
        in_file = fopen( argv[ 1 ], "r" );
        if ( in_file == NULL )
            err( 1, "opening file" );
    }

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        sum += process_bank( line, part_fn );
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        rv = 1;
    }

    if ( line != NULL )
        free( line );

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
