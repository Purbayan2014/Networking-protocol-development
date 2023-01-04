/*
 * =====================================================================================
 *
 *       Filename:  uapi_mm.h
 *
 *    Description:  This Header file ocntains public APIs to be used by the application
 *
 *        Version:  1.0
 *
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Purbayan Majumder, purbayan2014@gmail.com
 *        
 *
 *        This file is part of the Linux Memory Manager distribution ( 
 *        Copyright (c) Purbayan Majumder
 *        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
 *        Public License as published by the Free Software Foundation, version 3.
 *        
 *        This program is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *        General Public License for more details.
 *
 *        
 *                                  
 * =====================================================================================
 */

#ifndef __UAPI_MM__
#define __UAPI_MM__

#include <stdint.h>
typedef struct mm_instance_ mm_instance_t;

void *
xcalloc(mm_instance_t *mm_inst, char *struct_name, int units);
void *
xcalloc_buff(mm_instance_t *mm_inst, uint32_t bytes) ;
void
xfree(void *app_ptr);

/*Printing Functions*/
void mm_print_memory_usage(mm_instance_t *mm_inst,  char *struct_name);
void mm_print_block_usage(mm_instance_t *mm_inst);
void mm_print_registered_page_families(mm_instance_t *mm_inst);
void mm_print_variable_buffers(mm_instance_t *mm_inst);

/*Initialization Functions*/
void
mm_init();

mm_instance_t *
mm_init_new_instance();

/*Registration function*/
void
mm_instantiate_new_page_family(
        mm_instance_t *mm_inst,
        char *struct_name,
        uint32_t struct_size);

#define XCALLOC(mm_inst, units, struct_name) \
    (calloc(units, sizeof(struct_name)))

#define XCALLOC_BUFF(mm_inst, size_in_bytes) \
    (calloc(1, size_in_bytes))

#define MM_REG_STRUCT(mm_inst, struct_name)  \
    (mm_instantiate_new_page_family(mm_inst, #struct_name, sizeof(struct_name)))

#define XFREE(ptr)  \
   free(ptr)

#endif /* __UAPI_MM__ */
