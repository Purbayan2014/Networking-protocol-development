/*
* @Author: purbayan2014
* @Date:   2023-01-10 22:47:42
* @Last Modified 2023-01-10
* @Last Modified time: 2023-01-10 23:09:04
*/

# include "../../tcp_public.h"
# include "isis_pkt.h"
# include "isis_const.h"

// adding the packet classification rule 
/* bool function checks if the packet is an isis packet or not */
/* pkt is the pointer to the ethernet header */
bool isis_pkt_trap_rule(char *pkt, size_t pkt_size) {

	// as it is a pointer of the packet so we need to typecast it to the ethernet header
	ethernet_hdr_t *eth_hdr = (ethernet_hdr_t*)pkt;
	return eth_hdr->type == ISIS_ETH_PKT_TYPE;
}

/* pushes the packet into the protocol for processing */
/* the first arg will be the start of the ethernet header */
/* the second arg will be the total size of the ethernet header */
void isis_pkt_recieve(void *arg, size_t arg_size) {
	printf("%s()/n",__FUNCTION__);
}