/*
 * =====================================================================================
 *
 *       Filename: layer5.h 
 *
 *    Description:  This file represents the Test application to test graph topology creation
 *
 *        Version:  1.0

 *       Revision:  1.0
 *       Compiler:  gcc
 *
 *         Author: Purbayan Majumder
 *        
 *        
.
 *        This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by  
 *        the Free Software Foundation, version 3.
 *
 *        This program is distributed in the hope that it will be useful, but 
 *        WITHOUT ANY WARRANTY; without even the implied warranty of 
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 *        General Public License for more details.
 *
 *        You should have received a copy of the GNU General Public License 
 *        along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#ifndef __LAYER5__
#define __LAYER5__

#include "../tcpconst.h"
#include "../tcpip_notif.h"

typedef struct node_ node_t;
typedef struct interface_ interface_t;

typedef struct pkt_notif_data_{

	node_t *recv_node;
	interface_t *recv_interface;
	char *pkt;
	uint32_t pkt_size;
	hdr_type_t hdr_code;
	int8_t return_code;
} pkt_notif_data_t;

void
promote_pkt_from_layer2_to_layer5(node_t *node,
					  interface_t *recv_intf,
        			  char *pkt,
					  uint32_t pkt_size,
					  hdr_type_t hdr_code);

void
promote_pkt_from_layer3_to_layer5(node_t *node,
					  interface_t *recv_intf,
        			  char *pkt,
					  uint32_t pkt_size,
					  hdr_type_t hdr_code);

void
tcp_stack_register_l2_pkt_trap_rule(
		node_t *node,
        nfc_pkt_trap pkt_trap_cb,
        nfc_app_cb app_cb);

void
tcp_stack_de_register_l2_pkt_trap_rule(
		node_t *node,
        nfc_pkt_trap pkt_trap_cb,
        nfc_app_cb app_cb);

#endif /* __LAYER5__ */
