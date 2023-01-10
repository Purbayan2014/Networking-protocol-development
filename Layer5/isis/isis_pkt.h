
# ifndef __ISIS_PKT__
# define __ISIS_PKT__ 


// adding the packet classification rule 
/* bool function checks if the packet is an isis packet or not */
/* pkt is the pointer to the ethernet header */
bool isis_pkt_trap_rule(char *pkt, size_t pkt_size);
/* pushes the packet into the protocol for processing */
/* the first arg will be the start of the ethernet header */
/* the second arg will be the total size of the ethernet header */
void isis_pkt_recieve(void *arg, size_t arg_size);

# endif