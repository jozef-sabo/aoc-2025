/*
 * This code is vastly ineffective for part 2 of Day 9 of the AOC 2025
 * It is because the solution is simulated on the 2D board and checked
 * against the simulation results. The better approach would be checking
 * if the new shape is contained fully inside using all the other points
 * or another thought was to use ray-tracing.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#define ALLOC_BATCH 32

#define SEC_POW( n ) ( ( n ) * ( n ) )

// row-wise lower triangular
#define TRIANGULAR_POS_TO_IDX( x, y ) ( ( ( ( ( y ) + 1 ) * ( y ) ) / 2 ) + ( x ) )
#define TRIANGULAR_LIN_SIZE_DIAG( rows ) ( TRIANGULAR_POS_TO_IDX( 0, rows ) )

#define POS_TO_IDX( x, y, width ) ( ( ( y ) * ( width ) ) + ( x ) )

typedef struct point_2d_t
{
    uint64_t x;
    uint64_t y;
} point_2d_t;

uint64_t euclidean_distance( point_2d_t *a, point_2d_t *b )
{
    return SEC_POW( a->x - b->x ) + SEC_POW( a->y - b->y );
}

uint64_t rectangle_size( point_2d_t *a, point_2d_t *b )
{
    uint64_t width = ( a->x > b->x ) ? ( a->x - b->x ) : ( b->x - a->x ) ;
    uint64_t height = ( a->y > b->y ) ? ( a->y - b->y ) : ( b->y - a->y ) ;
    return ( width + 1 ) * ( height + 1 );
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


int data_load( point_2d_t **points, int *points_n, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;

    char *line = NULL;
    point_2d_t *tmp, *points_l = NULL;
    int idx = 0;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( idx % ALLOC_BATCH == 0 )
        {
            int new_n = idx + ALLOC_BATCH;
            tmp = realloc( points_l, new_n * sizeof( point_2d_t ) );
            if ( tmp == NULL )
                goto final;
            points_l = tmp;
        }

        char *end_p = line - 1;

        if ( strtoull_rv( end_p + 1, &end_p, &points_l[ idx ].x ) == -1 )
            goto final;
        if ( strtoull_rv( end_p + 1, &end_p, &points_l[ idx ].y ) == -1 )
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

void find_borders ( int points_n, point_2d_t *points, uint64_t *min_x, uint64_t *min_y, uint64_t *max_x, uint64_t *max_y )
{
    uint64_t min_x_l = UINT64_MAX, min_y_l = UINT64_MAX,
             max_x_l = 0, max_y_l = 0;
    for ( int point_idx = 0; point_idx < points_n; ++point_idx )
    {
        if ( points[ point_idx ].x < min_x_l )
            min_x_l = points[ point_idx ].x;
        else if ( points[ point_idx ].x > max_x_l )
            max_x_l = points[ point_idx ].x;

        if ( points[ point_idx ].y < min_y_l )
            min_y_l = points[ point_idx ].y;
        else if ( points[ point_idx ].y > max_y_l )
            max_y_l = points[ point_idx ].y;
    }

    *max_x = max_x_l;
    *max_y = max_y_l;
    *min_x = min_x_l;
    *min_y = min_y_l;
}

void find_point_x( char *board, uint64_t height, uint64_t width, uint64_t *x, uint64_t *y )
{
    uint64_t x_l = *x;
    uint64_t y_l = *y;
    for ( x_l = *x + 1; x_l < width; ++x_l )
        if ( board[ POS_TO_IDX( x_l, y_l, width ) ] == '#' )
            break;

    if ( x_l == width )
    {
        x_l = *x;
        do
        {
            --x_l;
            if ( board[ POS_TO_IDX( x_l, y_l, width ) ] == '#' )
                break;
        } while ( x_l != 0 );
    }

    uint64_t x_old_l = *x;
    uint64_t x_min_l = ( x_old_l < x_l ) ? x_old_l : x_l;
    uint64_t x_max_l = ( x_old_l < x_l ) ? x_l : x_old_l;

    for ( uint64_t x_iter = x_min_l + 1; x_iter < x_max_l; ++x_iter )
        board[ POS_TO_IDX( x_iter, y_l, width ) ] = 'x';

    *x = x_l;
}

void find_point_y( char *board, uint64_t height, uint64_t width, uint64_t *x, uint64_t *y )
{
    uint64_t x_l = *x;
    uint64_t y_l = *y;
    for ( y_l = *y + 1; y_l < height; ++y_l )
        if ( board[ POS_TO_IDX( x_l, y_l, width ) ] == '#' )
            break;

    if ( y_l == height )
    {
        y_l = *y;
        do
        {
            --y_l;
            if ( board[ POS_TO_IDX( x_l, y_l, width ) ] == '#' )
                break;
        } while ( y_l != 0 );
    }

    uint64_t y_old_l = *y;
    uint64_t y_min_l = ( y_old_l < y_l ) ? y_old_l : y_l;
    uint64_t y_max_l = ( y_old_l < y_l ) ? y_l : y_old_l;

    for ( uint64_t y_iter = y_min_l + 1; y_iter < y_max_l; ++y_iter )
        board[ POS_TO_IDX( x_l, y_iter, width ) ] = 'x';

    *y = y_l;
}

void flood_fill( char *board, uint64_t height, uint64_t width, uint64_t x, uint64_t y )
{
    if ( board[ POS_TO_IDX( x, y, width ) ] != '.' )
        return;

    while ( board[ POS_TO_IDX( x, y, width ) ] == '.' )
        --x;

    ++x;
    uint64_t min_x = x;
    while ( board[ POS_TO_IDX( x, y, width ) ] == '.' )
    {
        board[ POS_TO_IDX( x, y, width ) ] = 'O';
        ++x;
    }
    --x;
    uint64_t max_x = x;

    for ( uint64_t idx_x = min_x; idx_x <= max_x; ++idx_x )
    {
        flood_fill( board, height, width, idx_x, y + 1 );
        flood_fill( board, height, width, idx_x, y - 1 );
    }
}

void outline_shape( char *board, uint64_t height, uint64_t width, uint64_t *x_in, uint64_t *y_in ) {
    uint64_t y = 0;
    uint64_t x = 0;

    for ( ; x < width; ++x )
       if ( board[ x ] != '.' )
           break;

    uint64_t x_start = x;
    uint64_t y_start = y;

    do
    {
        find_point_x( board, height, width, &x, &y );
        find_point_y( board, height, width, &x, &y );

    } while ( x != x_start && y != y_start );

    *x_in = x + 1;
    *y_in = y + 1;
}

bool is_rect_contained( char *board, point_2d_t *point_a, point_2d_t *point_b, uint64_t width )
{
    uint64_t min_x = ( point_a->x < point_b->x ) ? point_a->x : point_b->x;
    uint64_t max_x = ( point_a->x < point_b->x ) ? point_b->x : point_a->x;
    uint64_t min_y = ( point_a->y < point_b->y ) ? point_a->y : point_b->y;
    uint64_t max_y = ( point_a->y < point_b->y ) ? point_b->y : point_a->y;

    for ( uint64_t x_iter = min_x; x_iter <= max_x; ++x_iter )
    {
        if ( board[ POS_TO_IDX( x_iter, min_y, width ) ] == '.' )
            return false;
        if ( board[ POS_TO_IDX( x_iter, max_y, width ) ] == '.' )
            return false;
    }

    for ( uint64_t y_iter = min_y; y_iter <= max_y; ++y_iter )
    {
        if ( board[ POS_TO_IDX( min_y, y_iter, width ) ] == '.' )
            return false;
        if ( board[ POS_TO_IDX( max_x, y_iter, width ) ] == '.' )
            return false;
    }
    return true;
}

ssize_t part_two( int points_n, point_2d_t *points )
{
    uint64_t min_x, min_y, max_x, max_y;
    find_borders( points_n, points, &min_x, &min_y, &max_x, &max_y );
    uint64_t width = max_x - min_x + 1;
    uint64_t height = max_y - min_y + 1;

    for ( int point_idx = 0; point_idx < points_n; ++point_idx )
    {
        points[ point_idx ].x -= min_x;
        points[ point_idx ].y -= min_y;
    }

    char *board = calloc( width * height, sizeof( char ) );
    if ( board == NULL )
        return -1;
    memset( board, '.', width * height );

    for ( int point_idx = 0; point_idx < points_n; ++point_idx )
        board[ POS_TO_IDX( points[ point_idx ].x, points[ point_idx ].y, width ) ] = '#';

    uint64_t x_in, y_in;

    outline_shape( board, height, width, &x_in, &y_in );
    flood_fill( board, height, width, x_in, y_in );

    uint64_t max_size = 0;
    for ( int y = 0; y < points_n; ++y )
        for ( int x = 0; x <= y; ++x )
        {
            uint64_t size = rectangle_size( points + x, points + y );
            if ( size > max_size && is_rect_contained( board, points + x, points + y, width ) )
                max_size = size;
        }

    free( board );
    return ( ssize_t ) max_size;
}

ssize_t part_one( int points_n, point_2d_t *points )
{
    uint64_t *rectangle_sizes = rectangle_sizes = malloc( TRIANGULAR_LIN_SIZE_DIAG( points_n ) * sizeof( uint64_t ) );
    if ( rectangle_sizes == NULL )
        return -1;

    for ( int y = 0; y < points_n; ++y )
        for ( int x = 0; x <= y; ++x )
            rectangle_sizes[ TRIANGULAR_POS_TO_IDX( x, y ) ] = rectangle_size( points + x, points + y );

    uint64_t elem_num = TRIANGULAR_LIN_SIZE_DIAG( points_n );
    uint64_t max_size = 0;

    for ( uint64_t i = 0; i < elem_num; ++i )
        if ( max_size < rectangle_sizes[ i ] )
            max_size = rectangle_sizes[ i ];

    free( rectangle_sizes );
    return ( ssize_t ) max_size;
}

ssize_t process_data( FILE *in_file, ssize_t ( *part_fn )( int, point_2d_t* ) )
{
    ssize_t rv = -1;

    int points_n = 0;
    point_2d_t *points = NULL;

    if ( data_load( &points, &points_n, in_file ) < 0 )
        goto finalize;



    rv =  part_fn( points_n, points );

finalize:
    if ( points != NULL )
        free( points );

    return rv;
}

int main( int argc, char *argv[ ] )
{
    ssize_t ( *part_fn )( int, point_2d_t* ) = part_two;

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
