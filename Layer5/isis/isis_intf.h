#ifndef ISIS_INTF
#define ISIS_INTF

/* macros for packet interval and the cost */

#define ISIS_INTF_COST(intf_ptr) \
        (((isis_intf_info_t *)((intf_ptr)->intf_nw_props.isis_intf_info))->cost)
#define ISIS_INTF_HELLO_INTERVAL(intf_ptr) \
        (((isis_intf_info_t *)((intf_ptr)->intf_nw_props.isis_intf_info))->hello_interval)

/* proposed data structure for isis interface configuration holders */

typedef struct isis_intf_info_ {
   uint16_t hello_interval; // time interval for sending periodic hello packets
   uint32_t cost; // cost for each interface 
} isis_intf_info_t;

/* fetching the isis_node info from the intf_nw_prop datastructure which holds the 
configuration for all the nodes at interface levels and then getting typecasted */
#define ISIS_INTF_INFO(intf_ptr) ((isis_intf_info_t*)((intf_ptr)->intf_nw_props.isis_intf_info))

bool isis_node_intf_is_enable(interface_t *intf);
void isis_enable_protocol_on_interface(interface_t *intf);
void isis_disable_protocol_on_interface(interface_t *intf);
void isis_show_enabled_interfaces(interface_t *intf);


#endif
