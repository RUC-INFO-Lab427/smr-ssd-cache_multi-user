#define DEBUG 0
/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		serial_id;			// the corresponding descriptor serial number.
    long        next_lru;               // to link used SSD as LRU
    long        last_lru;               // to link used SSD as LRU
    long        next_self_lru;
    long        last_self_lru;
    unsigned long   user_id;
    pthread_mutex_t lock;
} SSDBufDespForLRU;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
} SSDBufferStrategyControlForLRU;

/********
 ** SHM**
 ********/
SSDBufferStrategyControlForLRU *ssd_buf_strategy_ctrl_lru;
SSDBufDespForLRU	*ssd_buf_desp_for_lru;
SSDBufferStrategyControlForLRU    *self_ssd_buf_strategy_ctrl_lru;
extern unsigned long flush_times;


extern int initSSDBufferForLRU();
extern long Unload_LRUBuf();
extern int hitInLRUBuffer(long serial_id);
extern void *insertLRUBuffer(long serial_id);
