#include "../../tcp_public.h"
#include "isis_rtr.h"

bool isis_is_protocol_enable_on_node(node_t *node) {
    /* checks whether isis protocol is enabled or disabled  */
    isis_node_info_t *isis_node_info = ISIS_NODE_INFO(node);

    if (!isis_node_info) {
        return false;
    }

    return true;
}

void isis_show_node_protocol_state(node_t *node) {
    // printf("%s\n()",__FUNCTION__);
    printf("ISIS Protocol : %s\n", isis_is_protocol_enable_on_node(node) ? "ENABLE":"DISABLE");    
}
