#include <assert.h>
#include "../../tcp_public.h"
#include "isis_cmdcodes.h"

static void isis_init(node_t *node) {
      /* init the node */
      printf("%s()/n",__FUNCTION__);
}

static void isis_deinit(node_t *node) {
    /* deinit the node */
    printf("%s()/n",__FUNCTION__);
}

/* conf <node-name> protocol isis */
static int isis_config_handler(param_t *param, ser_buff_t *tlv_buf,
                                    op_mode enable_or_disable) {
        printf("passed isis config handler");
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
                    default:
                }
        }

       return 0;
}


int isis_config_cli_tree(param_t *param) {

    {
            // cli intergration protocol hiearchy
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
    }  

    return 0;  
}