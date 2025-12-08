#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#define BOX_PAIRS 1000

#define ALLOC_BATCH 32

#define SEC_POW( n ) ( ( n ) * ( n ) )

// row-wise lower triangular
#define POS_TO_IDX( x, y ) ( ( ( ( ( y ) + 1 ) * ( y ) ) / 2 ) + ( x ) )
#define TRIANGULAR_LIN_SIZE_DIAG( rows ) ( POS_TO_IDX( 0, rows ) )

typedef struct point_3d_t
{
    uint64_t x;
    uint64_t y;
    uint64_t z;
} point_3d_t;

uint64_t euclidean_distance( point_3d_t *a, point_3d_t *b )
{
    return SEC_POW( a->x - b->x ) + SEC_POW( a->y - b->y ) + SEC_POW( a->z - b->z );
}

int strtoull_rv( char* from, char **end_p, uint64_t *num )
{
    errno = 0;
    uint64_t converted = strtoull( from, end_p, 10 );
    if ( errno == EINVAL )
    {
        warn( "parsing sliced number" );
        return -1;
    }
    *num = converted;
    return 0;
}


int data_load( point_3d_t **points, int *points_n, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;

    char *line = NULL;
    point_3d_t *tmp, *points_l = NULL;
    int idx = 0;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( idx % ALLOC_BATCH == 0 )
        {
            int new_n = idx + ALLOC_BATCH;
            tmp = realloc( points_l, new_n * sizeof( point_3d_t ) );
            if ( tmp == NULL )
                goto final;
            points_l = tmp;
        }

        char *end_p = line - 1;

        if ( strtoull_rv( end_p + 1, &end_p, &points_l[ idx ].x ) == -1 )
            goto final;
        if ( strtoull_rv( end_p + 1, &end_p, &points_l[ idx ].y ) == -1 )
            goto final;
        if ( strtoull_rv( end_p + 1, &end_p, &points_l[ idx ].z ) == -1 )
            goto final;

        ++idx;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        goto final;
    }

    rv = 0;

    *points = points_l;
    *points_n = idx;

final:
    if ( line != NULL )
        free( line );

    return rv;
}

uint32_t connect_groups( int x, int y, int points_n, uint32_t *points_group, uint32_t *max_group, uint32_t *distant_groups )
{
    if ( points_group[ x ] == points_group[ y ] && points_group[ y ] != 0 )
        return points_group[ x ];

    if ( points_group[ x ] != 0 && points_group[ y ] != 0 )
    {
        // merging by x
        uint32_t old_group = points_group[ y ];
        uint32_t new_group = points_group[ x ];
        for ( int idx = 0; idx < points_n; ++idx )
            if ( points_group[ idx ] == old_group )
                points_group[ idx ] = new_group;
        --(*distant_groups);
        return new_group;
    }
    if ( points_group[ x ] != 0 )
        return points_group[ y ] = points_group[ x ];

    if ( points_group[ y ] != 0 )
        return points_group[ x ] = points_group[ y ];

    ++(*distant_groups);
    points_group[ x ] = points_group[ y ] = ++(*max_group);

    return points_group[ x ];
}

void connect_points( int points_n, uint64_t *points_distances, uint32_t *points_group, uint32_t *max_group, uint32_t *distant_groups, int *last_x, int *last_y  )
{
    uint64_t min_dist = UINT64_MAX;
    int min_dist_x = -1;
    int min_dist_y = -1;
    for ( int y = 0; y < points_n; ++y )
        // can be made faster by argmax-ing and reverting the coordinates
        for ( int x = 0; x <= y; ++x )
        {
            uint64_t dist = points_distances[ POS_TO_IDX( x, y ) ];
            if ( dist != 0 && dist < min_dist )
            {
                min_dist = dist;
                min_dist_x = x;
                min_dist_y = y;
            }
        }

    connect_groups( min_dist_x, min_dist_y, points_n, points_group, max_group, distant_groups );
    points_distances[ POS_TO_IDX( min_dist_x, min_dist_y ) ] = UINT64_MAX;
    if ( last_x != NULL )
        *last_x = min_dist_x;
    if ( last_y != NULL )
        *last_y = min_dist_y;
}

bool contains_zeroes( int points_n, uint32_t *points_group )
{
    for ( int i = 0; i < points_n; ++i )
        if ( points_group[ i ] == 0 )
            return true;

    return false;
}

ssize_t part_two( int points_n, uint32_t *points_group, uint32_t *max_group, uint32_t *distant_groups, uint64_t *points_distances, point_3d_t *points )
{
    int last_x, last_y;
    while ( *distant_groups != 1 || contains_zeroes( points_n, points_group ) )
        connect_points( points_n, points_distances, points_group, max_group, distant_groups, &last_x, &last_y );

    return ( ssize_t ) ( points[ last_x ].x * points[ last_y ].x );
}

ssize_t part_one( int points_n, uint32_t *points_group, uint32_t *max_group, uint32_t *distant_groups, uint64_t *points_distances, point_3d_t *points )
{
    size_t *counts = NULL;

    counts = malloc( ( *max_group + 1 ) * sizeof( size_t ) );
    if ( counts == NULL )
        return -1;

    for ( int i = 0; i < points_n; ++i )
        ++counts[ points_group[ i ] ];

    uint64_t sum = 1;
    for ( int i = 0; i < 3; ++i )
    {
        uint32_t max_group_idx = UINT32_MAX;
        uint32_t max_group_value = 0;
        for ( uint32_t group_idx = 1; group_idx <= *max_group; ++group_idx )
            if ( counts[ group_idx ] > max_group_value )
            {
                max_group_value = counts[ group_idx ];
                max_group_idx = group_idx;
            }
        sum *= max_group_value;
        counts[ max_group_idx ] = 0;
    }

    free( counts );

    return ( ssize_t ) sum;
}

ssize_t process_data( FILE *in_file, ssize_t ( *part_fn )( int, uint32_t*, uint32_t*, uint32_t*, uint64_t*, point_3d_t* ) )
{
    ssize_t rv = -1;

    uint32_t *points_group = NULL;
    uint64_t *points_distances = NULL;

    int points_n = 0;
    point_3d_t *points = NULL;

    if ( data_load( &points, &points_n, in_file ) < 0 )
        goto finalize;

    points_group = calloc( points_n, sizeof( uint32_t ) );
    if ( points_group == NULL )
        goto finalize;

    points_distances = malloc( TRIANGULAR_LIN_SIZE_DIAG( points_n ) * sizeof( uint64_t ) );
    if ( points_distances == NULL )
        goto finalize;

    for ( int y = 0; y < points_n; ++y )
        for ( int x = 0; x <= y; ++x )
            points_distances[ POS_TO_IDX( x, y ) ] = euclidean_distance( points + x, points + y );

    uint32_t distant_groups = 0;
    uint32_t max_group = 0;
    for ( int i = 0; i < BOX_PAIRS; ++i )
        connect_points( points_n, points_distances, points_group, &max_group, &distant_groups, NULL, NULL );

    rv =  part_fn( points_n, points_group, &max_group, &distant_groups, points_distances, points );

finalize:
    if ( points != NULL )
        free( points );
    if ( points_group != NULL )
        free( points_group );
    if ( points_distances != NULL )
        free( points_distances );

    return rv;
}

int main( int argc, char *argv[ ] )
{
    ssize_t ( *part_fn )( int, uint32_t*, uint32_t*, uint32_t*, uint64_t*, point_3d_t* ) = part_two;

    int rv = 0;
    FILE *in_file = stdin;
    ssize_t sum = 0;

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
