/*
 * Common display code for VGA and DVI
 */
#include <stdio.h>
#define MAX_FREE 4
#define MAX_PEND 2

// Note: Need 40 buffers, as the ZX81 produces rates at greater than 50 Hz
// so 2 frames can be created in one time slice, and 1 emulated time slice
// may complete in 14ms, therefore a backlog of 2 frames is valid, no_skip
// when nominally frame matched
static  bool blank = true;
static  uint16_t blank_colour = BLACK;

static semaphore_t display_initialised;
static mutex_t next_frame_mutex;

static uint8_t* curr_buff = 0;
static uint8_t* pend_buff[MAX_PEND] = {0, 0};
static uint8_t* free_buff[MAX_FREE] = {0, 0, 0, 0};
static uint8_t free_count = 0;
static uint8_t pend_count = 0;

static uint16_t stride = 0;
static bool interlace = true;
static bool no_skip = false;

//
// Private interface
//
static void core1_main();
static inline void freeAllPending(void);
static inline void newFrame(void);

//
// Public functions
//
void displayStart(void)
{
    // create a semaphore to be posted when video init is complete
    sem_init(&display_initialised, 0, 1);

    // launch all the video on core 1, so it isn't affected by USB handling on core 0
    multicore_launch_core1(core1_main);

    sem_acquire_blocking(&display_initialised);
}

/* Obtain a buffer from the free list */
void __not_in_flash_func(displayGetFreeBuffer)(uint8_t** buff)
{
    mutex_enter_blocking(&next_frame_mutex);
    if (free_count)
    {
        *buff = free_buff[0];
        --free_count;

        // Move items towards front of free list
        for (int i=0; i<free_count; ++i)
        {
            free_buff[i] = free_buff[i+1];
        }
    }
    else
    {
        // No buffer available - which means that must be 2 buffers waiting
        // to be displayed. We have the lock, so force the queued buffer now
        // and take the current one
        if (pend_count == 2)
        {
            *buff = curr_buff;
            curr_buff = pend_buff[0];
            pend_buff[0] = pend_buff[1];
            --pend_count;
        }
        else
        {
            printf("In displayGetFreeBuffer: pend_count = %i\n", pend_count);
        }
    }
    mutex_exit(&next_frame_mutex);
}

/* Display the buffer, optionally delaying presentation until vsync
   Option to put the currently displayed buffer onto free list,
   note the option to not put the previous buffer onto the
   free list allows for it to be redisplayed later (e.g.
   after a menu is removed) */
void __not_in_flash_func(displayBuffer)(uint8_t* buff, bool sync, bool free)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    if (sync && !blank)
    {
        // Store in next, unless next is full
        if (pend_count != MAX_PEND)
        {
            pend_buff[pend_count++] = buff;
        }
        else
        {
            // Already have two next buffers
            if (interlace)
            {
                // Release 2
                free_buff[free_count++] = pend_buff[0];
                free_buff[free_count++] = pend_buff[1];

                pend_buff[0] = buff;
                pend_count = 1;
                printf("D2\n");
            }
            else
            {
                // Release 1
                free_buff[free_count++] = pend_buff[0];

                pend_buff[0] = pend_buff[1];
                pend_buff[1] = buff;
                printf("D1\n");
            }
        }
    }
    else
    {
        // Display immediately
        freeAllPending();

        // Read existing
        uint8_t* temp_buff = curr_buff;

        // write existing
        curr_buff = buff;
        if (temp_buff && free)
            free_buff[free_count++] = temp_buff;

        blank = false;
    }

    // free lock
    mutex_exit(&next_frame_mutex);
}

/* Get a pointer to the buffer currently being displayed.
   Typically this is done if the buffer should be redisplayed
   after a menu is displayed and subsequently removed*/
void __not_in_flash_func(displayGetCurrentBuffer)(uint8_t** buff)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    // Want the current buffer, or the queued buffer if one exists
    if (pend_count)
    {
        // Free the current buffer
        free_buff[free_count++] = curr_buff;

        curr_buff = pend_buff[--pend_count];
        freeAllPending();
    }
    *buff = curr_buff;

    // free lock
    mutex_exit(&next_frame_mutex);
}

void __not_in_flash_func(displayBlank)(bool black)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    blank = true;
    blank_colour = black ? BLACK : WHITE;

    /* Free all buffers */
    freeAllPending();

    if (curr_buff)
    {
        free_buff[free_count++] = curr_buff;
        curr_buff = 0;
    }
    // free lock
    mutex_exit(&next_frame_mutex);
}

bool displayIsBlank(bool* isBlack)
{
    *isBlack = (blank_colour == BLACK);
    return blank;
}

//
// Private function
//

static inline void __not_in_flash_func(newFrame)(void)
{
    mutex_enter_blocking(&next_frame_mutex);
    if (!blank && pend_count)
    {
        if ((!interlace) || no_skip)
        {
            free_buff[free_count++] = curr_buff;
            curr_buff = pend_buff[0];
            --pend_count;
            if (pend_count)
            {
                pend_buff[0] = pend_buff[1];
            }
        }
        else
        {
            no_skip = !no_skip;
            printf("S\n");

        }
    }
    else
    {
        no_skip = !no_skip;
    }
    mutex_exit(&next_frame_mutex);
}

static inline void __not_in_flash_func(freeAllPending)(void)
{
    for (int i=0; i<pend_count; ++i)
    {
        free_buff[free_count++] = pend_buff[i];
    }
    pend_count = 0;
}