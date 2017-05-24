#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "lru.h"
#include "shmlib.h"

static volatile void *addToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(SSDBufDespForLRU * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru);
static int hasBeenDeleted(SSDBufDespForLRU* ssd_buf_hdr_for_lru);
/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferForLRU()
{
    int stat = SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        ssd_buf_strategy_ctrl_lru =(SSDBufferStrategyControlForLRU *)SHM_alloc(SHM_SSDBUF_STRATEGY_CTRL,sizeof(SSDBufferStrategyControlForLRU));
        ssd_buf_desp_for_lru = (SSDBufDespForLRU *)SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(SSDBufDespForLRU) * NBLOCK_SSD_CACHE);

        ssd_buf_strategy_ctrl_lru->first_lru = -1;
        ssd_buf_strategy_ctrl_lru->last_lru = -1;
        SHM_mutex_init(&ssd_buf_strategy_ctrl_lru->lock);

        SSDBufDespForLRU *ssd_buf_hdr_for_lru = ssd_buf_desp_for_lru;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_lru = -1;
            ssd_buf_hdr_for_lru->last_lru = -1;
            ssd_buf_hdr_for_lru->next_self_lru = -1;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            SHM_mutex_init(&
            ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        ssd_buf_strategy_ctrl_lru =(SSDBufferStrategyControlForLRU *)SHM_get(SHM_SSDBUF_STRATEGY_CTRL,sizeof(SSDBufferStrategyControlForLRU));
        ssd_buf_desp_for_lru = (SSDBufDespForLRU *)SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(SSDBufDespForLRU) * NBLOCK_SSD_CACHE);

    }
    SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");
    self_ssd_buf_strategy_ctrl_lru = (SSDBufferStrategyControlForLRU *)malloc(sizeof(SSDBufferStrategyControlForLRU));
    self_ssd_buf_strategy_ctrl_lru->first_lru = -1;
    self_ssd_buf_strategy_ctrl_lru->last_lru = -1;
    return stat;
}

static volatile void *
addToLRUHead(SSDBufDespForLRU* ssd_buf_hdr_for_lru)
{
    if (ssd_buf_strategy_ctrl_lru->last_lru < 0)
    {
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->serial_id;
        ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    else
    {
        ssd_buf_hdr_for_lru->next_lru = ssd_buf_desp_for_lru[ssd_buf_strategy_ctrl_lru->first_lru].serial_id;
        ssd_buf_hdr_for_lru->last_lru = -1;
        ssd_buf_desp_for_lru[ssd_buf_strategy_ctrl_lru->first_lru].last_lru = ssd_buf_hdr_for_lru->serial_id;
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    //deal with self LRU queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(self_ssd_buf_strategy_ctrl_lru->last_lru < 0)
        {
            self_ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->serial_id;
            self_ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = ssd_buf_desp_for_lru[self_ssd_buf_strategy_ctrl_lru->first_lru].serial_id;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            ssd_buf_desp_for_lru[self_ssd_buf_strategy_ctrl_lru->first_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            self_ssd_buf_strategy_ctrl_lru->first_lru =  ssd_buf_hdr_for_lru->serial_id;

        }
    }
    return NULL;
}

static volatile void *
deleteFromLRU(SSDBufDespForLRU * ssd_buf_hdr_for_lru)
{
    if (ssd_buf_hdr_for_lru->last_lru >= 0)
    {
        ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->last_lru].next_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    else
    {
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    //deal with self queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(ssd_buf_hdr_for_lru->last_self_lru>=0)
        {
            ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->last_self_lru].next_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        }
        else
        {
            self_ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->next_self_lru;
        }
    }
    if (ssd_buf_hdr_for_lru->next_lru >= 0)
    {
        ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->next_lru].last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    else
    {
        ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    //deal with self queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(ssd_buf_hdr_for_lru->next_self_lru>=0)
        {
            ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->next_self_lru].last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        }
        else
        {
            self_ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->last_self_lru;
        }
    }

    ssd_buf_hdr_for_lru->last_lru = ssd_buf_hdr_for_lru->next_lru = -1;

    return NULL;
}

static volatile void *
moveToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);
    return NULL;
}

long
Unload_LRUBuf()
{
    _LOCK(&ssd_buf_strategy_ctrl_lru->lock);

    long frozen_id = self_ssd_buf_strategy_ctrl_lru->last_lru;
    deleteFromLRU(&ssd_buf_desp_for_lru[frozen_id]);

    _UNLOCK(&ssd_buf_strategy_ctrl_lru->lock);
    return frozen_id;

//    if ((old_flag & SSD_BUF_VALID) != 0)
//    {
//        unsigned long	old_hash = HashTab_GetHashCode(&old_tag);
//        HashTab_Delete(&old_tag, old_hash);
//    }
}

int
hitInLRUBuffer(long serial_id)
{
    _LOCK(&ssd_buf_strategy_ctrl_lru->lock);

    SSDBufDespForLRU* ssd_buf_hdr_for_lru = &ssd_buf_desp_for_lru[serial_id];
    if(hasBeenDeleted(ssd_buf_hdr_for_lru))
    {
        _UNLOCK(&ssd_buf_strategy_ctrl_lru->lock);
        return -1;
    }
    moveToLRUHead(ssd_buf_hdr_for_lru);
    _UNLOCK(&ssd_buf_strategy_ctrl_lru->lock);

    return 0;
}

void*
insertLRUBuffer(long serial_id)
{
    _LOCK(&ssd_buf_strategy_ctrl_lru->lock);

    //setting for maintaining our queue
    ssd_buf_desp_for_lru[serial_id].user_id = UserId;

    addToLRUHead(&ssd_buf_desp_for_lru[serial_id]);

    _UNLOCK(&ssd_buf_strategy_ctrl_lru->lock);
    return 0;
}

static int
hasBeenDeleted(SSDBufDespForLRU* ssd_buf_hdr_for_lru)
{
    if(ssd_buf_hdr_for_lru->last_lru < 0 && ssd_buf_hdr_for_lru->next_lru < 0)
        return 1;
    else
        return 0;
}
