#include "../../tcp_public.h"

#ifndef ISIS_RTR
#define ISIS_RTR

/* proposed data structures for router configuration holders */
typedef struct isis_node_info_ {

}isis_node_info_t;


/* macro to return node_t->node_nw_prop->isis_node_info ptr which is typecasted to isis_node_info_t type*/

/* fetching the isis_node infor from the node_nw_prop datastructure which holds the 
configuration for all the nodes at device levels and then getting typecasted */
#define ISIS_NODE_INFO(node_ptr) ((isis_node_info_t*)((node_ptr)->node_nw_prop.isis_node_info))

bool isis_is_protocol_enable_on_node(node_t *node);

#endif