cmake_minimum_required(VERSION 3.9)
project(simulation)

set(CMAKE_CXX_STANDARD 11)

add_executable(simulation
        main.c
        flash.c
        flash.h
        fast.c
        fast.h
        type.h global.h SSDsim.c SSDsim.h Interface.c Interface.h)
