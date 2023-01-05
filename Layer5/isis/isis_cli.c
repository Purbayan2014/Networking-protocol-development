#include <assert.h>
#include "../../tcp_public.h"
#include "isis_cmdcodes.h"
#include "isis_rtr.h"
#include "isis_intf.h"

static void isis_init(node_t *node) {
      /* init the node */
      printf("%s()/n",__FUNCTION__);
      isis_node_info_t *isis_node_info = ISIS_NODE_INFO(node);
      if (isis_node_info) return;
      // ISIS_NODE_INFO(node) = isis_node_info;
      isis_node_info = calloc(1, sizeof(isis_node_info_t));
      node->node_nw_prop.isis_node_info = isis_node_info;
}

static void isis_deinit(node_t *node) {
    /* deinit the node */
    printf("%s()/n",__FUNCTION__);
    isis_node_info_t *isis_node_info = ISIS_NODE_INFO(node);
    if (!isis_node_info) return;
    free(isis_node_info);
    // ISIS_NODE_INFO(node) = NULL;
    node->node_nw_prop.isis_node_info = NULL; 
}

/*show node <node-name> proto isis */
static int isis_show_handler(param_t *param, ser_buff_t *tlv_buf,
                                    op_mode enable_or_disable) {
       printf("%s()/n",__FUNCTION__);
       int cmd_codes = -1;
       cmd_codes = EXTRACT_CMD_CODE(tlv_buf); /* the tlv buffer shares the data passed by the user 
                which is used to obtain the command codes based on it*/
        /* cmd_codes == ISIS_CONFIG_MODE_ENABLE */

        /*extracting the node-name*/
        tlv_struct_t *tlv = NULL; /* used as a iterator */
        char *node_name = NULL; /* pointer to the node_name */

        TLV_LOOP_BEGIN(tlv_buf,tlv){

            /* compare the strings */
            if(strncmp(tlv->leaf_id, "node-name",strlen("node-name"))==0){
                node_name = tlv->value;
            }else {
                assert(0);
            }

        }TLV_LOOP_END;


        /* getting the value of the node by the node_name */
        node_t *node;
        node = node_get_node_by_name(topo, node_name); /* topo is a global variable
                        representing the topology
                        node_name is the name of the node  */
        
        /* positve or negative command codes */
        switch(cmd_codes) {
            /* passing the global first */
            case CMDCODE_SHOW_NODE_ISIS_PROTOCOL:
                isis_show_node_protocol_state(node);
                /* iterating through all the interfaces of the router and checking which are enabled */
                interface_t *interface;
                ITERATE_NODE_INTERFACES_BEGIN(node, interface) {
                    isis_show_enabled_interfaces(interface);
                } ITERATE_NODE_INTERFACES_END(node, interface);
                break;
                default:;
        }                   
                                
    return 0;        
                                    }

/* conf <node-name> protocol isis */
static int isis_config_handler(param_t *param, ser_buff_t *tlv_buf,
                                    op_mode enable_or_disable) {
        printf("%s()/n",__FUNCTION__);
        /*
            Various command can invoke the same backend 
            handler but the command helps to differenctiate between 
            the various codes that gets executed

                > Extract the command codes and 
                > And use it later to implement various clis
        */

       int cmd_codes = -1;
       cmd_codes = EXTRACT_CMD_CODE(tlv_buf); /* the tlv buffer shares the data passed by the user 
                which is used to obtain the command codes based on it*/
        /* cmd_codes == ISIS_CONFIG_MODE_ENABLE */

        /*extracting the node-name*/
        tlv_struct_t *tlv = NULL; /* used as a iterator */
        char *node_name = NULL; /* pointer to the node_name */

        TLV_LOOP_BEGIN(tlv_buf,tlv){

            /* compare the strings */
            if(strncmp(tlv->leaf_id, "node-name",strlen("node-name"))==0){
                node_name = tlv->value;
            }else {
                assert(0);
            }

        }TLV_LOOP_END;


        /* getting the value of the node by the node_name */
        node_t *node;
        node = node_get_node_by_name(topo, node_name); /* topo is a global variable
                        representing the topology
                        node_name is the name of the node  */
        
        /* positve or negative command codes */
        switch(cmd_codes) {
            /* passing the global first */
            case ISIS_CONFIG_NODE_ENABLE:
                switch(enable_or_disable) {
                    case CONFIG_ENABLE:
                        isis_init(node);
                        break;
                    case CONFIG_DISABLE:
                        isis_deinit(node);
                        break;
                    default:;
                }
        }

       return 0;
}

/* conf node <node-name> proto isis interface all*/
static int isis_intf_config_handler(param_t *param, ser_buff_t *tlv_buf,
                                    op_mode enable_or_disable) {
        printf("%s()\n",__FUNCTION__);
        /*
            Various command can invoke the same backend 
            handler but the command helps to differenctiate between 
            the various codes that gets executed

                > Extract the command codes and 
                > And use it later to implement various clis
        */

       int cmd_codes = -1;
       cmd_codes = EXTRACT_CMD_CODE(tlv_buf); /* the tlv buffer shares the data passed by the user 
                which is used to obtain the command codes based on it*/
        /* cmd_codes == ISIS_CONFIG_MODE_ENABLE */

        /*extracting the node-name*/
        tlv_struct_t *tlv = NULL; /* used as a iterator */
        char *node_name = NULL; /* pointer to the node_name */
        char *if_name; /*pointer to interface name */
        interface_t *interface = NULL; /* pointer to the interface */

        TLV_LOOP_BEGIN(tlv_buf,tlv){

            /* compare the strings */
            if(strncmp(tlv->leaf_id, "node-name",strlen("node-name"))==0){
                node_name = tlv->value;
            } else if(strncmp(tlv->leaf_id, "if-name", strlen("if-name"))==0){
                if_name = tlv->value;
            }
            else {
                assert(0);
            }

        }TLV_LOOP_END;


        /* getting the value of the node by the node_name */
        node_t *node;
        node = node_get_node_by_name(topo, node_name); /* topo is a global variable
                        representing the topology
                        node_name is the name of the node  */
        


        

        /* positve or negative command codes */
        switch(cmd_codes) {
            /* passing the global first */
            case CMDCODE_C0NF_NODE_ISIS_PROTO_INTF_ALL_ENABLE:
                switch(enable_or_disable) {

                    /* config node <node-name> proto isis interface all */
                    case CONFIG_ENABLE:
                        printf("\nconf node %s proto isis interface all ", node_name);
                        /* iterating over all the interfaces */
                        ITERATE_NODE_INTERFACES_BEGIN(node, interface) {
                            isis_enable_protocol_on_interface(interface);
                        } ITERATE_NODE_INTERFACES_END(node, intterface)
                        break;

                    /* conf node <node-name> [no] proto isis interface all*/   
                    case CONFIG_DISABLE:
                        printf("\nconf node %s [no] proto isis interface all\n", node_name);
                        ITERATE_NODE_INTERFACES_BEGIN(node, interface) {
                            isis_disable_protocol_on_interface(interface);
                        }ITERATE_NODE_INTERFACES_END(node, interface);             
                        break;
                    default:;
                }
                break;
                case CMDCODE_CONF_NODE_ISIS_PROTOT_INTF_ENABLE:
                /* getting the interface by the interface name */
                interface = node_get_intf_by_name(node, if_name);
                if(!interface) { printf(" Error : Interface doesnt exists\n"); return -1; }
                switch(enable_or_disable) {

                    case CONFIG_ENABLE:
                        isis_enable_protocol_on_interface(interface);
                        break;

                    case CONFIG_DISABLE:
                        isis_disable_protocol_on_interface(interface);
                        break;
                    
                    default:;
                }
        }

       return 0;
}


int isis_config_cli_tree(param_t *param) {

    {
            // cli intergration protocol hiearchy
            // conf node <node-name> proto isis
            static param_t isis_proto;
            init_param(
                &isis_proto, /* address of the current param*/
                CMD, /* CMD for command param and LEAF for leaf param */
                "isis", /* name of the param for getting displayed in the command line */
                isis_config_handler, /* callback function :: pointers to the application routine  */
                0, /* Also a application specific function which is used for validating data from the user */
                INVALID, /* data type for the leaf node always null for CMD params */
                0, /* name of the leaf param that is used for parsing the application code to find the value 
                        passed by the user */
                "isis_protocol" /* Used as help string*/
            );

            // register the parameter
            /*The isis_proto becomes the leaf of the parent param
                Here the param indicates the command protocol*/
            libcli_register_param(param, &isis_proto);

            // setting the command code
            /* The command code here is used to sent to the the command code to the callback function
                which is used to trigger different types of logics in the background */
            set_param_cmd_code(&isis_proto,ISIS_CONFIG_NODE_ENABLE);


            {
                /* config node <node-name> proto isis interface */
                static param_t interface; // interface is a new keyword which is the child of the isis
                init_param(
                    &interface,
                    CMD,
                    "interface",
                    0,  /* incomplete command so no callback handler required */
                    0,
                    INVALID,
                    0,
                    "interface");

                    libcli_register_param(&isis_proto, &interface);
                        
                        {
                               /* conf node <node-name> proto isis interface all */
                               static param_t all;
                               init_param(
                                &all,
                                CMD,
                                "all",
                                isis_intf_config_handler,
                                0,
                                INVALID,
                                0,
                                "all interface");

                                libcli_register_param(&interface, &all);
                                set_param_cmd_code(&all, CMDCODE_C0NF_NODE_ISIS_PROTO_INTF_ALL_ENABLE);
                        }
                            /* conf node <node-name> proto isis interface <intf> */
                        {
                                static param_t if_name;
                                init_param(
                                    &if_name,
                                    LEAF,
                                    0,
                                    isis_intf_config_handler,
                                    0,
                                    STRING,
                                    "if-name", /* this will be used in the callback handler to extract the info about the specific interfaces*/
                                    "interface-name" 
                                );
                                
                                libcli_register_param(&interface, &if_name);
                                set_param_cmd_code(&if_name, CMDCODE_CONF_NODE_ISIS_PROTOT_INTF_ENABLE);

                        }
            }
    }  

    return 0;  
}


/* show node <node-name> proto*/
int isis_show_cli_tree(param_t *param) {

    {
            // cli intergration protocol hiearchy
            static param_t isis_proto;
            init_param(
                &isis_proto, /* address of the current param*/
                CMD, /* CMD for command param and LEAF for leaf param */
                "isis", /* name of the param for getting displayed in the command line */
                isis_show_handler, /* callback function :: pointers to the application routine  */
                0, /* Also a application specific function which is used for validating data from the user */
                INVALID, /* data type for the leaf node always null for CMD params */
                0, /* name of the leaf param that is used for parsing the application code to find the value 
                        passed by the user */
                "isis_protocol" /* Used as help string*/
            );

            // register the parameter
            /*The isis_proto becomes the leaf of the parent param
                Here the param indicates the command protocol*/
            libcli_register_param(param, &isis_proto);

            // setting the command code
            /* The command code here is used to sent to the the command code to the callback function
                which is used to trigger different types of logics in the background */
            set_param_cmd_code(&isis_proto,CMDCODE_SHOW_NODE_ISIS_PROTOCOL);
    }  

    return 0;  
}