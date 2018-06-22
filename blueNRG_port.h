#ifndef blueNRG_port_H
#define blueNRG_port_H


unsigned short bnRand(void );
mible_status_t bnReadMacAddr(uint8_t *addr);
mible_status_t bnsetScanParam(mible_gap_scan_type_t scanType,mible_gap_scan_param_t *sp);
mible_status_t bnEnDisScan(uint8_t enScan);
mible_status_t bnsetAdvParam(mible_gap_adv_param_t *ap);
mible_status_t bnGapAdvDataSet(uint8_t const * p_data,
                              uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);
mible_status_t bnEnDisAdv(uint8_t enAdv);
mible_status_t bnCreateConnection(mible_gap_scan_param_t *sp, mible_gap_connect_t *cp);
mible_status_t bnDisconnect(uint16_t handle);
mible_status_t bnUpdateConnParam(uint16_t conHandle,uint16_t intervalMin, 
                 uint16_t intervalMax, uint16_t latency, uint16_t timeout);
mible_status_t bnAddService(mible_gatts_srv_db_t *sdb);
mible_status_t bnReadLocalGattValue(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len);
mible_status_t bnUpdateCharacterValue(uint16_t conn_handle, uint16_t serv_handle, uint16_t char_value_handle, 
                                      uint8_t offset, uint8_t* p_value,
                                      uint8_t len, uint8_t type);
mible_status_t bnPrimServDiscByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, 
                                    mible_uuid_t *p_char_uuid);
mible_status_t bnCharacterDiscByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid);
mible_status_t bnReadValueByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid);
mible_status_t bnGattWrite(unsigned char enRsp,uint16_t conn_handle, uint16_t att_handle, uint8_t* p_value, uint8_t len);
mible_status_t bnGattREAuthorReply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type);
unsigned char bnTimerStart(void *t);
unsigned char bnTimerExpired(void *t);
unsigned char bnAesBlock(unsigned char* key,unsigned char *text, unsigned char *cipher);
void st_hex_dump(uint8_t *base_addr,uint8_t bytes );



#endif
