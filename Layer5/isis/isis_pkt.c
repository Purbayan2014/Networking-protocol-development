/*
* @Author: purbayan2014
* @Date:   2023-01-10 22:47:42
* @Last Modified 2023-01-10
* @Last Modified time: 2023-01-10 23:09:04
*/

# include "../../tcp_public.h"
#include "isis_intf.h"
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

byte *isis_prepare_hello_pkt(interface_t *intf, size_t *hello_pkt_size) {

	/*
	 *  The architecture has been manufactured with reference to the hello packet 
	 *  diagram 
	 * */
	isis_pkt_hdr_t *hello_pkt_hdr;
	/* total size required by the isis hello packet */
	/* considering the size of the payload because it considers the size of the payload 
	 * as well as the ethernet packet header */
	uint32_t eth_hdr_payload_size = sizeof(isis_pkt_hdr_t) + 
		/* AS 6 tlv are present we need to consider the total over head size of the six tlv*/
		(TLV_OVERHEAD_SIZE * 6) +
		NODE_NAME_SIZE + /* Length of the first tlv */
		/* Now considerding the data type length for each of the tlv */
		4 + /* type loopback address*/ 
		4 + /* type ip address */
		4 + /* type if index */
		4 + /* type hold time */
		4 ; /* type cost value */

	/* hello packet size = size of the ethernet header + payload size */
	*hello_pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD  /* dest mac + source mac + type field + fcfs (but without the payload) */
		+ eth_hdr_payload_size;
	
	/* allocate the new packet memory */
		/* typecastting to ethernet header as the packet starts with ethernet header */
	ethernet_hdr_t *hello_eth_hdr = (ethernet_hdr_t*)tcp_ip_get_new_pkt_buffer(*hello_pkt_size);

	/* populating the entire the memory chunk */
	
	layer2_fill_with_broadcast_mac(hello_eth_hdr->dst_mac.mac); // destination mac
	memset(hello_eth_hdr->src_mac.mac, 0, sizeof(mac_add_t)); // source mac address
	hello_eth_hdr->type = ISIS_ETH_PKT_TYPE; // type field of the ethernet hdr
	
	
	/* getting a pointer to the payload of the ethernet header to manipulate the data in the isis_pkt_hdr*/
	hello_pkt_hdr = (isis_pkt_hdr_t *)GET_ETHERNET_HDR_PAYLOAD(hello_eth_hdr);

	hello_pkt_hdr->isis_pkt_type = ISIS_PTP_HELLO_PKT_TYPE;
	hello_pkt_hdr->seq_no = 0; /* as it is not required */
	/* fetching the router's loop back address and then returning it into a integer format */
	hello_pkt_hdr->rtr_id = tcp_ip_covert_ip_p_to_n(NODE_LO_ADDR(intf->att_node));
	hello_pkt_hdr->flags = 0;
	

	/* filling the TLV's one by one */
	
	byte *temp; /* getting the pointer after the end of the grey region */
	temp = (byte *)(hello_pkt_hdr + 1);

	temp = tlv_buffer_insert_tlv(temp, // ptr
			ISIS_TLV_HOSTNAME,  // type
			NODE_NAME_SIZE, // size
			intf->att_node->node_name); // value
	
	temp = tlv_buffer_insert_tlv(temp, 
			ISIS_TLV_RTR_ID,
			4,
			(byte *)(&hello_pkt_hdr->rtr_id));

	/* fetching the ip address in the integer format */
	uint32_t ip_addr_intf = tcp_ip_covert_ip_p_to_n(IF_IP(intf));
	temp = tlv_buffer_insert_tlv(temp,
			ISIS_TLV_IF_IP,
			4,
			(byte *)&ip_addr_intf);

	temp = tlv_buffer_insert_tlv(temp, 
			ISIS_TLV_IF_INDEX,
			4,
			(byte *)(&IF_INDEX(intf)));

	/* calculating the hold time*/
	uint32_t hold_time = ISIS_INTF_HELLO_INTERVAL(intf) * ISIS_HOLD_TIME_FACTOR;


	temp = tlv_buffer_insert_tlv(temp,
			ISIS_TLV_HOLD_TIME,
			4,
			(byte *)&hold_time);

	uint32_t cost = ISIS_INTF_COST(intf);
	temp = tlv_buffer_insert_tlv(temp,
			ISIS_TLV_METRIC_VAL,
			4,
			(byte *)&cost);

	/* set the fcfs to 0*/
	SET_COMMON_ETH_FCS(hello_eth_hdr, eth_hdr_payload_size, 0);

	return (byte *)&hello_eth_hdr;
}
