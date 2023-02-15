#include "../../tcp_public.h"
#include "isis_rtr.h"
#include "isis_pkt.h"
#include "isis_lspdb.h"
#include "isis_flood.h"
#include "isis_spf.h"
#include "isis_events.h"
#include "isis_adjacency.h"
#include "isis_ted.h"

void
isis_parse_lsp_tlvs_internal(isis_lsp_pkt_t *new_lsp_pkt, 
                             bool *on_demand_tlv);
// dummy lsp packet
static isis_lsp_pkt_t *gl_lsp_dummy_pkt = NULL;

static isis_lsp_pkt_t *
isis_get_dummy_lsp_pkt_with_key(uint32_t rtr_id) {

    uint32_t pkt_size;
    uint32_t *rtr_id_addr;

    if (!gl_lsp_dummy_pkt) {
        // create a new one 
        gl_lsp_dummy_pkt = XCALLOC(0, 1, isis_lsp_pkt_t);
        /* pkt size of the dummy lsp pkt */
        pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD +
                    ISIS_LSP_HDR_SIZE; /* sizeof(isis_pkt_hdr_t) as the isis_pkt_hdr_t contains the rtr_id */
        // init the pkt buffer that represents the isis_lsp_pkt
        gl_lsp_dummy_pkt->pkt = tcp_ip_get_new_pkt_buffer ( pkt_size);
        /* dummy lsp packet should not be used for flooding  */
        isis_mark_isis_lsp_pkt_flood_ineligible(0, gl_lsp_dummy_pkt);
        /* init the pkt size for the dummy lsp pkt */
        gl_lsp_dummy_pkt->pkt_size = pkt_size;
        /* expiry timer of the dummy lsp pkt */
        gl_lsp_dummy_pkt->expiry_timer = NULL;
        /* update the flag */
        gl_lsp_dummy_pkt->installed_in_db = false;
        /* referece the dummy lsp pkt  */
        isis_ref_isis_pkt(gl_lsp_dummy_pkt);
    }
    /* getting the rtr id of the dummy lsp pkt */
    rtr_id_addr = isis_get_lsp_pkt_rtr_id(gl_lsp_dummy_pkt);
    /* overwriting the rtr id with our rtr id for the dummy lsp pkt  */
    *rtr_id_addr = rtr_id;
    /* return the lsp pkt */
    return gl_lsp_dummy_pkt;
}

void
isis_free_dummy_lsp_pkt(void){
    /* as the dummy lsp pkt is created on the heap its getting freed from this func 
    as the protocol is getting shutdown */
    int rc;
    if(!gl_lsp_dummy_pkt) return ;
    rc = isis_deref_isis_pkt(gl_lsp_dummy_pkt);
    if (rc == 0) gl_lsp_dummy_pkt = NULL;
}

avltree_t *
isis_get_lspdb_root(node_t *node) {

    isis_node_info_t *node_info = ISIS_NODE_INFO(node);
    if(node_info) {
        return &node_info->lspdb_avl_root;
    }
    return NULL;
}

void
isis_install_lsp(node_t *node,
                 interface_t *iif,
                 isis_lsp_pkt_t *new_lsp_pkt) {
/* 
Event
Criteria
Received via interface
Action
Remark
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    isis_event_self_duplicate_lsp
    isis_our_lsp(node, new_lsp) == True and new_lsp→seq_no == old_lsp→seq_no
    Yes
    Ignore the LSP
    As a part of the flooding algo, Node may recv its own duplicate lsp from other node.Since node already has it, no action.

    No
    Assert(0);
    Node never generates a new LSP pkt with some seq no as before

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    isis_event_self_fresh_lsp
    isis_our_lsp(node, new_lsp) == True and old_lsp == NULL
    No 
    Install the lsp in the lspdb and Blind flood it 
    Propagete the lsp further as a part of the flooding algo


    Yes
    Ignore the lsp and self gen lsp with higher seq nos and blind flood it 
    Node receiving its own lsp when itself doesn’t have one, Node would try to overwrite the lsp of other node in the lspdb
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  
    isis_event_self_new_lsp
    isis_our_lsp(node, new_lsp) == True and new_lsp→seq_no > old_lsp→seq_no
    Yes
    Ignore the lsp and self gen lsp with higher seq nos and blind flood it 
    Node receiving its own lsp when itself doesn’t have one, Node would try to overwrite the lsp of other node in the lspdb


    No
    Replace the new lsp in the lspdb with old lsp and blind flood it
    Node is refreshing its own lsp with higher seq no
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    isis_event_self_old_lsp
    isis_our_lsp(node, new_lsp) == True and new_lsp→seq_no < old_lsp→seq_no
    yes
    Ignore the lsp and blood flood own lsp
    Node would blind flood its own lsp so as to overwrite its own old lsp in other node’s lspdb

    no
    Assert(0);
    Node cannot generate the lsp with old seq no

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Now the same for remote LSP
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    isis_event_non_local_duplicate_lsp
    isis_our_lsp(node, new_lsp) == False and new_lsp-> seq_no == old_lsp->seq_no
    Yes
    Ignore the LSP
    Node may recv own duplicate remote lsp from nbr node 

    No 
    assert(0);
    Node will never generate remote lsps 

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    isis_event_non_local_fresh_lsp
    isis_our_lsp(node, new_lsp) == False and old_lsp == null
    No 
    assert(0);
    Node never generate the remote lsps

    Yes
    Add LSP in DB, forward flood it 
    Node recvd lsp of remote node for the first time, install it 

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    isis_event_non_local_new_lsp
    isis_our_lsp(node, new_lsp) == False and new_lsp->seq_no > old_lsp->seq_no 
    Yes
    replace the old lsp with the new lsp in the lspdb, forward flood new lsp 
    Node recvd more recent remote lsp, update the lsdb with new lsp 

    No 
    assert(0);
    Node never generates remote lsps

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    isis_event_non_local_old_lsp
    isis_our_lsp(node, new_lsp) == False and new_lsp->seq_no < old_lsp->seq_no
    Yes 
    ignore the lsp, shoot back the lsp already in the lspdb on the recving interface 
    Node would try to ovverwrite tge older remote lsp in other node lsdb by adversting the newer remote lsp it has 

    No
    assert(0);
    node never generate the remote lsps
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */


    bool self_lsp; /* tracks whether the self lsp packet is the lsp packet of the node or not */
    bool recvd_via_intf; /* tracks if it was recvd on the interface or not  */
    uint32_t *rtr_id; /* ptr to the rtr id that has been stored in the new lsp packet  */
    ip_add_t rtr_id_str;
    isis_lsp_pkt_t *old_lsp_pkt;
    isis_event_type_t event_type;
    bool duplicate_lsp;
    uint32_t *old_seq_no = NULL;
    isis_pkt_hdr_flags_t lsp_flags;
    
    /* recvd on the interface or not */
    recvd_via_intf = iif ? true : false;
    /* checking if its own lsp  */
    self_lsp = isis_our_lsp(node, new_lsp_pkt);
    event_type = isis_event_none; /* will help in computing the event the has been occured */
    lsp_flags = isis_lsp_pkt_get_flags(new_lsp_pkt);
    /* fetch the rtr id of the new lsp packet  */
    rtr_id = isis_get_lsp_pkt_rtr_id(new_lsp_pkt);
    /* convert to the string form to print it out  */
    tcp_ip_covert_ip_n_to_p(*rtr_id, rtr_id_str.ip_addr);

    bool purge_lsp = lsp_flags & ISIS_LSP_PKT_F_PURGE_BIT;
    /* fetch the old lsp from the rtr id */
    old_lsp_pkt = isis_lookup_lsp_from_lsdb(
                    node, *rtr_id);
    /* if the old lsp exist then get the seq nos for that old lsp  */
    if (old_lsp_pkt) {
        isis_ref_isis_pkt(old_lsp_pkt);
        old_seq_no = isis_get_lsp_pkt_seq_no(old_lsp_pkt);
    }
    /* or else print the new seq no if it exists  */
    uint32_t *new_seq_no = isis_get_lsp_pkt_seq_no(new_lsp_pkt);

    sprintf(tlb, "%s : Lsp Recvd : %s-%u(%p) on intf %s, old lsp : %s-%u(%p)\n",
            ISIS_LSPDB_MGMT,
            rtr_id_str.ip_addr, *new_seq_no, 
            new_lsp_pkt->pkt,
            iif ? iif->if_name : 0,
            old_lsp_pkt ? rtr_id_str.ip_addr : 0,
            old_lsp_pkt ? *old_seq_no : 0,
            old_lsp_pkt ? old_lsp_pkt->pkt : 0);
    tcp_trace(node, iif, tlb);

    /* set it to true if the old lsp matches the new lsp  */
    duplicate_lsp = (old_lsp_pkt && (*new_seq_no == *old_seq_no));

    if (self_lsp && duplicate_lsp) {

        event_type = isis_event_self_duplicate_lsp;
        sprintf(tlb, "\t%s : Event : %s : self Duplicate LSP, No Action\n",
            ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foriegn lsp then do nothing
            2. if self originated lsp then assert, impossible case*/
        if (recvd_via_intf) {

             // no action
        } else {

            assert(0);
        }
    }

    else if (self_lsp && !old_lsp_pkt) {

        event_type = isis_event_self_fresh_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foriegn lsp then ignore, and regenerate self lsp with higher sequence
                no and flood on all intf
            2. if self originated lsp then install in db and flood on all intf*/
        if (recvd_via_intf) {

            ((isis_node_info_t *)(node->node_nw_prop.isis_node_info))->seq_no = *new_seq_no;

            sprintf(tlb, "\t%s : Event : %s : self-LSP to be generated with seq no %u\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type), *new_seq_no + 1);
            tcp_trace(node, iif, tlb);
            
            isis_schedule_lsp_pkt_generation(node, isis_event_self_fresh_lsp);
        } else {
            sprintf(tlb, "\t%s : Event : %s : LSP to be Added in LSPDB and flood\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type));
            tcp_trace(node, iif, tlb);
            isis_add_lsp_pkt_in_lspdb(node, new_lsp_pkt);
            /* as the exemepted interface is null so its blind flooding */
            isis_schedule_lsp_flood(node, new_lsp_pkt, 0, event_type);
        }
    }

    else if (self_lsp && old_lsp_pkt && (*new_seq_no > *old_seq_no)) {

        event_type = isis_event_self_new_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp, regenerate self lsp with higher 
                sequence no and flood on all intf
            2. if self originated lsp then replace it in db and 
                install new one and flood it on all intf */
        if (recvd_via_intf) {

            ((isis_node_info_t *)(node->node_nw_prop.isis_node_info))->seq_no = *new_seq_no;
            sprintf(tlb, "\t%s : Event : %s : LSP to be generated with seq no %u\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type), *new_seq_no + 1);
            tcp_trace(node, iif, tlb);
            isis_schedule_lsp_pkt_generation(node, isis_event_self_new_lsp);
        } else {
            sprintf(tlb, "\t%s : Event : %s : LSP %s-%u to be replaced in LSPDB "
                "with new LSP %s-%u and flood\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                rtr_id_str.ip_addr, *old_seq_no,
                rtr_id_str.ip_addr, *new_seq_no);
            tcp_trace(node, iif, tlb);
            isis_remove_lsp_pkt_from_lspdb(node, old_lsp_pkt);
            isis_mark_isis_lsp_pkt_flood_ineligible(node, old_lsp_pkt);
            isis_add_lsp_pkt_in_lspdb(node, new_lsp_pkt);
            isis_schedule_lsp_flood(node, new_lsp_pkt, 0, event_type);
        }
    }

    else if (self_lsp && old_lsp_pkt && (*new_seq_no < *old_seq_no)) {

        event_type = isis_event_self_old_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp, then flood existing one on all intf
            2. if self originated lsp then assert, impossible case */
        if (recvd_via_intf) {
            sprintf(tlb, "\t%s : Event : %s : LSP %s-%u to be flooded\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                rtr_id_str.ip_addr, *old_seq_no);
            tcp_trace(node, iif, tlb);
            isis_schedule_lsp_flood(node, old_lsp_pkt, 0, event_type);
        } else {

            assert(0);
        }
    }

    /* processing the remote lsp packet events  */
    else if (!self_lsp && duplicate_lsp) {

        event_type = isis_event_non_local_duplicate_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp then do nothing
            2. if self originated lsp then assert, impossible case */
        if (recvd_via_intf) {
            sprintf(tlb, "\t%s : Event : %s Recvd Duplicate LSP %s-%u, no Action\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                rtr_id_str.ip_addr, *new_seq_no);
            tcp_trace(node, iif, tlb);
        } else {

            assert(0);
        }
    }

    else if (!self_lsp && !old_lsp_pkt) {

        event_type = isis_event_non_local_fresh_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp then install in db and flood forward it
            2. if self originated lsp then assert, impossible case */
        if (recvd_via_intf) {
            sprintf(tlb, "\t%s : Event : %s : LSP %s-%u to be Added in LSPDB and flood\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                rtr_id_str.ip_addr, *new_seq_no);
            tcp_trace(node, iif, tlb);
            if (!purge_lsp) {
                isis_add_lsp_pkt_in_lspdb(node, new_lsp_pkt);
                /* Do not flood purge LSP if it do not removes LSP from our DB*/
                isis_schedule_lsp_flood(node, new_lsp_pkt, iif, event_type);
            }
        } else {

            assert(0);
        }
    }

    else if (!self_lsp && old_lsp_pkt && (*new_seq_no > *old_seq_no)) {

        event_type = isis_event_non_local_new_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp then replace in db and flood forward it
            2. if self originated lsp then assert, impossible case */
        if (recvd_via_intf) {
            sprintf(tlb, "\t%s : Event : %s : LSP %s-%u to be replaced in LSPDB with"
                    " LSP %s-%u and flood\n",
                    ISIS_LSPDB_MGMT, isis_event_str(event_type),
                    rtr_id_str.ip_addr, *old_seq_no,
                    rtr_id_str.ip_addr, *new_seq_no);
            tcp_trace(node, iif, tlb);
            isis_remove_lsp_pkt_from_lspdb(node, old_lsp_pkt);
            isis_mark_isis_lsp_pkt_flood_ineligible(node, old_lsp_pkt);
            if (!purge_lsp) {
                isis_add_lsp_pkt_in_lspdb(node, new_lsp_pkt);
            }
            isis_schedule_lsp_flood(node, new_lsp_pkt, iif, event_type);
        } else {

            assert(0);
        }
    }

    else if (!self_lsp && old_lsp_pkt && (*new_seq_no < *old_seq_no)) {

        event_type = isis_event_non_local_old_lsp;
        sprintf(tlb, "\t%s : Event : %s\n", ISIS_LSPDB_MGMT, isis_event_str(event_type));
        tcp_trace(node, iif, tlb);
        /* Action :
            1. if foreign lsp then shoot out lsp back on recv intf
            2. if self originated lsp then assert, impossible case */
        if (recvd_via_intf) {
            sprintf(tlb, "\t%s : Event : %s Old LSP %s-%u will be back fired out of intf %s\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                rtr_id_str.ip_addr, *old_seq_no,
                iif->if_name);
            tcp_trace(node, iif, tlb);
            isis_queue_lsp_pkt_for_transmission(iif, old_lsp_pkt);
        } else {

            assert(0);
        }
    }

    sprintf(tlb, "%s : LSPDB Updated  for new Lsp Recvd : %s-%u, old lsp : %s-%u, Event : %s\n",
            ISIS_LSPDB_MGMT,
            rtr_id_str.ip_addr, *new_seq_no,
            old_lsp_pkt ? rtr_id_str.ip_addr :0,
            old_lsp_pkt ? *old_seq_no : 0,
            isis_event_str(event_type));
    tcp_trace(node, iif, tlb);

    ISIS_INCREMENT_NODE_STATS(node, isis_event_count[event_type]);
    
    if (purge_lsp && event_type == isis_event_non_local_new_lsp) {

        /* purge LSP actually caused deletion from our DB, trigger spf*/
        isis_schedule_spf_job(node);
    }

    /* Now Decide what we need to do after updating LSP DB */
    if (!purge_lsp) {
        isis_parse_lsp_tlvs(node, new_lsp_pkt, old_lsp_pkt, event_type);
    }

    if (old_lsp_pkt) {
        isis_deref_isis_pkt(old_lsp_pkt);
    }
}

void
isis_parse_lsp_tlvs_internal(isis_lsp_pkt_t *new_lsp_pkt, 
                             bool *on_demand_tlv) {

    *on_demand_tlv = false;

    /* Now parse and see on demand TLV is present */

    ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)(new_lsp_pkt->pkt);
    byte *lsp_hdr = eth_hdr->payload;
    byte *lsp_tlv_buffer = lsp_hdr + ISIS_LSP_HDR_SIZE;
    uint16_t lsp_tlv_buffer_size = new_lsp_pkt->pkt_size - 
                                   ETH_HDR_SIZE_EXCL_PAYLOAD -
                                   ISIS_LSP_HDR_SIZE;

    byte tlv_type, tlv_len, *tlv_value = NULL;

    ITERATE_TLV_BEGIN(lsp_tlv_buffer, tlv_type, 
                      tlv_len, tlv_value, 
                      lsp_tlv_buffer_size) {

        switch(tlv_type) {

            case ISIS_TLV_ON_DEMAND:
                *on_demand_tlv = true;
            break;
            default: ;
        }
    } ITERATE_TLV_END(lsp_tlv_buffer, tlv_type, 
                      tlv_len, tlv_value,
                      lsp_tlv_buffer_size)
}

void
isis_parse_lsp_tlvs(node_t *node,
                    isis_lsp_pkt_t *new_lsp_pkt,
                    isis_lsp_pkt_t *old_lsp_pkt,
                    isis_event_type_t event_type) {

    bool need_spf = false;
    bool pkt_diff = false;
    bool on_demand_tlv = false;
    bool need_pkt_diff = false;
    bool need_on_demand_flood = false;
    ip_add_t rtr_id_str;

    uint32_t *rtr_id = isis_get_lsp_pkt_rtr_id(new_lsp_pkt);
    uint32_t *old_seq_no = old_lsp_pkt ? isis_get_lsp_pkt_seq_no(old_lsp_pkt) : 0;
    uint32_t *new_seq_no = isis_get_lsp_pkt_seq_no(new_lsp_pkt);

    tcp_ip_covert_ip_n_to_p(*rtr_id, rtr_id_str.ip_addr);

    isis_node_info_t *node_info = ISIS_NODE_INFO(node);

    isis_parse_lsp_tlvs_internal(new_lsp_pkt, &on_demand_tlv);

    switch(event_type) {
        case isis_event_self_duplicate_lsp:
        break;
        case isis_event_self_fresh_lsp:
            need_spf = true;
        break;
        case isis_event_self_new_lsp:
            /* spf would have scheduled already when event causing
               lsp generation happened */
        break;
        case isis_event_self_old_lsp:
        break;
        case isis_event_non_local_duplicate_lsp:
        break;
        case isis_event_non_local_fresh_lsp:
            need_spf = true;
            if (on_demand_tlv) need_on_demand_flood = true;
        case isis_event_non_local_new_lsp:
            need_pkt_diff = true;
            if (on_demand_tlv) need_on_demand_flood = true;
        case isis_event_non_local_old_lsp:
        break;
        default: ;
    }
    
    if (!need_spf && need_pkt_diff) {

        pkt_diff = isis_is_lsp_diff(new_lsp_pkt, old_lsp_pkt);
        
        if (pkt_diff) {
            need_spf = true;
        }
    }

    if (need_spf) {
        isis_schedule_spf_job(node);
    }

    sprintf(tlb, "%s : Lsp Recvd : %s-%u, old lsp : %s-%u, Event : %s\n"
            "\tneed_spf : %u  on_demand_tlv : %u  need_on_demand_flood : %u\n",
            ISIS_LSPDB_MGMT,
            rtr_id_str.ip_addr, *new_seq_no,
            old_lsp_pkt ? rtr_id_str.ip_addr : 0,
            old_lsp_pkt ? *old_seq_no : 0,
            isis_event_str(event_type),
            need_spf, on_demand_tlv, need_on_demand_flood);
    tcp_trace(node, 0, tlb);


    if (need_on_demand_flood) {

        /* Somebody requested us On-Demand Flood */
        if (node_info->lsp_pkt_gen_task ||
            isis_is_reconciliation_in_progress(node)) {
            return;
        }

        if (node_info->self_lsp_pkt &&
            node_info->self_lsp_pkt->flood_eligibility) {

                uint32_t *seq_no = isis_get_lsp_pkt_seq_no(node_info->self_lsp_pkt);
                ISIS_INCREMENT_NODE_STATS(node, seq_no);
                *seq_no = node_info->seq_no;
                
                isis_ted_refresh_seq_no(node, *seq_no);

                sprintf(tlb, "\t%s : Event : %s : self-LSP %s to be on-demand flooded\n",
                    ISIS_LSPDB_MGMT, isis_event_str(event_type),
                    isis_print_lsp_id(node_info->self_lsp_pkt));
                tcp_trace(node, 0, tlb);

                isis_schedule_lsp_flood(node, node_info->self_lsp_pkt,
                                        0, isis_event_on_demand_flood);

                ISIS_INCREMENT_NODE_STATS(node,
                    isis_event_count[isis_event_on_demand_flood]);
        }
        else {
            sprintf(tlb, "\t%s : Event : %s : self-LSP %s to be re-generated with next seq no\n",
                ISIS_LSPDB_MGMT, isis_event_str(event_type),
                isis_print_lsp_id(node_info->self_lsp_pkt));
            tcp_trace(node, 0, tlb);
            isis_schedule_lsp_pkt_generation(node, isis_event_on_demand_flood);
        }
    }
}

isis_lsp_pkt_t *
isis_lookup_lsp_from_lsdb(node_t *node, uint32_t rtr_id) {

    avltree_t *lspdb = isis_get_lspdb_root(node);

    if (!lspdb) return NULL;
    /* to perform the lookup we need a dummy which only contains the key*/
    isis_lsp_pkt_t *dummy_lsp_pkt = isis_get_dummy_lsp_pkt_with_key(rtr_id);
    /* performing the lookup */
    avltree_node_t *avl_node =
        avltree_lookup(&dummy_lsp_pkt->avl_node_glue, lspdb);

    if (!avl_node) return NULL;
    // return the lsp pkt parent node
    return avltree_container_of(avl_node, isis_lsp_pkt_t, avl_node_glue);
}

bool
isis_our_lsp(node_t *node, isis_lsp_pkt_t *lsp_pkt) {
    /* util function to check if this is native lsp packet or not */
    isis_node_info_t *node_info = ISIS_NODE_INFO(node);
    /* extracting the rtr id of the lsp  */
    uint32_t *rtr_id = isis_get_lsp_pkt_rtr_id(lsp_pkt);
    /* extaracting the rtr id of the node */
    uint32_t self_loop_back = tcp_ip_covert_ip_p_to_n(
                                NODE_LO_ADDR(node));

    return *rtr_id == self_loop_back;
}

void
isis_cleanup_lsdb(node_t *node) {

    avltree_node_t *curr;
    isis_lsp_pkt_t *lsp_pkt;
    avltree_t *lspdb = isis_get_lspdb_root(node);

    if (!lspdb) return;

    // iterate through the avl tree 
    ITERATE_AVL_TREE_BEGIN(lspdb, curr){
        // fetch the lsp packet
        lsp_pkt = avltree_container_of(curr, isis_lsp_pkt_t, avl_node_glue);
        // remove the lsp pkt from the lspdb
        isis_remove_lsp_pkt_from_lspdb(node, lsp_pkt);
    } ITERATE_AVL_TREE_END;
}

void
isis_show_lspdb(node_t *node) {

    int rc = 0;
    isis_lsp_pkt_t *lsp_pkt;
    avltree_node_t *curr;
    avltree_t *lspdb = isis_get_lspdb_root(node);
    byte *buff = node->print_buff;

    if (!lspdb) return;

    ITERATE_AVL_TREE_BEGIN(lspdb, curr){

        lsp_pkt = avltree_container_of(curr, isis_lsp_pkt_t, avl_node_glue);

        rc += isis_show_one_lsp_pkt(lsp_pkt, buff + rc );

    } ITERATE_AVL_TREE_END;

    cli_out (buff, rc);
}


/* lsp pkt printing */

int
isis_show_one_lsp_pkt( isis_lsp_pkt_t *lsp_pkt, byte *buff) {

    int rc = 0;
    ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)lsp_pkt->pkt;
    byte *lsp_hdr = eth_hdr->payload;

    uint32_t *rtr_id = isis_get_lsp_pkt_rtr_id(lsp_pkt);
    uint32_t *seq_no = isis_get_lsp_pkt_seq_no(lsp_pkt);

    byte *lsp_tlv_buffer = lsp_hdr + ISIS_LSP_HDR_SIZE;

    unsigned char *rtr_id_str = tcp_ip_covert_ip_n_to_p(*rtr_id, 0);
    rc += sprintf(buff + rc, "LSP : %-16s   Seq # : %-4u    size(B) : %-4lu    "
            "ref_c : %-3u   ",
            rtr_id_str, *seq_no, 
            lsp_pkt->pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD,
            lsp_pkt->ref_count);

    if (lsp_pkt->expiry_timer) {
        rc += sprintf(buff + rc, "Life Time Remaining : %u sec\n",
            wt_get_remaining_time(lsp_pkt->expiry_timer)/1000);
    }
    else {
        rc += sprintf(buff + rc, "\n");
    }
   return rc;
}

void
isis_show_one_lsp_pkt_detail(node_t *node, char *rtr_id_str) {

    /* printing the contents of the lsp packets */
    int rc = 0;
    isis_lsp_pkt_t *lsp_pkt;
    uint32_t rtr_id_int;

    // printable memory buffer 
    char *buff = node->print_buff;

    byte tlv_type, tlv_len, *tlv_value = NULL;

    // fetching the lspdb
    avltree_t *lspdb = isis_get_lspdb_root(node);

    if (!lspdb) return;

    rtr_id_int = tcp_ip_covert_ip_p_to_n(rtr_id_str);

    lsp_pkt = isis_lookup_lsp_from_lsdb(
                    node, rtr_id_int);
                    
    if (!lsp_pkt) {
        rc = sprintf(buff + rc ,  "No LSP Found\n");
        cli_out(buff, rc);
        return;
    }

    ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)lsp_pkt->pkt;
    isis_pkt_hdr_t *lsp_pkt_hdr = (isis_pkt_hdr_t *)(eth_hdr->payload);
    isis_pkt_hdr_flags_t flags = isis_lsp_pkt_get_flags(lsp_pkt);

    rc += sprintf(buff + rc, "LSP : %s(%u)\n",
        tcp_ip_covert_ip_n_to_p(lsp_pkt_hdr->rtr_id, 0), 
        lsp_pkt_hdr->seq_no);

    rc += sprintf(buff + rc,  "Flags :  \n");
    rc += sprintf(buff + rc,  
                "  OL bit : %s\n", flags & ISIS_LSP_PKT_F_OVERLOAD_BIT ? "Set" : "UnSet");
    rc += sprintf(buff + rc, 
                "  Purge bit : %s\n", flags & ISIS_LSP_PKT_F_PURGE_BIT ? "Set" : "UnSet");
    rc += sprintf(buff + rc, "TLVs\n");

    byte *lsp_tlv_buffer = (byte *)(lsp_pkt_hdr + 1);
    uint16_t lsp_tlv_buffer_size = (uint16_t)(lsp_pkt->pkt_size -
                                        ETH_HDR_SIZE_EXCL_PAYLOAD -
                                        sizeof(isis_pkt_hdr_t)) ;

    // iterating over the tlv until the tlvs getting exhausted 
    ITERATE_TLV_BEGIN(lsp_tlv_buffer, tlv_type,
                        tlv_len, tlv_value,
                        lsp_tlv_buffer_size) {

        switch(tlv_type) {
            case ISIS_TLV_HOSTNAME:
                rc += sprintf(buff + rc,  "\tTLV%d Host-Name : %s\n", 
                        tlv_type, tlv_value);
            break;
            case ISIS_IS_REACH_TLV:
                 rc += isis_print_formatted_nbr_tlv22( buff ? buff + rc:NULL, // pointer to the start the tlv 22
                        tlv_value - TLV_OVERHEAD_SIZE,
                        tlv_len + TLV_OVERHEAD_SIZE); // size of the tlv 22
                break;
            case ISIS_TLV_ON_DEMAND:
                rc += sprintf(buff + rc, "\tTLV%d On-Demand TLV : %hhu\n",
                        tlv_type, *(uint8_t *)tlv_value);
                break;
            default: ;
        }
    } ITERATE_TLV_END(lsp_tlv_buffer, tlv_type,
                        tlv_len, tlv_value,
                        lsp_tlv_buffer_size);

    cli_out(buff, rc);
}

bool
isis_is_lsp_diff(isis_lsp_pkt_t *lsp_pkt1, isis_lsp_pkt_t *lsp_pkt2) {

    if ((lsp_pkt1 && !lsp_pkt2) || (!lsp_pkt1 && lsp_pkt2)) {

        return true;
    }

    if (lsp_pkt1->pkt_size != lsp_pkt2->pkt_size) {

        return true;
    }


    

    ethernet_hdr_t *lsp_eth_hdr1 = (ethernet_hdr_t *)lsp_pkt1->pkt;
    ethernet_hdr_t *lsp_eth_hdr2 = (ethernet_hdr_t *)lsp_pkt2->pkt;

    isis_pkt_hdr_t *lsp_hdr1 = (isis_pkt_hdr_t *)lsp_eth_hdr1->payload;
    isis_pkt_hdr_t *lsp_hdr2 = (isis_pkt_hdr_t *)lsp_eth_hdr2->payload;

    assert(lsp_hdr1->rtr_id == lsp_hdr2->rtr_id);

    if (lsp_hdr1->flags != lsp_hdr2->flags) return true;

    return memcmp( (byte *) (lsp_hdr1 + 1) , 
                                (byte *) (lsp_hdr2 + 1),
                                lsp_pkt1->pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD - ISIS_LSP_HDR_SIZE);
}

byte*
isis_print_lsp_id(isis_lsp_pkt_t *lsp_pkt) {

    static byte lsp_id[32];
    
    memset(lsp_id, 0, sizeof(lsp_id));
    uint32_t *rtr_id = isis_get_lsp_pkt_rtr_id(lsp_pkt);
    uint32_t *seq_no = isis_get_lsp_pkt_seq_no(lsp_pkt);

    sprintf(lsp_id, "%s-%u", tcp_ip_covert_ip_n_to_p(*rtr_id, 0), *seq_no);
    return lsp_id;
}

/* LSP pkt Timers */

static void
isis_lsp_pkt_delete_from_lspdb_timer_cb(void *arg, uint32_t arg_size){

    if (!arg) return;

    isis_timer_data_t *timer_data = 
            (isis_timer_data_t *)arg;

    node_t *node = timer_data->node;
    isis_lsp_pkt_t *lsp_pkt = (isis_lsp_pkt_t *)timer_data->data;

    timer_data->data = NULL;
    XFREE(timer_data);

    timer_de_register_app_event(lsp_pkt->expiry_timer);
    lsp_pkt->expiry_timer = NULL;

    avltree_remove(&lsp_pkt->avl_node_glue, isis_get_lspdb_root(node));
    lsp_pkt->installed_in_db = false;
    isis_ted_uninstall_lsp(node, lsp_pkt);
    isis_deref_isis_pkt(lsp_pkt);
}

void
isis_start_lsp_pkt_installation_timer(node_t *node, isis_lsp_pkt_t *lsp_pkt) {

    wheel_timer_t *wt;
    isis_node_info_t *node_info;

    node_info = ISIS_NODE_INFO(node);

    wt = node_get_timer_instance(node);

    if (lsp_pkt->expiry_timer) return;

    isis_timer_data_t *timer_data = XCALLOC(0, 1, isis_timer_data_t);
    timer_data->node = node;
    timer_data->data = (void *)lsp_pkt;
    timer_data->data_size = sizeof(isis_lsp_pkt_t);
    
    lsp_pkt->expiry_timer = timer_register_app_event(wt,
                                isis_lsp_pkt_delete_from_lspdb_timer_cb,
                                (void *)timer_data,
                                sizeof(isis_timer_data_t),
                                ISIS_NODE_INFO(node)->lsp_lifetime_interval * 1000,
                                0);
}

void
isis_stop_lsp_pkt_installation_timer(isis_lsp_pkt_t *lsp_pkt) {

    if (!lsp_pkt->expiry_timer) return;

    isis_timer_data_t *timer_data = wt_elem_get_and_set_app_data(
                                        lsp_pkt->expiry_timer, 0);
    XFREE(timer_data);                                 
    timer_de_register_app_event(lsp_pkt->expiry_timer);
    lsp_pkt->expiry_timer = NULL;
}

void
isis_refresh_lsp_pkt_installation_timer(node_t *node, isis_lsp_pkt_t *lsp_pkt) {

    isis_stop_lsp_pkt_installation_timer(lsp_pkt);
    isis_start_lsp_pkt_installation_timer(node, lsp_pkt);
}

bool
isis_is_lsp_pkt_installed_in_lspdb(isis_lsp_pkt_t *lsp_pkt) {

    return lsp_pkt->installed_in_db;
}

void
isis_remove_lsp_pkt_from_lspdb(node_t *node, isis_lsp_pkt_t *lsp_pkt) {

    avltree_t *lspdb = isis_get_lspdb_root(node);

    if (!lspdb) return;

    // sanity check
    if (!isis_is_lsp_pkt_installed_in_lspdb(lsp_pkt)) return;
    // remove the node from the lspdb
    avltree_remove(&lsp_pkt->avl_node_glue, lspdb);
    // updating the bool flag 
    lsp_pkt->installed_in_db = false;
    isis_ted_uninstall_lsp(node, lsp_pkt);
    isis_stop_lsp_pkt_installation_timer(lsp_pkt);
    sprintf(tlb, "%s : LSP %s removed from LSPDB\n", ISIS_LSPDB_MGMT,
        isis_print_lsp_id(lsp_pkt));
    tcp_trace(node, 0, tlb);
    // deref the removed lsp pkt
    isis_deref_isis_pkt(lsp_pkt);
}

bool
isis_add_lsp_pkt_in_lspdb(node_t *node, isis_lsp_pkt_t *lsp_pkt) {

    // get the root node 
    avltree_t *lspdb = isis_get_lspdb_root(node);
     // sanity check
     if (!lspdb) return false;
     // sanity check
     if (isis_is_lsp_pkt_installed_in_lspdb(lsp_pkt)) return false;
     // add the lsp packet in the lsdpdb
     avltree_insert(&lsp_pkt->avl_node_glue, lspdb);
     // update the bool flag 
     lsp_pkt->installed_in_db = true;
     isis_ted_install_lsp(node, lsp_pkt);
     if (!isis_our_lsp(node, lsp_pkt)) {
         isis_start_lsp_pkt_installation_timer(node, lsp_pkt);
     }
     // reference the lsp pkt
     isis_ref_isis_pkt(lsp_pkt);
     sprintf(tlb, "%s : LSP %s added to lspdb\n", ISIS_LSPDB_MGMT, 
        isis_print_lsp_id(lsp_pkt));
     tcp_trace(node, 0, tlb);
     return true;
}

void
isis_remove_lsp_from_lspdb(node_t *node, uint32_t rtr_id) {

    avltree_t *lspdb = isis_get_lspdb_root(node);

    if (!lspdb) return ;
    // lcoate the lsp pkt from the lspdb using the rtr id
    isis_lsp_pkt_t *lsp_pkt = isis_lookup_lsp_from_lsdb(node, rtr_id);

    if (!lsp_pkt) return;
    // remove the packet after being found
    isis_remove_lsp_pkt_from_lspdb(node, lsp_pkt);
}
