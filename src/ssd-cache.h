#define DEBUG 0
/* ---------------------------ssd cache---------------------------- */
#ifndef _SSD_CACHE_H
#define _SSD_CACHE_H 1

#include <pthread.h>

#define off_t	unsigned long
#define bool	unsigned char

typedef struct
{
    off_t	offset;
} SSDBufferTag;

typedef struct
{
    long        serial_id;              // the serial number of the descriptor corresponding to SSD buffer.
    long 		ssd_buf_id;				// SSD buffer location in shared buffer
    unsigned 	ssd_buf_flag;
    long		next_freessd;           // to link the desp serial number of free SSD buffer
    SSDBufferTag 	ssd_buf_tag;
    pthread_mutex_t lock;               // For the fine grain size
} SSDBufDesp;

#define SSD_BUF_VALID 0x01
#define SSD_BUF_DIRTY 0x02

typedef struct SSDBufHashCtrl
{
    pthread_mutex_t lock;
} SSDBufHashCtrl;

typedef struct SSDBufHashBucket
{
    SSDBufferTag 			hash_key;
    long    				desp_serial_id;
    struct SSDBufHashBucket 	*next_item;
} SSDBufHashBucket;

typedef struct
{
    long		n_usedssd;			// For eviction
    long		first_freessd;		// Head of list of free ssds
    long		last_freessd;		// Tail of list of free ssds
    pthread_mutex_t lock;
} SSDBufDespCtrl;

typedef enum
{
    CLOCK = 0,
    LRU,
    LRUofBand,
    Most,
    Most_Dirty,
    SCAN,
    CMR,
    SMR,
    WA,
    MaxCold,
    MaxAll,
    AvgBandHot,
    HotDivSize,
    /** add for multiuser **/
    LRU_global,
    LRU_peruser
} SSDEvictionStrategy;

#define GetSSDBufHashBucket(hash_code) ((SSDBufHashBucket *) (ssd_buf_hashtable + (unsigned) (hash_code)))

extern int hdd_fd;
extern int ssd_fd;

extern size_t BLCKSZ;
extern size_t BNDSZ;
extern int BandOrBlock;

extern SSDBufHashCtrl   *ssd_buf_hash_ctrl;
extern SSDBufHashBucket	*ssd_buf_hashtable;
extern SSDBufHashBucket *ssd_buf_hashdesps;

extern SSDBufDespCtrl *ssd_buf_desp_ctrl;
extern SSDBufDesp	*ssd_buf_desps;

extern pthread_mutex_t  *lock_process_req;

extern unsigned long hit_num;
extern unsigned long load_ssd_blocks;
extern unsigned long load_hdd_blocks;
extern unsigned long flush_hdd_blocks;
extern unsigned long flush_ssd_blocks;

extern unsigned long hashmiss_sum;
extern unsigned long hashmiss_read;
extern unsigned long hashmiss_write;

extern double time_read_hdd;
extern double time_write_hdd;
extern double time_read_ssd;
extern double time_write_ssd;

extern unsigned long read_hit_num;

extern void initSSD();
extern void read_block(off_t offset, char* ssd_buffer);
extern void write_block(off_t offset,int blkcnt, char* ssd_buffer);
extern void read_band(off_t offset, char* ssd_buffer);
extern void write_band(off_t offset, char* ssd_buffer);
extern bool isSamebuf(SSDBufferTag *, SSDBufferTag *);

extern void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag);

extern void         _LOCK(pthread_mutex_t* lock);
extern void         _UNLOCK(pthread_mutex_t* lock);

extern unsigned long NBLOCK_SSD_CACHE;
extern unsigned long NTABLE_SSD_CACHE;
extern size_t SSD_BUFFER_SIZE;
extern char	smr_device[100];
extern int 	smr_fd;
extern int 	ssd_fd;
extern SSDEvictionStrategy EvictStrategy;

extern const char* SHM_SSDBUF_STRATEGY_CTRL;
extern const char* SHM_SSDBUF_STRATEGY_DESP;
extern const char* SHM_SSDBUF_DESP_CTRL;
extern const char* SHM_SSDBUF_DESPS;
extern const char* SHM_SSDBUF_HASHTABLE_CTRL;
extern const char* SHM_SSDBUF_HASHTABLE;
extern const char* SHM_SSDBUF_HASHDESPS;
extern const char* SHM_PROCESS_REQ_LOCK;

extern unsigned long MaxSSD;
extern unsigned long UsedSSD;
extern unsigned long UserId;

#endif
