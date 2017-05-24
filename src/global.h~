#ifndef _GLOBAL_H
#define _GLOBAL_H 1
#include "ssd-cache.h"
unsigned long NBLOCK_SSD_CACHE;
unsigned long NTABLE_SSD_CACHE;
unsigned long NSMRBands = 19418000;		// 194180*(18MB+36MB)/2~5TB
unsigned long NSMRBlocks = 2621952;		// 2621952*8KB~20GB
unsigned long NSSDs;
unsigned long NSSDTables;
unsigned long NBANDTables = 2621952;
size_t SSD_BUFFER_SIZE;;

size_t SSD_SIZE = 4096;
size_t BLCKSZ = 4096;
size_t BNDSZ = 36*1024*1024;
size_t ZONESZ;
unsigned long INTERVALTIMELIMIT = 100000000;
unsigned long NSSDLIMIT;
unsigned long NSSDCLEAN = 1;
unsigned long WRITEAMPLIFICATION = 100;
unsigned long NCOLDBAND = 1;
unsigned long PERIODTIMES;
char smr_device[] = "/dev/sdc";
char ssd_device[] = "/mnt/ssd/ssd";

SSDEvictionStrategy EvictStrategy;
int BandOrBlock;

/*Block = 0, Band=1*/
int hdd_fd;
int ssd_fd;

unsigned long hit_num;
unsigned long read_hit_num;

unsigned long run_times;
unsigned long flush_times;

unsigned long load_ssd_blocks;
unsigned long load_hdd_blocks;
unsigned long flush_hdd_blocks;
unsigned long flush_ssd_blocks;

double time_read_ssd;
double time_read_hdd;
double time_write_ssd;
double time_write_hdd;

unsigned long hashmiss_sum;
unsigned long hashmiss_read;
unsigned long hashmiss_write;

SSDBufDespCtrl	*ssd_buf_desp_ctrl;
SSDBufDesp	    *ssd_buf_desps;

//SSDBufHashCtrl   *ssd_buf_hash_ctrl;
SSDBufHashBucket *ssd_buf_hashtable;
//SSDBufHashBucket *ssd_buf_hashdesps;

#ifdef SIMULATION
int 		    inner_ssd_fd;
SSDStrategyControl	*ssd_strategy_control;
SSDDesc		*ssd_descriptors;
SSDHashBucket	*ssd_hashtable;
unsigned long interval_time;
unsigned long read_smr_bands;
unsigned long flush_bands;
unsigned long flush_band_size;

pthread_mutex_t free_ssd_mutex;
pthread_mutex_t inner_ssd_hdr_mutex;
pthread_mutex_t inner_ssd_hash_mutex;
#endif // SIMULATION

/** Shared memory variable names **/
const char* SHM_SSDBUF_STRATEGY_CTRL = "SHM_SSDBUF_STRATEGY_CTRL";
const char* SHM_SSDBUF_STRATEGY_DESP = "SHM_SSDBUF_STRATEGY_DESP";

const char* SHM_SSDBUF_DESP_CTRL = "SHM_SSDBUF_DESP_CTRL";
const char* SHM_SSDBUF_DESPS = "SHM_SSDBUF_DESPS";

const char* SHM_SSDBUF_HASHTABLE_CTRL = "SHM_SSDBUF_HASHTABLE_CTRL";
const char* SHM_SSDBUF_HASHTABLE = "SHM_SSDBUF_HASHTABLE";
const char* SHM_SSDBUF_HASHDESPS =  "SHM_SSDBUF_HASHDESPS";
const char* SHM_PROCESS_REQ_LOCK = "SHM_PROCESS_REQ_LOCK";

#endif
