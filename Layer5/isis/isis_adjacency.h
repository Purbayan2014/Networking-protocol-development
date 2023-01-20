#ifndef __IGP_NBRSHIP__
#define __IGP_NBRSHIP__

typedef enum isis_adj_state_ {

    ISIS_ADJ_STATE_UNKNOWN,
    ISIS_ADJ_STATE_DOWN,
    ISIS_ADJ_STATE_INIT,
    ISIS_ADJ_STATE_UP
} isis_adj_state_t;

/* inline function to convert the adjacency states from down , init and up state*/
static inline char *
isis_adj_state_str(isis_adj_state_t adj_state) {

    switch(adj_state){
        case ISIS_ADJ_STATE_DOWN:
            return "Down";
        case ISIS_ADJ_STATE_INIT:
            return "Init";
        case ISIS_ADJ_STATE_UP:
            return "Up";
        default : ;
    }
    return NULL;
}

typedef struct isis_adjacency_{

    /* back ptr to the the interface */
    interface_t *intf; 
    /* nbr Device Name */
    unsigned char nbr_name[NODE_NAME_SIZE];
    /* Nbr intf Ip */
    uint32_t nbr_intf_ip;
    /* Mac Address */
     mac_add_t nbr_mac;
    /*Nbr lo 0 address */
    uint32_t nbr_rtr_id;
    /* Nbr if index */
    uint32_t remote_if_index;
    /* Adj State */
    isis_adj_state_t adj_state;
    /* timestamp when Adj state changed */
    time_t last_transition_time;
    /* Hold time in sec reported by nbr*/
    uint32_t hold_time;
    /* Nbr link cost Value */
    uint32_t cost; 
    /* Expiry timer */
    timer_event_handle *expiry_timer;
    /* Delete timer */
    timer_event_handle *delete_timer;
    /* uptime */
    time_t uptime;
} isis_adjacency_t;

#endif
