/*
 * main.c
 */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "global.h"
#include "ssd-cache.h"
//#include "smr-simulator/smr-simulator.h"
#include "trace2call.h"

unsigned int INIT_PROCESS = 0;

//void* argvparse(char** argv)
//{
//    if (argc == 9) {
//		NBLOCK_SSD_CACHE = atoi(argv[1]);
//		NTABLE_SSD_CACHE = atoi(argv[1]);
//		NSSDs = atoi(argv[2]);
//		NSSDTables = atoi(argv[2]);
//		NSSDLIMIT = atoi(argv[2]);
//		BandOrBlock = atoi(argv[3]);
//		SSD_BUFFER_SIZE = atoi(argv[4]);
//		ZONESZ = atoi(argv[5]);
//		PERIODTIMES = atoi(argv[6]);
//		if (atoi(argv[7]) == 1)
//			EvictStrategy = LRU;
//		if (atoi(argv[7]) == 2)
//			EvictStrategy = LRUofBand;
//		if (atoi(argv[7]) == 3)
//			EvictStrategy = Most;
//		if (atoi(argv[7]) == 4)
//			EvictStrategy = CMR;
//		if (atoi(argv[7]) == 5)
//			EvictStrategy = SMR;
//		if (atoi(argv[7]) == 6)
//			EvictStrategy = MaxCold;
//		if (atoi(argv[7]) == 7)
//			EvictStrategy = MaxAll;
//		if (atoi(argv[7]) == 8)
//			EvictStrategy = AvgBandHot;
//		if (atoi(argv[7]) == 9)
//			EvictStrategy = HotDivSize;
//	} else {
//		printf("parameters are wrong %d\n", argc);
//		exit(-1);
//	}
//
//}

int
main(int argc, char** argv)
{
    int isWriteOnly;
    int traceID;
    off_t startLBA;
    if(argc == 7)
    {
        NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = atoi(argv[1]);
        isWriteOnly = atoi(argv[2]);
        traceID = atoi(argv[3]);
        startLBA = atol(argv[4]);
        MaxSSD = atol(argv[5]);
	UserId = atoi(argv[6]);
    }
    else
    {
        printf("parameters are wrong %d\n", argc);
        exit(-1);
    }
#ifdef SIMULATION
    initFIFOCache();
    inner_ssd_fd = open(inner_ssd_device, O_RDWR | O_DIRECT);
#endif
    //NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = 500000;//280M //50000; // 200MB

    SSD_BUFFER_SIZE = 4096;
    EvictStrategy = LRU;

    initSSD();
    hdd_fd = open(smr_device, O_RDWR | O_DIRECT);
    ssd_fd = open(ssd_device, O_RDWR | O_DIRECT);

    char* tracefile[] = {"/home/trace/src1_2.csv.req","/home/trace/wdev_0.csv.req"};
    trace_to_iocall(tracefile[traceID],isWriteOnly,startLBA);
    close(hdd_fd);
    close(ssd_fd);

#ifdef SIMULATION
    close(inner_ssd_fd);
#endif
    return 0;
//	void
//parse_args(int argc, char ** argv, param_t * cmd_params)
//{
//
//    int c, i, found;
//    /* Flag set by ‘--verbose’. */
//    static int verbose_flag;
//    static int nonunique_flag;
//    static int fullrange_flag;
//    static int basic_numa;
//
//    while(1) {
//        static struct option long_options[] =
//            {
//                {"help",       no_argument,    0, 'h'},
//                {"version",    no_argument,    0, 'v'},
//                {"i-am-init-process",no_argument,0,'I' },
//                {"nblock-SSD",required_argument, 0, 'c'},
//                {"nblock-HDD",required_argument, 0, 'm'},
//                {"trace",required_argument, 0, 't'},
//                {"strategy",required_argument,0,'s'},
//                {0, 0, 0, 0}
//            };
//        /* getopt_long stores the option index here. */
//        int option_index = 0;
//
//        c = getopt_long (argc, argv, "c:m:t:s:hv",
//                         long_options, &option_index);
//
//        /* Detect the end of the options. */
//        if (c == -1)
//            break;
//        switch (c)
//        {
//          case 0:
//              /* If this option set a flag, do nothing else now. */
//              if (long_options[option_index].flag != 0)
//                  break;
//              printf ("option %s", long_options[option_index].name);
//              if (optarg)
//                  printf (" with arg %s", optarg);
//              printf ("\n");
//              break;
//          case 'h':
//          case '?':
//              /* getopt_long already printed an error message. */
//              print_help(argv[0]);
//              exit(EXIT_SUCCESS);
//              break;
//          case 'I':
//                INIT_PROCESS = 1;
//                break;
//          case 'c':
//            NBLOCK_SSD_CACHE
//          case 'm':
//          case 't':
//          case 's':
//          default:
//              break;
//        }
//    }
//
//    /* if (verbose_flag) */
//    /*     printf ("verbose flag is set \n"); */
//
//    cmd_params->nonunique_keys = nonunique_flag;
//    cmd_params->verbose        = verbose_flag;
//    cmd_params->fullrange_keys = fullrange_flag;
//    cmd_params->basic_numa     = basic_numa;
//
//    /* Print any remaining command line arguments (not options). */
//    if (optind < argc) {
//        printf ("non-option arguments: ");
//        while (optind < argc)
//            printf ("%s ", argv[optind++]);
//        printf ("\n");
//    }
//}
}


