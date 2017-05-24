#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "ssd-cache.h"
//#include "smr-simulator/smr-simulator.h"
#include "ssd_buf_table.h"
#include "strategy/lru.h"
#include "shmlib.h"

static int          init_SSDDescriptorBuffer();
static int          init_StatisticObj();
static void flushSSDBuffer(SSDBufDesp * ssd_buf_hdr);
static SSDBufDesp*  allocSSDBuf(SSDBufferTag *ssd_buf_tag, bool * found, int alloc4What);
static SSDBufDesp*  getAFreeSSDBuf();

static int          initStrategySSDBuffer(SSDEvictionStrategy strategy);
static long         Strategy_GetUnloadBufID(SSDBufferTag *ssd_buf_tag, SSDEvictionStrategy strategy);
static int          Strategy_HitIn(long serial_id, SSDEvictionStrategy strategy);
static void*        Strategy_AddBufID(long serial_id);

void                CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag);

void         _LOCK(pthread_mutex_t* lock);
void         _UNLOCK(pthread_mutex_t* lock);
/* stopwatch */
static double time_begin_temp;
static double time_now_temp;
static struct timeval   tv;
static struct timezone	tz;
static void startTimer();
static void stopTimer();
static double lastTimerinterval();

/* Device I/O operation with Timer */
static int dev_pread(int fd, void* buf,size_t nbytes,off_t offset);
static int dev_pwrite(int fd, void* buf,size_t nbytes,off_t offset);
static char* ssd_buffer;

//for our queue
unsigned long MaxSSD;
unsigned long UsedSSD;
unsigned long UserId;


/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */
void
initSSD()
{
    int r_initstrategybuf   =   initStrategySSDBuffer(EvictStrategy);
    int r_initbuftb         =   HashTab_Init();
    int r_initdesp          =   init_SSDDescriptorBuffer();
    int r_initstt           =   init_StatisticObj();
    printf("init_Strategy: %d, init_table: %d, init_desp: %d, inti_Stt: %d\n",r_initstrategybuf, r_initbuftb, r_initdesp, r_initstt);

    int returnCode;
    returnCode = posix_memalign(&ssd_buffer, 512, sizeof(char) * BLCKSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] flushSSDBuffer():--------posix memalign\n");
        free(ssd_buffer);
        exit(-1);
    }
    UsedSSD = 0;
}

static int
init_SSDDescriptorBuffer()
{
    int stat = SHM_lock_n_check("LOCK_SSDBUF_DESP");
    if(stat == 0)
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl*)SHM_alloc(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)SHM_alloc(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);

        ssd_buf_desp_ctrl->n_usedssd = 0;
        ssd_buf_desp_ctrl->first_freessd = 0;
        ssd_buf_desp_ctrl->last_freessd = NBLOCK_SSD_CACHE - 1;
        SHM_mutex_init(&ssd_buf_desp_ctrl->lock);

        long i;
        SSDBufDesp  *ssd_buf_hdr = ssd_buf_desps;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr++, i++)
        {
            ssd_buf_hdr->serial_id = i;
            ssd_buf_hdr->ssd_buf_id = i;
            ssd_buf_hdr->ssd_buf_flag = 0;
            ssd_buf_hdr->next_freessd = i + 1;
            SHM_mutex_init(&ssd_buf_hdr->lock);
        }
        ssd_buf_desps[NBLOCK_SSD_CACHE - 1].next_freessd = -1;
    }
    else
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl *)SHM_get(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)SHM_get(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);
    }
    SHM_unlock("LOCK_SSDBUF_DESP");
    return stat;
}

static int
init_StatisticObj()
{
    int stat = SHM_lock_n_check("GLOBALSTATISTIC");
    if(stat == 0)
    {
        hit_num = 0;
        load_ssd_blocks = 0;
        flush_ssd_blocks = 0;
        load_hdd_blocks = 0;
        flush_hdd_blocks = 0;
        time_read_hdd = 0.0;
        time_write_hdd = 0.0;
        time_read_ssd = 0.0;
        time_write_ssd = 0.0;
        hashmiss_sum = 0;
        hashmiss_read = 0;
        hashmiss_write = 0;
        read_hit_num = 0;
    }
    SHM_unlock("GLOBALSTATISTIC");
    return stat;
}

static void
flushSSDBuffer(SSDBufDesp * ssd_buf_hdr)
{
//    if (BandOrBlock == 1)
//    {
//        SSD_BUFFER_SIZE = BNDSZ;
//        BLCKSZ = BNDSZ;
//    }
//
    dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    time_read_ssd += lastTimerinterval();
    load_ssd_blocks++;

    dev_pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
    time_write_hdd += lastTimerinterval();
    flush_hdd_blocks++;
}

static SSDBufDesp*
allocSSDBuf(SSDBufferTag *ssd_buf_tag, bool * found, int alloc4What)
{
    /* Lookup if already cached. */
    SSDBufDesp      *ssd_buf_hdr;
    unsigned long   ssd_buf_hash = HashTab_GetHashCode(ssd_buf_tag);
    long            ssd_buf_id = HashTab_Lookup(ssd_buf_tag, ssd_buf_hash);

    /* Cache HIT IN */
    if (ssd_buf_id >= 0)
    {
        ssd_buf_hdr = &ssd_buf_desps[ssd_buf_id];
        _LOCK(&ssd_buf_hdr->lock);
        if(isSamebuf(&ssd_buf_hdr->ssd_buf_tag,ssd_buf_tag))
        {
            Strategy_HitIn(ssd_buf_hdr->serial_id, EvictStrategy); //need lock
            hit_num++;
            *found = 1;
            return ssd_buf_hdr;
        }
        else
        {
            _UNLOCK(&ssd_buf_hdr->lock);
            /** passive delete hash item, which corresponding cache buf has been evicted early **/
            HashTab_Delete(ssd_buf_tag,ssd_buf_hash);
            hashmiss_sum++;
            if(alloc4What == 1)	// alloc for write
                hashmiss_write++;
            else		//alloc for read
                hashmiss_read++;
        }
    }

    /* Cache MISS */
    *found = 0;

    _LOCK(&ssd_buf_desp_ctrl->lock);
    ssd_buf_hdr = getAFreeSSDBuf();
    _UNLOCK(&ssd_buf_desp_ctrl->lock);

    if (ssd_buf_hdr != NULL)
    {
        _LOCK(&ssd_buf_hdr->lock);
        // if there is free SSD buffer.
        Strategy_AddBufID(ssd_buf_hdr->serial_id);
    }
    else
    {
        /** When there is NO free SSD space for cache, TODO flush **/
        long renew_buf = Strategy_GetUnloadBufID(ssd_buf_tag, EvictStrategy); //need look
        ssd_buf_hdr = &ssd_buf_desps[renew_buf];
        _LOCK(&ssd_buf_hdr->lock);

        unsigned char	old_flag = ssd_buf_hdr->ssd_buf_flag;
        if ((old_flag & SSD_BUF_DIRTY) != 0)
        {
            flushSSDBuffer(ssd_buf_hdr);
        }

        Strategy_AddBufID(ssd_buf_hdr->serial_id);
    }

    ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
    CopySSDBufTag(&ssd_buf_hdr->ssd_buf_tag,ssd_buf_tag);

    HashTab_Insert(ssd_buf_tag, ssd_buf_hash, ssd_buf_hdr->serial_id);
    return ssd_buf_hdr;
}

static int
initStrategySSDBuffer(SSDEvictionStrategy strategy)
{
    /** Add for multi-user **/
    if (strategy == LRU)
        return initSSDBufferForLRU();
    return -1;
}

static long
Strategy_GetUnloadBufID(SSDBufferTag *ssd_buf_tag, SSDEvictionStrategy strategy)
{

    if (strategy == LRU)
        return Unload_LRUBuf();
    return -1;
}

static int
Strategy_HitIn(long serial_id, SSDEvictionStrategy strategy)
{
    if (strategy == LRU)
        return hitInLRUBuffer(serial_id);
    return -1;
}

static void*
Strategy_AddBufID(long serial_id)
{
    if(EvictStrategy == LRU)
        return insertLRUBuffer(serial_id);
}
/*
 * read--return the buf_id of buffer according to buf_tag
 */

void
read_block(off_t offset, char *ssd_buffer)
{
    bool found = 0;
    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp* ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    if (DEBUG)
        printf("[INFO] read():-------offset=%lu\n", offset);

    ssd_buf_hdr = allocSSDBuf(&ssd_buf_tag, &found, 0);

    if (found)
    {
        read_hit_num++;
        dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        time_read_ssd += lastTimerinterval();
        load_ssd_blocks++;
    }
    else
    {
        dev_pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
        time_read_hdd += lastTimerinterval();
        load_hdd_blocks++;

        dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        time_write_ssd += lastTimerinterval();
        flush_ssd_blocks++;
    }
    ssd_buf_hdr->ssd_buf_flag &= ~SSD_BUF_DIRTY;
    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID;

    _UNLOCK(&ssd_buf_hdr->lock);
}

/*
 * write--return the buf_id of buffer according to buf_tag
 */
void
write_block(off_t offset,int blkcnt, char *ssd_buffer)
{
    bool	found;

    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp   *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    if (DEBUG)
        printf("[INFO] write():-------offset=%lu\n", offset);

    ssd_buf_hdr = allocSSDBuf(&ssd_buf_tag, &found, 1);

    dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    time_write_ssd += lastTimerinterval();
    flush_ssd_blocks++;

    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;
    _UNLOCK(&ssd_buf_hdr->lock);

}

/******************
**** Utilities*****
*******************/

static int dev_pread(int fd, void* buf,size_t nbytes,off_t offset)
{
    startTimer();
    int r = pread(fd,buf,nbytes,offset);
    stopTimer();
    if (r < 0)
    {
        printf("[ERROR] read():-------read from device: fd=%d, errorcode=%d, offset=%lu\n", fd, r, offset);
        exit(-1);
    }
    return r;
}

static int dev_pwrite(int fd, void* buf,size_t nbytes,off_t offset)
{
    startTimer();
    int w = pwrite(fd,buf,nbytes,offset);
    stopTimer();
    if (w < 0)
    {
        printf("[ERROR] read():-------read from device: fd=%d, errorcode=%d, offset=%lu\n", fd, w, offset);
        exit(-1);
    }
    return w;
}

static void startTimer()
{
    gettimeofday(&tv, &tz);
    time_begin_temp = tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void stopTimer()
{
    gettimeofday(&tv, &tz);
    time_now_temp = tv.tv_sec + tv.tv_usec / 1000000.0;
}

static double lastTimerinterval()
{
    return time_now_temp - time_begin_temp;
}

void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag)
{
    objectTag->offset = sourceTag->offset;
}

bool isSamebuf(SSDBufferTag *tag1, SSDBufferTag *tag2)
{
    if (tag1->offset != tag2->offset)
        return 0;
    else return 1;
}

static SSDBufDesp*
getAFreeSSDBuf()
{
    if(ssd_buf_desp_ctrl->first_freessd < 0 || UsedSSD >= MaxSSD)
        return NULL;

    SSDBufDesp* ssd_buf_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
    ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->next_freessd;
    ssd_buf_hdr->next_freessd = -1;
    ssd_buf_desp_ctrl->n_usedssd++;
    //counting used ssd for this user
    UsedSSD++;
    return ssd_buf_hdr;
}

void
_LOCK(pthread_mutex_t* lock)
{
    SHM_mutex_lock(lock);
}

void
_UNLOCK(pthread_mutex_t* lock)
{
    SHM_mutex_unlock(lock);
}
