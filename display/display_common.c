/*
 * Common display code for VGA and DVI
 */
#define MAX_BUFFERS 4
#define MAX_NEXT 2

// Note: Need 40 buffers, as the ZX81 produces rates at greater than 50 Hz
// so 2 frames can be created in one time slice, and 1 emulated time slice
// may complete in 14ms, therefore a backlog of 2 frames is valid, even
// when nominally frame matched
static  bool blank = true;
static  uint16_t blank_colour = BLACK;

static semaphore_t display_initialised;
static mutex_t next_frame_mutex;

static uint8_t* curr_buff = 0;
static uint8_t* next_buff[MAX_NEXT] = {0, 0};
static uint8_t* free_buff[MAX_BUFFERS] = {0, 0, 0, 0};
static uint8_t max_free = 0;
static uint8_t next_count = 0;

static uint16_t stride = 0;

//
// Private interface
//
static void core1_main();

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
    if (max_free)
    {
        *buff = free_buff[0];
        --max_free;

        for (int i=0; i<max_free; ++i)
        {
            free_buff[i] = free_buff[i+1];
        }
    }
    else
    {
        // No buffer available - which means that must be 2 buffers waiting
        // to be displayed. We have the lock, so force the queued buffer now
        // and take the current one
        *buff = curr_buff;
        curr_buff = next_buff[0];
        next_buff[0] = next_buff[1];
        next_count = 1;
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
        if (next_count != MAX_NEXT)
        {
            next_buff[next_count++] = buff;
        }
        else
        {
            // Already have two next buffers, so release 1
            free_buff[max_free++] = next_buff[0];

            next_buff[0] = next_buff[1];
            next_buff[1] = buff;
        }
    }
    else
    {
        // Display immediately
        for (int i=0; i<next_count; ++i)
        {
            free_buff[max_free++] = next_buff[i];
        }
        next_count = 0;

        // Read existing
        uint8_t* temp_buff = curr_buff;

        // write existing
        curr_buff = buff;
        if (temp_buff && free)
            free_buff[max_free++] = temp_buff;

        blank = false;
    }

    // free lock
    mutex_exit(&next_frame_mutex);
}

/* Get a pointer to the buffer currently being displayed
   typically this is done if the buffer should be redisplayed
   after a menu is displayed */
void __not_in_flash_func(displayGetCurrentBuffer)(uint8_t** buff)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    // Want the current buffer, or the queued buffer if one exists
    if (next_count)
    {
        // Free the current buffer
        free_buff[max_free++] = curr_buff;

        curr_buff = next_buff[--next_count];

        for (int i=0; i<next_count; ++i)
        {
            free_buff[max_free++] = next_buff[i];
        }
        next_count = 0;
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
    for (int i=0; i<next_count; ++i)
    {
        free_buff[max_free++] = next_buff[i];
    }
    next_count = 0;

    if (curr_buff)
    {
        free_buff[max_free++] = curr_buff;
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

static inline void __not_in_flash_func(displayNewFrame)(void)
{
    mutex_enter_blocking(&next_frame_mutex);
    if (!blank && next_count)
    {
        free_buff[max_free++] = curr_buff;
        curr_buff = next_buff[0];
        --next_count;
        if (next_count)
        next_buff[0] = next_buff[1];
    }
    mutex_exit(&next_frame_mutex);
}
