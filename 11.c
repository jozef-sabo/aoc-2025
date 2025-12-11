#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>

#define ALLOC_BATCH 32

#define POS_TO_IDX_MEMO( x, y, memo ) ( ( ( y ) * ( memo->dimension_size ) ) + ( x ) )

typedef struct vertex_t {
    char *name;
    size_t neighbours_n;
    struct vertex_t **neighbours;
    size_t memo;
    size_t memo2;
} vertex_t;

typedef struct vertices_t {
    vertex_t *vertices;
    size_t vertices_n;
} vertices_t;

typedef struct memo_2d_t {
    uint64_t *memo;
    size_t dimension_size;
    size_t total_size;
} memo_2d_t;

int memo_reinit( memo_2d_t *memo, size_t dimension_size )
{
    if ( memo->memo == NULL )
    {
        size_t total = dimension_size * dimension_size;
        uint64_t *memo_p = malloc( total * sizeof( uint64_t ) );
        if ( memo_p == NULL )
            return -1;

        memo->memo = memo_p;
        memo->dimension_size = dimension_size;
        memo->total_size = total;
    }

    for ( size_t i = 0; i < memo->total_size; ++i )
        memo->memo[ i ] = UINT64_MAX;

    return 0;
}

void memo_destroy( memo_2d_t *memo )
{
    if ( memo->memo != NULL )
        free( memo->memo );
    memo->memo = NULL;
    memo->dimension_size = 0;
    memo->total_size = 0;
}

int vertex_add( vertices_t* vertices )
{
    vertex_t *tmp;

    if ( vertices->vertices_n % ALLOC_BATCH == 0 )
    {
        vertex_t *old_base = vertices->vertices;

        size_t new_n = vertices->vertices_n + ALLOC_BATCH;
        tmp = realloc( vertices->vertices, new_n * sizeof( vertex_t ) );
        if ( tmp == NULL )
            return -1;
        memset( tmp + vertices->vertices_n, 0, ALLOC_BATCH * sizeof( vertex_t ) );

        intptr_t ptr_dif = ( char* ) tmp - ( char* ) old_base;

        for ( size_t vertex_idx = 0; vertex_idx < vertices->vertices_n; ++vertex_idx )
            for ( size_t neighbour_idx = 0; neighbour_idx < tmp[ vertex_idx ].neighbours_n; ++neighbour_idx )
                ( tmp[ vertex_idx ].neighbours[ neighbour_idx ] = ( vertex_t* ) ( ( char* )tmp[ vertex_idx ].neighbours[ neighbour_idx ] + ptr_dif ));

        vertices->vertices = tmp;
    }

    ++vertices->vertices_n;

    return 0;
}

vertex_t *find_vertex( char *vertex_name, vertices_t *vertices )
{
    for ( size_t i = 0; i < vertices->vertices_n; ++i )
        if ( strcmp( vertex_name, vertices->vertices[ i ].name ) == 0 )
            return vertices->vertices + i;

    return NULL;
}

int find_or_add_vertex( char *vertex_name, vertices_t *vertices, vertex_t **desired_vertex )
{
    vertex_t *actual_vertex = find_vertex( vertex_name, vertices );
    if ( actual_vertex == NULL )
    {
        if ( vertex_add( vertices ) == -1 )
            return -1;

        actual_vertex = vertices->vertices + vertices->vertices_n - 1;
        actual_vertex->name = strdup( vertex_name );

        if ( actual_vertex->name == NULL )
            return -1;
    }

    if ( desired_vertex != NULL )
        *desired_vertex = actual_vertex;

    return 0;
}

int process_line( char *line, vertices_t *vertices )
{
    char *state_r = NULL;
    strtok_r( line, "\n", &state_r );

    state_r = NULL;
    char *vertex_name = strtok_r( line, ":", &state_r );

    if ( vertex_name == NULL )
    {
        warnx( "incorrect input format, missing vertex name" );
        return -1;
    }

    char *neighbours = strtok_r( NULL, ":", &state_r );
    if ( neighbours == NULL )
    {
        warnx( "incorrect input format, missing neighbours" );
        return -1;
    }

    vertex_t *actual_vertex;
    if ( find_or_add_vertex( vertex_name, vertices, &actual_vertex ) == -1 )
        return -1;

    if ( actual_vertex->neighbours != NULL )
    {
        warnx( "incorrect input file - vertex occurred more than once" );
        return -1;
    }

    actual_vertex->neighbours = malloc( ALLOC_BATCH * sizeof( vertex_t * ) );
    if ( actual_vertex->neighbours == NULL )
    {
        warnx( "unable to alloc space for neighbours" );
        return -1;
    }

    state_r = NULL;
    char *neighbour_name = strtok_r( neighbours, " ", &state_r );
    do
    {
        if ( find_or_add_vertex( neighbour_name, vertices, NULL ) < 0 ) // may realloc
            return -1;

        find_or_add_vertex( vertex_name, vertices, &actual_vertex ); // would not realloc
        find_or_add_vertex( neighbour_name, vertices, &actual_vertex->neighbours[ actual_vertex->neighbours_n++ ] );

    } while ( ( neighbour_name = strtok_r( NULL, " ", &state_r ) ) != NULL );

    return 0;
}


int data_load( vertices_t *vertices, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;

    char *line = NULL;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( process_line( line, vertices ) < 0 )
            goto final;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        goto final;
    }

    rv = 0;

    final:
        if ( line != NULL )
            free( line );

    return rv;
}

uint64_t dive_rec( vertex_t *start, vertex_t *end, memo_2d_t *memo, vertices_t *vertices, int act_depth, int max_depth )
{
    if ( act_depth == max_depth )
    {
        warnx( "max depth" );
        return 0;
    }

    size_t start_idx = start - vertices->vertices;
    size_t end_idx = end - vertices->vertices;
    size_t idx = POS_TO_IDX_MEMO( start_idx, end_idx, memo );

    uint64_t cache = memo->memo[ idx ];
    if ( cache != UINT64_MAX )
        return cache;

    if ( start == end )
    {
        memo->memo[ idx ] = 1;
        return 1;
    }

    uint64_t paths = 0;
    for ( size_t neighbour_idx = 0; neighbour_idx < start->neighbours_n; ++neighbour_idx )
        paths += dive_rec( start->neighbours[ neighbour_idx ], end, memo, vertices, act_depth + 1, max_depth );

    memo->memo[ idx ] = paths;
    return paths;
}

uint64_t paths_between( char *point_a, char *point_b, vertices_t *vertices, memo_2d_t *memo )
{
    vertex_t *start = find_vertex( point_a, vertices );
    vertex_t *end = find_vertex( point_b, vertices );
    if ( start == NULL )
    {
        warnx( "in vertex (%s) not found", point_a );
        return 0;
    }

    if ( end == NULL )
    {
        warnx( "out vertex (%s) not found", point_b );
        return 0;
    }

    if ( memo_reinit( memo, vertices->vertices_n ) == -1 )
        return 0;

    uint64_t rv = dive_rec( start, end, memo, vertices, 0, vertices->vertices_n );

    return rv;
}

uint64_t part_one( vertices_t *vertices )
{
    memo_2d_t memo = { 0 };
    uint64_t rv = paths_between( "you", "out", vertices, &memo );
    memo_destroy( &memo );
    return rv;
}

uint64_t part_two( vertices_t *vertices )
{
    memo_2d_t memo = { 0 };
    uint64_t path_1 = paths_between( "svr", "fft", vertices, &memo )
                    * paths_between( "fft", "dac", vertices, &memo )
                    * paths_between( "dac", "out", vertices, &memo );

    uint64_t path_2 = paths_between( "svr", "dac", vertices, &memo )
                    * paths_between( "dac", "fft", vertices, &memo )
                    * paths_between( "fft", "out", vertices, &memo );

    memo_destroy( &memo );
    return path_1 + path_2;
}

ssize_t process_data( FILE *in_file, uint64_t ( *part_fn )( vertices_t * ) )
{
    ssize_t rv = -1;
    vertices_t vertices = { .vertices = NULL, .vertices_n = 0 };

    if ( data_load( &vertices, in_file ) < 0 )
        goto finalize;

    rv = part_fn( &vertices );

finalize:
    if ( vertices.vertices != NULL )
    {
        for ( size_t i = 0; i < vertices.vertices_n; ++i )
        {
            if ( vertices.vertices[ i ].name != NULL )
                free( vertices.vertices[ i ].name );
            if ( vertices.vertices[ i ].neighbours != NULL )
                free( vertices.vertices[ i ].neighbours );
        }
        free( vertices.vertices );
    }

    return rv;
}

int main( int argc, char *argv[ ] )
{
    uint64_t ( *part_fn )( vertices_t* ) = part_two;
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
