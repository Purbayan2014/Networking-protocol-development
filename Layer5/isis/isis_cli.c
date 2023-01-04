#include <assert.h>
#include "../../tcp_public.h"
#include "isis_cmdcodes.h"

static int isis_config_handler(param_t *param, ser_buff_t *tlv_buf,
                                    op_mode enabe_or_disable) {
        printf("passed isis config handler");
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