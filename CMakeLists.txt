cmake_minimum_required(VERSION 3.9)
project(simulation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-m32")
set(CMAKE_SHARED_LINKER_FLAG "-m32")

add_executable(simulation
        main.c
        flash.c
        dftl.c
        dftl.h
        flash.h
        fast.c
        fast.h
        type.h global.h SSDsim.c SSDsim.h Interface.c Interface.h Cache.c Cache.h LRU.c LRU.h  CFLRU.h CFLRU.c  List.c List.h
        CASA.c CASA.h BlkList.c BlkList.h FAB.c FAB.h
        ADLRU.h ADLRU.c BPLRU.c BPLRU.h)
