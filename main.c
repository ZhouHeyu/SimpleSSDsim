#include "global.h"
#include "SSDsim.h"


int main(int argc ,char **argv)
{
	//根据输入的参数，处理仿真的环境
    SSDsim=SSDsim_initialize_SSDsim_structre();
    SSDsim_setup_SSDsim(argc,argv);

    //初始化相应的FTL,flash配置
    if(ftl_type!=-1){
//        initFlash();
//        reset_flash_stat();
//        nand_stat_reset();
    }

    //预热，使nand数组存在相应的数据
//    warmFlash(argv[3]);

//    开始仿真运行
     SSDsim_run_simulation();


    //结束仿真，输出结果
    SSDsim_cleanup_and_printstats();
	return 0;
}


