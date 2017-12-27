//
// Created by zhouheyu on 17-12-27.
//

#ifndef SIMULATION_SSDSIM_H
#define SIMULATION_SSDSIM_H

#include "global.h"

SSDsim_t * SSDsim_initialize_SSDsim_structre();
void SSDsim_setup_iotracefile(char *filename);
void SSDsim_loadparams(char *filename);
void SSDsim_setup_outputfile(char *filename,char *mode);
void SSDsim_setup_SSDsim(int argc,char ** argv);
void SSDsim_cleanup_and_printstats();
void SSDsim_run_simulation();
#endif //SIMULATION_SSDSIM_H
