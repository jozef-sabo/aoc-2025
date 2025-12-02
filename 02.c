#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

bool is_special_subparts( uint64_t num, int num_size, int subnum_size )
{
    if ( num_size % subnum_size != 0 )
        return false;

    int parts = num_size / subnum_size;

    uint64_t mask = 1;
    for ( int i = 0; i < subnum_size; ++i )
        mask *= 10;

    uint64_t base = num % mask;

    for ( int part_idx = 1; part_idx < parts; ++part_idx )
    {
        num /= mask;
        if ( num % mask != base )
            return false;
    }

    return true;
}

bool part_one( uint64_t num, int num_size )
{
    if ( num_size % 2 != 0 )
        return false;

   return is_special_subparts( num, num_size, num_size / 2 );
}

bool part_two( uint64_t num, int num_size )
{
    int half_size = num_size / 2;

    for ( int subnum_size = 1; subnum_size <= half_size; ++subnum_size )
        if ( is_special_subparts( num, num_size, subnum_size ) )
            return true;

    return false;
}

int whole_log( uint64_t num, uint64_t base )
{
    int log = 0;

    for (  ; num != 0; num /= base )
        ++log;

    return log;
}

uint64_t process_range( char *range, bool ( *part_fn ) ( uint64_t, int ) )
{
    uint64_t rv = 0;

    if ( range == NULL )
        return 0;

    char *end_p = NULL;

    errno = 0;

    uint64_t from = strtoull( range, &end_p, 10 );
    if ( errno == EINVAL )
        return 0;

    uint64_t to = strtoull( end_p + 1, &end_p, 10 );
    if ( errno == EINVAL )
        return 0;

    if ( from > to )
        return 0;

    for ( uint64_t num = from; num <= to; ++num )
    {
        int num_size = whole_log( num, 10 );

        if ( part_fn( num, num_size ) )
            rv += num;
    }
    return rv;
}

int main( int argc, char *argv[ ] )
{
    bool ( *part_fn ) ( uint64_t, int ) = part_two;

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
    while ( ( bytes_r = getdelim( &line, &n, ',', in_file ) ) > 0 )
    {
        sum += process_range( line, part_fn );
    }

    if ( line != NULL )
        free( line );

    if ( in_file != stdin )
        fclose( in_file );

    if ( bytes_r < 0 && errno != 0 )
        err( 1, "reading from a file" );

    printf( "%lu\n", sum );

    return 0;
}
