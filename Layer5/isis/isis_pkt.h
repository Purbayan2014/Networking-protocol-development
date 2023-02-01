
# ifndef __ISIS_PKT__
# define __ISIS_PKT__ 
#include "isis_const.h"
typedef uint16_t isis_pkt_type_t;

typedef struct isis_pkt_ {

    /* The pkt type - Hellos or LSPs */
    isis_pkt_type_t isis_pkt_type;
    /* The wired form of pkt */
    byte *pkt;
    /* pkt size, including eithernet hdr */
    size_t pkt_size;
    /* Rest of the below fields are usually used in the
    context of LSPs only */

    /* ref count on this pkt */
    uint16_t ref_count;
    /* No of interfaces out of which LSP has been
    Queued to xmit */
    uint16_t flood_queue_count;
    /* if set to false, this LSP would not xmit out */
    bool flood_eligibility;
    /* glue to attach this lsp pkt to lspdb*/
    avltree_node_t avl_node_glue;
    /* Life time timer */
    timer_event_handle *expiry_timer;
    /* to check if this LSP is present in lspdb or not */
    bool installed_in_db;
} isis_pkt_t;

typedef struct isis_pkt_hdr_{

    isis_pkt_type_t isis_pkt_type;
    uint32_t seq_no; /* meaningful only for LSPs */
    uint32_t rtr_id;
    isis_pkt_hdr_flags_t flags;
} isis_pkt_hdr_t;


// adding the packet classification rule 
/* bool function checks if the packet is an isis packet or not */
/* pkt is the pointer to the ethernet header */
bool isis_pkt_trap_rule(char *pkt, size_t pkt_size);
/* pushes the packet into the protocol for processing */
/* the first arg will be the start of the ethernet header */
/* the second arg will be the total size of the ethernet header */
void isis_pkt_recieve(void *arg, size_t arg_size);

byte *isis_prepare_hello_pkt(interface_t *intf, size_t *hello_pkt_size);
# endif
