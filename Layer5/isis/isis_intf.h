// #include "../../tcp_public.h"
#ifndef ISIS_INTF
#define ISIS_INTF

/* proposed data structure for isis interface configuration holders */

typedef struct isis_intf_info_ {

} isis_intf_info_t;

/* fetching the isis_node info from the intf_nw_prop datastructure which holds the 
configuration for all the nodes at interface levels and then getting typecasted */
#define ISIS_INTF_INFO(intf_ptr) ((isis_intf_info_t*)((intf_ptr)->intf_nw_props.isis_intf_info))

bool isis_node_intf_is_enable(interface_t *intf);


#endif 