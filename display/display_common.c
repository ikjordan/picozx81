/*
 * Common display code for VGA and DVI
 */
#define MAX_BUFFERS 3

static volatile bool blank = true;
static volatile uint16_t blank_colour = BLACK;

static semaphore_t display_initialised;
static mutex_t next_frame_mutex;

static uint8_t* curr_buff = 0;
static uint8_t* next_buff = 0;
static uint8_t* free_buff[MAX_BUFFERS] = {0, 0, 0};
static uint8_t max_free = 0;

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
void displayGetFreeBuffer(uint8_t** buff)
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
        // No buffer available - which means that must be a buffer waiting
        // to be displayed. We have the lock, so force the queued buffer now
        // and take the current one
        *buff = curr_buff;
        curr_buff = next_buff;
        next_buff = 0;
    }
    mutex_exit(&next_frame_mutex);
}

/* Return a buffer to free list without displaying it */
void displayFreeBuffer(uint8_t* buff)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    free_buff[max_free++] = buff;

    // free lock
    mutex_exit(&next_frame_mutex);
}

/* Display the buffer, optionally delaying presentation until vsync
   Option to put the currently displayed buffer onto free list,
   note the option to mot put the previous buffer onto the
   free list allows for it to be redisplayed later (e.g.
   after a menu is removed) */
void displayBuffer(uint8_t* buff, bool sync, bool free)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    // Return any existing queue to free list
    if (next_buff)
    {
        free_buff[max_free++] = next_buff;
    }

    if (sync && !blank)
    {
        // Store in new frame buffer
        next_buff = buff;
    }
    else
    {
        next_buff = 0;

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
void displayGetCurrentBuffer(uint8_t** buff)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    // Want the current buffer, or the queued buffer if one exists
    if (next_buff)
    {
        // Free the current buffer
        free_buff[max_free++] = curr_buff;
        curr_buff = next_buff;
        next_buff = 0;
    }
    *buff = curr_buff;

    // free lock
    mutex_exit(&next_frame_mutex);
}

void displayBlank(bool black)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    blank = true;
    blank_colour = black ? BLACK : WHITE;

    /* Free all buffers */
    if (next_buff)
    {
        free_buff[max_free++] = next_buff;
        next_buff = 0;
    }

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

static inline void displayNewFrame(void)
{
    mutex_enter_blocking(&next_frame_mutex);
    if (next_buff)
    {
        free_buff[max_free++] = curr_buff;
        curr_buff = next_buff;
        next_buff = 0;
    }
    mutex_exit(&next_frame_mutex);
}
