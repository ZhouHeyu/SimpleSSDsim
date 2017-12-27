//
// Created by zhouheyu on 17-12-27.
//

#include "SSDsim.h"


SSDsim_t * SSDsim_initialize_SSDsim_structre()
{
    SSDsim_t *SSDsim;
    SSDsim=(SSDsim_t *)malloc(sizeof(SSDsim_t));
    if(SSDsim==NULL){
        fprintf(stderr,"malloc for SSDsim structre failed\n");
        exit(1);
    }
    //初始化内存段
    bzero((char *)SSDsim,sizeof(SSDsim_t));
    SSDsim->simtime=0.0;
    SSDsim->warmuptime=0.0;
    return (SSDsim);

}

void SSDsim_loadparams(char *filename)
{
    FILE   *parasfile;
    char line[201];
    char Stemp[200];
    int temp;


    if(strcmp(filename,"stdin")==0){
        parasfile=stdin;
    }else{
        if((parasfile=fopen(filename,"r"))==NULL){
            fprintf(stderr,"parasfile %s cannot be opened for read access\n",filename);
            exit(1);
        }
    }

    //一次按行读入，找到对应的匹配项，给对应的全局变量赋值

    while(fgets(line, 200,parasfile)!=NULL){
        if(2 != sscanf(line, "%s %d\n", Stemp, &temp)){
            fprintf(stderr,"Wrong happend in %s ,format is error\n",filename);
            exit(1);
        }else{
            if(strcmp(Stemp,"flash_numblocks")==0){
                flash_numblocks=temp;
            }else if(strcmp(Stemp,"flash_extrablocks")==0){
                flash_extrblocks=temp;
            }else if(strcmp(Stemp,"cache_type")==0){
                cache_type=temp;
            }else if(strcmp(Stemp,"ftl_type")==0){
                ftl_type=temp;
            }
        }
    }

    fclose(parasfile);
}


void SSDsim_setup_iotracefile(char *filename)
{
    if(strcmp(filename,"stdin")==0){
        SSDsim->iotracefile=stdin;
    }else{
        if((SSDsim->iotracefile=fopen(filename,"rb"))==NULL){
            fprintf(stderr,"Tracefile %s cannot be opened for read access\n",filename);
            exit(1);
        }
    }
    if(strlen(filename)<=255){
        strcpy(SSDsim->iotracefilename,filename);
    }else{
        fprintf(stderr,"Name of iotrace file is too long (>255Bytes):; checkpointing disabled\n");
        exit(1);
    }
}

void SSDsim_setup_outputfile(char *filename,char *mode)
{
    if (strcmp(filename, "stdout") == 0) {
        outputfile = stdout;
    } else {
        if ((outputfile = fopen(filename,mode)) == NULL) {
            fprintf(stderr, "Outfile %s cannot be opened for write access\n", filename);
            exit(1);
        }
    }

    if (strlen(filename) <= 255) {
         strcpy(SSDsim->outputfilename,filename);
         SSDsim->outputfile=outputfile;
    } else {
        fprintf (stderr, "Name of output file is too long (>255 bytes); checkpointing disabled\n");
    }
    setvbuf(outputfile, 0, _IONBF, 0);
}


void SSDsim_setup_SSDsim(int argc,char ** argv)
{
    if (argc < 4) {
        fprintf(stderr,"Usage: %s paramfile outfile iotrace ?\n", argv[0]);
        exit(1);
    }

    //配置输出文件
    SSDsim_setup_outputfile( argv[2],"w");
    fprintf (outputfile, "\n*** Output file name: %s\n", argv[2]);
    fflush (outputfile);
    //加载配置文件
    SSDsim_loadparams( argv[1]);
    fprintf(outputfile,"flash_numblocks= %d\n",flash_numblocks);
    fflush(outputfile);
    fprintf(outputfile,"flash_extrablocks= %d\n",flash_extrblocks);
    fflush(outputfile);
    fprintf(outputfile,"cache_type= %d\n",cache_type);
    fflush(outputfile);
    fprintf(outputfile,"ftl_type= %d\n",ftl_type);
    fflush(outputfile);
    //加载仿真的负载文件
    SSDsim_setup_iotracefile(argv[3]);
    fprintf (outputfile, "*** I/O trace used: %s\n", argv[3]);
    fflush (outputfile);

    fprintf(outputfile, "Initialization complete\n");
    fflush(outputfile);

}

void SSDsim_cleanup_and_printstats()
{
    fprintf(outputfile,"simulation complete\n");
    fflush(outputfile);

    //输出相应缓冲区的命中统计结果

    //flashsim: close and exit flash simulator
    endFlash();


    //关闭输出文件
    if(outputfile){
        fclose(outputfile);
    }
    //关闭输入的负载文件
    if(SSDsim->iotracefile){
        fclose(SSDsim->iotracefile);
    }

    //释放相应的内存段
    if(SSDsim){
        free(SSDsim);
    }
    if(ioreq){
        free(ioreq);
    }
}

void SSDsim_run_simulation()
{
    double delay=0.0;
    ioreq=(ioreq_event *)malloc(sizeof(ioreq_event));
    if(ioreq==NULL){
        fprintf(stderr,"malloc for ioreq is failed\n");
        exit(1);
    }
    memset(ioreq,0xFF,sizeof(ioreq_event));
    SSDsim->simtime=0.0;
    SSDsim->totalreqs=0;
    char buf[256];
    while (fgets(buf,255,SSDsim->iotracefile)!=NULL){
        if(5 != sscanf(buf, "%lf %d %d %d %d\n", &ioreq->time, &ioreq->devno, &ioreq->blkno, &ioreq->bcount, &ioreq->operation)){
            fprintf(stderr,"the I/O trace format is errror\n");
            exit(1);
        }
        //倒入自己的缓冲区代码段
        //test sentence
//        fprintf(stdout,"%d %d %d\n",ioreq->blkno,ioreq->bcount,ioreq->operation);
        delay=callFsim(ioreq->blkno,ioreq->bcount,ioreq->operation);
        SSDsim->simtime+=delay;
        fprintf(stdout,"LPN-%d Size-%d flag-%d \ttime is %lf\n",ioreq->blkno,ioreq->bcount,ioreq->operation,delay);
    }
}

//预热函数
void warmFlash(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if(fp==NULL){
        fprintf(stderr,"WarmFlash is failed | TraceFile %s cannot open read access \n",filename);
        exit(1);
    }
    char buffer[80];
    double time;
    int devno, bcount, flags;
    long int blkno;
    double delay;
    int i;

    while(fgets(buffer, sizeof(buffer), fp)){
        sscanf(buffer, "%lf %d %d %d %d\n", &time, &devno, &blkno, &bcount, &flags);

        bcount = ((blkno + bcount -1) / 4 - (blkno)/4 + 1) * 4;
        blkno /= 4;
        blkno *= 4;

        delay = callFsim(blkno, bcount, 0);

        //for(i = blkno; i<(blkno+bcount); i++){ dm_table[i] = DEV_FLASH; }
    }
    nand_stat_reset();

//    if(ftl_type == 3) opagemap_reset();
//
//    else if(ftl_type == 4) {
//        write_count = 0; read_count = 0; }

    fclose(fp);

}
