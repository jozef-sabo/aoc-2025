/*
 * This code is vastly ineffective for part 2 of Day 4 of the AOC 2025
 * It is because the focus for the part one was on the memory
 * effectivity (the program uses memory linear to the line length
 * instead of linear to the whole input). In the second part the reuse
 * of the whole memory was needed and so there is an extensive use of
 * filesystem operations and heap allocation (which extremely slows down
 * the execution). This could have been avoided by loading the whole input
 * to the memory, a some kind of map.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define LINES_CHECKED 3
#define NEIGHBOURS 4

#define CH_EMPTY '.'
#define CH_FULL '@'
#define CH_REMOVED 'x'

#define PART_TWO

typedef struct line_buf_t
{
    char *line;
    size_t allocd;
} line_buf_t;

typedef struct circular_lines_t
{
    FILE *file;
    int lines;
    int idx;
    line_buf_t *line_buf;
} circular_lines_t;

void lines_destroy( circular_lines_t *lines )
{
    for ( int idx = 0; idx < lines->lines; ++idx )
        if ( lines->line_buf[ idx ].line != NULL )
        {
            free( lines->line_buf[ idx ].line );
            lines->line_buf[ idx ].line = NULL;
            lines->line_buf[ idx ].allocd = 0;
        }
}

int lines_load_line( circular_lines_t *lines )
{
    int next_idx = ( lines->idx + 1 ) % lines->lines;

    errno = 0;
    ssize_t bytes_r = getdelim(
        &lines->line_buf[ next_idx ].line,
        &lines->line_buf[ next_idx ].allocd,
        '\n', lines->file );
    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        return -1;
    }

    if ( bytes_r < 0 )
        return lines->idx;

    char *line = lines->line_buf[ next_idx ].line;
    for ( ssize_t idx = 0; idx < bytes_r; ++idx )
    {
        if ( line[ idx ] == CH_REMOVED )
            line[ idx ] = CH_EMPTY;
    }

    lines->idx = next_idx;

    return next_idx;
}

int lines_init( circular_lines_t *lines, FILE *in_file )
{
    lines->file = in_file;

    for ( int idx = 1; idx < lines->lines; ++idx )
        if ( lines_load_line( lines ) < 0 )
            return -1;

    char *tmp = malloc( lines->line_buf[ 1 ].allocd );

    if ( tmp == NULL )
        return -1;

    lines->line_buf[ 0 ].line = tmp;
    lines->line_buf[ 0 ].allocd = lines->line_buf[ 1 ].allocd;
    // because \n ( -1 )
    size_t line_len = strlen( lines->line_buf[ 1 ].line );
    memset( tmp, '.', line_len - 1 );
    tmp[ line_len ] = '\0';

    return 0;
}

void lines_add_dummy_line( circular_lines_t *lines )
{
    int next_idx = ( lines->idx + 1 ) % lines->lines;

    size_t line_len = strlen( lines->line_buf[ 0 ].line );
    memset( lines->line_buf[ next_idx ].line, CH_EMPTY, line_len - 1 );
    lines->line_buf[ next_idx ].line[ line_len ] = '\0';

    lines->idx = next_idx;
}


char* actual_line( circular_lines_t *lines )
{
    return lines->line_buf[ ( lines->idx + ( lines->lines / 2 ) + 1 ) % lines->lines ].line;
}

char* actual_pos( circular_lines_t *lines, int x_pos )
{
    return &actual_line( lines )[ x_pos ];
}

int count_radius( circular_lines_t *lines, int x_pos, char character )
{
    int count = 0;
    int radius = lines->lines / 2;

    int l_bound = x_pos < radius ? 0 : x_pos - radius;
    int r_bound = x_pos;
    for ( ;    lines->line_buf[ 0 ].line[ r_bound ] != '\n'
            && lines->line_buf[ 0 ].line[ r_bound ] != '\0'
            && r_bound <= x_pos + radius         ; ++r_bound )
        ;

    for ( int y = 0; y < lines->lines; ++y )
        for ( int x = l_bound; x < r_bound; ++x )
            if ( lines->line_buf[ y ].line[ x ] == character || lines->line_buf[ y ].line[ x ] == CH_REMOVED )
                ++count;

    if ( *actual_pos( lines, x_pos ) == character )
        --count;

    return count;
}

uint64_t process_lines( circular_lines_t *lines )
{
    uint64_t count = 0;

    for ( int x_pos = 0; lines->line_buf[ 0 ].line[ x_pos ] != '\n' && lines->line_buf[ 0 ].line[ x_pos ] != '\0'; ++x_pos )
        if ( count_radius( lines, x_pos, CH_FULL ) < NEIGHBOURS && *actual_pos( lines, x_pos ) == CH_FULL )
            ++count;

    return count;
}

uint64_t process_lines_two( circular_lines_t *lines )
{
    uint64_t count = 0;

    for ( int x_pos = 0; lines->line_buf[ 0 ].line[ x_pos ] != '\n' && lines->line_buf[ 0 ].line[ x_pos ] != '\0'; ++x_pos )
    {
        char *act_pos = actual_pos( lines, x_pos );
        if ( count_radius( lines, x_pos, CH_FULL ) < NEIGHBOURS && *act_pos == CH_FULL )
        {
            ++count;
            *act_pos = CH_REMOVED;
        }
    }

    return count;
}

ssize_t process_files( const char *in_file_name, const char *out_file_name )
{
    int rv = 0;
    FILE *in_file = stdin;
    FILE *out_file = stdout;
    size_t sum = 0;

    line_buf_t lines_buf[ LINES_CHECKED ] = { 0 };
    circular_lines_t circular = { .lines = LINES_CHECKED, .idx = 0, .line_buf = lines_buf };

    if ( in_file_name != NULL )
    {
        in_file = fopen( in_file_name, "r" );
        if ( in_file == NULL )
        {
            warn( "opening in file" );
            rv = -1;
            goto finish;
        }
    }

    if ( out_file_name != NULL )
    {
        out_file = fopen( out_file_name, "w" );
        if ( out_file == NULL )
        {
            warn( "opening out file" );
            rv = -1;
            goto finish;
        }
    }

    if ( lines_init( &circular, in_file ) < 0 )
    {
        rv = -1;
        goto finish;
    }

    int actual_idx = circular.idx;
    int load_line_rv = actual_idx;

    do {
        sum += process_lines_two( &circular );
        actual_idx = load_line_rv;

        fprintf( out_file, "%s", actual_line( &circular ) );
    } while ( ( load_line_rv = lines_load_line( &circular ) ) >= 0 && load_line_rv != actual_idx );

    if ( load_line_rv < 0 )
        rv = -1;

    lines_add_dummy_line( &circular );
    sum += process_lines_two( &circular );
    fprintf( out_file, "%s", actual_line( &circular ) );

    finish:
        lines_destroy( &circular );

    if ( in_file != stdin )
        if ( fclose( in_file ) == EOF )
        {
            warn( "closing an in file" );
            rv = -1;
        }

    if ( out_file != stdout )
    {
        if ( fflush( out_file ) == EOF )
            warn( "flushing an out file" );

        if ( fclose( out_file ) == EOF )
        {
            warn( "closing an out file" );
            rv = -1;
        }
    }

    if ( rv == 0 )
        return ( ssize_t ) sum;

    return rv;
}


int main( int argc, char *argv[ ] )
{
    char *in_file_name = NULL,
         *out_file_name = NULL,
         *tmp_file = "tmp_file.txt",
         *tmp_p;
    size_t sum = 0;

    if ( argc == 3 )
    {
        in_file_name = argv[ 1 ];
        out_file_name = argv[ 2 ];

    }
    ssize_t rv = process_files( in_file_name, out_file_name );
    if ( rv < 0 )
        return 1;
    sum = rv;

#ifdef PART_TWO
    in_file_name = out_file_name;
    out_file_name = tmp_file;

    while ( ( rv = process_files( in_file_name, out_file_name ) ) > 0 )
    {
        sum += rv;
        tmp_p = in_file_name;
        in_file_name = out_file_name;
        out_file_name = tmp_p;
    }
    if ( rv < 0 )
        return 1;
#endif

    printf( "%lu\n", sum );
    return 0;
}
