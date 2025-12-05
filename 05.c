#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>

typedef struct range_t
{
    uint64_t from;
    uint64_t to;
} range_t;

#define ALLOC_BATCH 16

int process_range( void **ranges_v, int *ranges_n, char *line )
{
    range_t **ranges = ( range_t ** ) ranges_v;

    // we need to relloc
    if ( *ranges_n % ALLOC_BATCH == 0 )
    {
        int new_n = *ranges_n + ALLOC_BATCH;
        range_t *tmp = realloc( *ranges, ( new_n + 1 ) * sizeof( range_t ) );
        if ( tmp == NULL )
            return -1;
        for ( int i = *ranges_n; i < ( new_n + 1 ); ++i )
        {
            tmp[ i ].from = tmp[ i ].to = UINT64_MAX;
        }
        *ranges = tmp;
    }

    char *end_p = NULL;

    errno = 0;
    uint64_t from = strtoull( line, &end_p, 10 );
    if ( errno == EINVAL )
    {
        warn( "parsing sliced number" );
        return -1;
    }

    uint64_t to = strtoull( end_p + 1, NULL, 10 );
    if ( errno == EINVAL )
    {
        warn( "parsing sliced number" );
        return -1;
    }

    ( *ranges )[ *ranges_n ].from = from;
    ( *ranges )[ *ranges_n ].to = to;
    ++*ranges_n;

    return 0;
}

int process_numlist( void **items_v, int *items_n, char *line )
{
    uint64_t **items = ( uint64_t** ) items_v;

    // we need to relloc
    if ( *items_n % ALLOC_BATCH == 0 )
    {
        int new_n = *items_n + ALLOC_BATCH;
        uint64_t *tmp = realloc( *items, new_n * sizeof( uint64_t ) );
        if ( tmp == NULL )
            return -1;
        for ( int i = *items_n; i < new_n; ++i )
        {
            tmp[ i ] = UINT64_MAX;
        }
        *items = tmp;
    }

    char *end_p = NULL;

    errno = 0;
    uint64_t num = strtoull( line, &end_p, 10 );
    if ( errno == EINVAL )
    {
        warn( "parsing sliced number" );
        return -1;
    }
    ( *items )[ *items_n ] = num;
    ++*items_n;

    return 0;
}

int data_load( void **ranges, int *ranges_n, FILE *in_file, int ( *proces_fn ) ( void **, int*, char* ) )
{
    ssize_t bytes_r;
    size_t n = 0;
    char *line = NULL;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( line[ 0 ] == '\n' )
            break;
        if ( proces_fn( ranges, ranges_n, line ) < 0 )
            return -1;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        return -1;
    }

    if ( line != NULL )
        free( line );

    return 0;
}

int range_compare( const void *a, const void *b )
{
    range_t *range_a = ( range_t * ) a;
    range_t *range_b = ( range_t * ) b;

    if ( range_a->from == range_b->from )
    {
        if ( range_a->to < range_b->to )
            return -1;
        if ( range_a->to > range_b->to )
            return 1;
        return 0;
    }

    if ( range_a->from < range_b->from )
        return -1;
    if ( range_a->from > range_b->from )
        return 1;
    return 0;
}

int num_compare( const void *a, const void *b )
{
    uint64_t *num_a = ( uint64_t * ) a;
    uint64_t *num_b = ( uint64_t * ) b;

    if ( *num_a < *num_b )
        return -1;
    if ( *num_a > *num_b )
        return 1;
    return 0;
}

int relax_ranges( range_t *ranges, int ranges_n )
{
    int relaxed = 0;
    for ( int i = 1; i < ranges_n; ++i )
    {
        if ( ranges[ i - 1 ].to == ranges[ i ].to
             && ranges[ i - 1 ].from == ranges[ i ].from )
            continue;

        if ( ranges[ i - 1 ].to < ranges[ i ].from )
            continue;

        // overlaping
        ++relaxed;

        // two possibilities - first is containing the second fully or
        // first and the second are overlapping

        // ranges[ i - 1 ].to > ranges[ i ].from
        if ( ranges[ i - 1 ].to > ranges[ i ].to )
        {
            ranges[ i ].to = ranges[ i - 1 ].to;
            ranges[ i ].from =  ranges[ i - 1 ].from;
            continue;
        }

        // ranges[ i - 1 ].to <= ranges[ i ].to
        ranges[ i - 1 ].to = ranges[ i ].to;
        ranges[ i ].from =  ranges[ i - 1 ].from;
    }

    return relaxed;
}

uint64_t part_one( range_t *ranges, int ranges_n, uint64_t *items, int items_n )
{
    ( void ) ranges_n;

    range_t *range_p = ranges;
    int count = 0;
    for ( int idx = 0; idx < items_n; ++idx )
    {
        while ( range_p->to < items[ idx ] )
            ++range_p;

        if ( items[ idx ] < range_p->from )
            ++count;
    }

    return items_n - count;
}

uint64_t part_two( range_t *ranges, int ranges_n, uint64_t *items, int items_n )
{
    ( void ) items;
    ( void ) items_n;

    uint64_t fresh = 0;

    for ( int idx = 1; idx < ranges_n + 1; ++idx )
    {
        if ( ranges[ idx - 1 ].from == ranges[ idx ].from
            && ranges[ idx - 1 ].to == ranges[ idx ].to )
            continue;

        fresh += ( ranges[ idx - 1 ].to - ranges[ idx - 1 ].from ) + 1;
    }
    return fresh;
}

ssize_t process_data( FILE *in_file, size_t ( *part_fn ) ( range_t*, int, uint64_t*, int )  )
{
    ssize_t rv = -1;

    int ranges_n = 0;
    range_t *ranges = NULL;

    int items_n = 0;
    uint64_t *items = NULL;

    if ( data_load( ( void** ) &ranges, &ranges_n, in_file, process_range ) < 0 )
        goto finalize;;

    if ( data_load( ( void** ) &items, &items_n, in_file, process_numlist ) < 0 )
        goto finalize;

    qsort( ranges, ranges_n, sizeof( range_t ), range_compare );
    qsort( items, items_n, sizeof( uint64_t ), num_compare );

    while ( relax_ranges( ranges, ranges_n ) > 0 )
        ;

    rv = ( ssize_t ) part_fn( ranges, ranges_n, items, items_n );

finalize:
    if ( ranges != NULL )
        free( ranges );

    if ( items != NULL )
        free( items );

    return rv;
}

int main( int argc, char *argv[ ] )
{
    uint64_t ( *part_fn ) ( range_t*, int, uint64_t*, int ) = part_two;

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
