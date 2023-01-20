#include "../../tcp_public.h"
#include "isis_intf.h"
#include "isis_rtr.h"
#include "isis_const.h"

bool isis_node_intf_is_enable(interface_t *intf) {
    return !(intf->intf_nw_props.isis_intf_info == NULL);
}

static void isis_init_isis_intf_info(interface_t *intf) {
    /*
     *  Initialize the isis_interface object that is being pointed by the interface_t
     * */

    isis_intf_info_t *isis_intf_info = ISIS_INTF_INFO(intf);
    memset(isis_intf_info, 0, sizeof(isis_intf_info_t));
    isis_intf_info->hello_interval = ISIS_DEFAULT_HELLO_INTERVAL;
    isis_intf_info->cost = ISIS_DEFAULT_INTF_COST;
}


void isis_enable_protocol_on_interface(interface_t *intf) {
    /* 
        > only enable at the node level 
        > else through an error
        > if protocol enabled do nothing
        > finally enable the protocol on the interface
      */

     // getting the pointer to the attached node of the interface
     // then fetching the information

     isis_intf_info_t *intf_info = NULL;
     if(ISIS_NODE_INFO(intf->att_node)==NULL){
        printf("Error: Enable Protocol on the node first \n");
        return;
     }

     intf_info = ISIS_INTF_INFO(intf);
     if(intf_info) {return;}
     intf_info = calloc(1, sizeof(isis_intf_info_t));
     intf->intf_nw_props.isis_intf_info = intf_info;
     isis_init_isis_intf_info(intf);
}

void isis_disable_protocol_on_interface(interface_t *intf) {
    
    isis_intf_info_t *intf_info = NULL;
    intf_info = ISIS_INTF_INFO(intf);
    if(!intf_info) {return;}
    free(intf_info);
    intf->intf_nw_props.isis_intf_info = NULL;
}

void isis_show_enabled_interfaces(interface_t *intf) {
    printf("\n");
    if(isis_node_intf_is_enable(intf)) {
        printf("%s : ENABLED", intf->if_name);
    }

    printf("\n");

}
