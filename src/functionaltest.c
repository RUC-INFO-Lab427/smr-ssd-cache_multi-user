#include "losertree4pore.h"
#include "pore.h"


void
FunctionalTest()
{
    loserTreeTest();
}
 void loserTreeTest()
 {
    StrategyDesp_pore initDesp[5];
    initDesp[0].stamp = 1;
    initDesp[1].stamp = 2;
    initDesp[2].stamp = 3;
    initDesp[3].stamp = 4;
    initDesp[4].stamp = 5;

    initDesp[0].serial_id = 1;
    initDesp[1].serial_id = 2;
    initDesp[2].serial_id = 3;
    initDesp[3].serial_id = 4;
    initDesp[4].serial_id = 5;

    int winnerPath, winnerDesp,winnerVal;

    void* passport;
    winnerVal = LoserTree_Create(5,initDesp,50,&passport,&winnerPath,&winnerDesp);
    printf("winnerPath = %d, winnerDesp = %d winnerVal = %d\n", winnerPath, winnerDesp,winnerVal);

    int id = 1;
    while(1){
        StrategyDesp_pore* newdesp = (StrategyDesp_pore*)malloc(sizeof(StrategyDesp_pore));
        int c;
        scanf("%d",&c);
        printf("c = %d\n",c);
        newdesp->serial_id = id;
        newdesp->stamp = c;
        winnerVal = LoserTree_GetWinner(passport,newdesp,&winnerPath,&winnerDesp);
        printf("winnerPath = %d, winnerDesp = %d winnerVal = %d\n", winnerPath, winnerDesp,winnerVal);
    }
 }
