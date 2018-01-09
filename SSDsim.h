//
// Created by zhouheyu on 17-12-27.
//

#ifndef SIMULATION_SSDSIM_H
#define SIMULATION_SSDSIM_H

#include "global.h"
#include "global.h"
#include "Interface.h"
#include "flash.h"
#include "Cache.h"

SSDsim_t * SSDsim_initialize_SSDsim_structre();
void SSDsim_setup_iotracefile(char *filename);
void SSDsim_loadparams(char *filename);
void SSDsim_setup_outputfile(char *filename,char *mode);
void SSDsim_setup_SSDsim(int argc,char ** argv);
void SSDsim_cleanup_and_printstats();
void SSDsim_run_simulation();
void warmFlash(char *filename);
#endif //SIMULATION_SSDSIM_H
