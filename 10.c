#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define ALLOC_BATCH 32

typedef struct node_t {
    uint32_t obj;
    struct node_t *next;
} node_t;

typedef struct queue_t {
    node_t *head;
    node_t* tail;
} queue_t;

int queue_init(queue_t **queue_ptr) {
    queue_t *new_queue = calloc(sizeof (queue_t), 1);
    if (new_queue == NULL) {
        return -1;
    }
    *queue_ptr = new_queue;
    return 0;
}

int queue_push(queue_t *queue, uint32_t obj) {
    node_t *new_node = calloc(sizeof (node_t), 1);
    if (new_node == NULL) {
        return -1;
    }
    new_node->obj = obj;

    if (queue->head == NULL) {
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    return 0;
}

int queue_pop(queue_t *queue, uint32_t *poped) {
    if (queue->head == NULL) {
        return 1;
    }

    node_t *old_node = queue->head;
    if (queue->head == queue->tail) {  // only one element
        queue->head = NULL;
        queue->tail = NULL;

    } else {
        queue->head = old_node->next;
    }

    if (poped != NULL) {
        *poped = old_node->obj;
    }

    free(old_node);

    return 0;
}

bool queue_is_empty(queue_t *queue) {
    return queue->head == NULL && queue->tail == NULL;
}

int queue_destroy(queue_t **queue_ptr) {
    while ((*queue_ptr)->head != NULL) {
        queue_pop(*queue_ptr, NULL);
    }
    free(*queue_ptr);
    *queue_ptr = NULL;

    return 0;
}


void queue_print(queue_t *queue) {
    node_t *act_node = queue->head;

    printf("Queue at %p:", queue);

    while (act_node != NULL) {
        printf(" <- %d", *((int*) act_node->obj));
        act_node = act_node->next;
    }
    printf("\n");
}

unsigned long long queue_size(queue_t *queue) {
    node_t *act_node = queue->head;
    unsigned long long count = 0;

    while (act_node != NULL) {
        count++;
        act_node = act_node->next;
    }

    return count;
}


typedef struct machine_t
{
    uint32_t desired_indicator;
    uint32_t *buttons;
    uint32_t *joltages;
} machine_t;

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

int load_joltages( char **line, uint32_t **joltages_p )
{
    return 0;
}

int load_buttons( char **line, uint32_t **buttons_p )
{
    char *save_ptr = NULL;
    char *line_l = *line;
    char *button = NULL;
    char *end_p;
    int rv = -1;

    uint32_t *buttons = calloc( ALLOC_BATCH, sizeof( uint32_t ) );
    if ( buttons == NULL )
        goto finload;

    int button_idx = 0;

    while ( ( button = strtok_r( line_l, " (", &save_ptr ) ) != NULL)
    {
        uint64_t actual_button = 0;
        uint64_t actual_num = 0;
        if ( *button == '{' )
        {
            *line = button;
            break;
        }

        end_p = button - 1;
        --button;

        while ( *end_p != ')' && *end_p != '\0' )
        {
            if ( strtoull_rv( end_p + 1, &end_p, &actual_num ) == -1 )
                goto finload;

            actual_button |= ( 1ul << actual_num );
        }
        buttons[ button_idx++ ] = actual_button;
        line_l = NULL;

    }

    rv = 0;
    *buttons_p = buttons;
finload:
    if ( rv != 0 )
        if ( buttons != NULL )
            free( buttons );
    return rv;

}

int process_line( char *line, machine_t *machine )
{
    machine_t machine_local = { 0 };

    if ( *line != '[' )
    {
        warnx( "incorrect file format, misssing [" );
        return -1;
    }

    ++line;
    int idx = 0;

    for ( ; *line != ']'; ++line, ++idx )
    {
        if ( *line == '#' )
            machine_local.desired_indicator |= ( 1u << idx );
        else
            if ( *line != '.' )
            {
                warnx( "incorrect file format, misssing [" );
                return -1;
            }
    }
    line += 2;

    if ( load_buttons( &line, &machine_local.buttons ) == -1 )
        return -1;

    if ( load_joltages( &line, &machine_local.joltages ) == -1 )
        return -1;

    *machine = machine_local;
    return 0;
}

int data_load( machine_t **machines, int *machines_n, FILE *in_file )
{
    int rv = -1;
    ssize_t bytes_r;
    size_t n = 0;

    char *line = NULL;
    machine_t *tmp, *machines_l = NULL;
    int idx = 0;

    errno = 0;
    while ( ( bytes_r = getdelim( &line, &n, '\n', in_file ) ) > 0 )
    {
        if ( idx % ALLOC_BATCH == 0 )
        {
            int new_n = idx + ALLOC_BATCH;
            tmp = realloc( machines_l, new_n * sizeof( machine_t ) );
            if ( tmp == NULL )
                goto final;
            machines_l = tmp;
        }

        if ( process_line( line, machines_l + idx ) < 0 )
            goto final;

        ++idx;
    }

    if ( bytes_r < 0 && errno != 0 )
    {
        warn( "reading from a file" );
        goto final;
    }

    rv = 0;

    *machines = machines_l;
    *machines_n = idx;

    final:
        if ( line != NULL )
            free( line );

    return rv;
}

ssize_t press_button( uint32_t *buttons, uint32_t state, queue_t *queue1, queue_t *queue2 )
{
    if ( queue_push( queue1, state ) == -1 ){
        warnx( "cannot push to queue init" );
        return -1;
    }

    uint32_t presses = 1;

    while ( 1 )
    {
        while ( !queue_is_empty( queue1 ) )
        {
            uint32_t *buttons_l = buttons;
            uint32_t state_from_queue;
            queue_pop( queue1, &state_from_queue );

            for ( ; *buttons_l != 0; ++buttons_l )
            {
                uint32_t new_state = state_from_queue ^ buttons_l[ 0 ];
                if ( new_state == 0 )
                    return presses;

                if ( queue_push( queue2, new_state ) == -1 )
                {
                    warnx( "cannot push to queue" );
                    return -1;
                }
            }
        }
        queue_t *tmp = queue1;
        queue1 = queue2;
        queue2 = tmp;
        ++presses;
    }
}

uint32_t button_presses( machine_t *machine )
{
    queue_t *queue = NULL;
    if ( queue_init( &queue ) == -1 )
    {
        warnx( "cannot create queue" );
        return -1;
    }

    queue_t *queue2 = NULL;
    if ( queue_init( &queue2 ) == -1 )
    {
        warnx( "cannot create queue" );
        return -1;
    }

    ssize_t rv = press_button( machine->buttons, machine->desired_indicator, queue, queue2 );

    queue_destroy( &queue );
    queue_destroy( &queue2 );

    return rv;
}

uint64_t part_one( int machines_n, machine_t *machines )
{
    uint64_t sum = 0;
    for ( int i = 0; i < machines_n; ++i )
        sum += button_presses( machines + i );

    return sum;
}
uint64_t part_two( int machines_n, machine_t *machines );


ssize_t process_data( FILE *in_file, uint64_t ( *part_fn )( int, machine_t* ) )
{
    ssize_t rv = -1;

    int machines_n = 0;
    machine_t *machines = NULL;

    if ( data_load( &machines, &machines_n, in_file ) < 0 )
        goto finalize;


    rv = part_fn( machines_n, machines );

finalize:
    if ( machines != NULL )
    {
        for ( int i = 0; i < machines_n; ++i )
        {
            if ( machines[ i ].buttons != NULL )
                free( machines[ i ].buttons );
            if ( machines[ i ].joltages != NULL )
                free( machines[ i ].joltages );
        }
        free( machines );
    }

    return rv;
}

int main( int argc, char *argv[ ] )
{

    uint64_t ( *part_fn )( int, machine_t* ) = part_one;
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
