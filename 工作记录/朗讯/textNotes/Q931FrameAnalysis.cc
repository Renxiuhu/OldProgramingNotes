/*Q931 msg buffer打印函数备注解析
*  L2~L3:   pad为原始的q921桢结构，其他为解析出来的附加信息
*     Msg Buffer Length(BYTE 0) + Buffer Offset(BYTE 1) + pad (BYTE 2~7)
*         + Discriminator(BYTE 8) + Reference (BYTE 9,10) + Msg Type (BYTE 11)
*         + BA_UP_IE(BYTE 12,13,14) + Interface Id(BYTE15)
*         + SAPI_IE(BYTE 16,17,18) + TEI_IE(BYTE 19,20,21)
*         + Q931_PAYLOAD_IE(22) + q931_data_length (BYTE 23,24)
*         + Q931 Msg Buffer (....)
*
*        Msg Buffer Length(BYTE 0) + Buffer Offset(BYTE 1) + pad (BYTE 2~7)
*               + Discriminator(BYTE 8) + Reference (BYTE 9,10) + Msg Type (BYTE 11)
*               + BA_UP_IE(BYTE 12,13,14) + Interface Id(BYTE15)
*               + SAPI_IE(BYTE 16,17,18) + TEI_IE(BYTE 19,20,21)
*               + TEI_CONF_IE(BYTE 22,23,24)
 */
void IACS_PrintQ931Packet(u32 trace_flag, u8 *pData, u16 data_len)
{
    u16 q931_cr;
    u8  protocol_discriminator;
    u8  q931_cr_len;
    u8  q931_msg_type;
    char  *msg_type_str;
    u8  *q931_ies_ptr;
    u16 ies_len_left;
    u16 ie_len_processed;
    u8  ret;

    protocol_discriminator = pData[0];
    ret = 1;
    q931_ies_ptr = 0;
    ies_len_left = 0;
    //协议鉴别语为08表示是931协议
    if (protocol_discriminator == 0x08) 
    {
        q931_cr_len = pData[1] & 0x0F; //获取呼叫参考的长度

        switch (q931_cr_len)
        {
            case 0x01:    //呼叫参考为一个byte长度

                if (data_len < 4)
                {
                    trc_printf(trace_flag,  "IACS_PrintQ931Pkt, invalid data len:%d\r\n",
                                    data_len);
                    ret = 0;
                    break;
                }

                q931_cr = pData[2] & 0x7F;
                q931_msg_type = pData[3] & 0x7F;
                q931_ies_ptr = &pData[4];
                //计算剩余信息单元的长度，去除协议鉴别语，呼叫参考，消息类型的长度
                ies_len_left = data_len - 4;   
                break;
            case 0x02:   //呼叫参考为两个byte长度
                if (data_len < 5)
                {
                    trc_printf(trace_flag,  "IACS_PrintQ931Pkt, invalid data len:%d\r\n",
                                    data_len);
                    ret = 0;
                    break;
                }
                q931_cr = (pData[2] & 0x7F) << 8;
                q931_cr |= pData[3];
                q931_msg_type = pData[4] & 0x7F;
                q931_ies_ptr = &pData[5];
                ies_len_left = data_len - 5;
                break;
            default:
                trc_printf(trace_flag,  "IACS_PrintQ931Pkt, invalid callref len:%d\r\n",
                                    q931_cr_len);
                ret = 0;
                break;
        }

    }
    else  //不是931协议
    {
        trc_printf(trace_flag,  "IACS_PrintQ931Pkt, invalid protocol discriminator:%d\r\n",
                                    protocol_discriminator);
        ret = 0;
    }

    if (ret)
    {//根据获取到的msg_type得到消息字符串
        IACS_ConvtQ931MsgTypeToStr(q931_msg_type, &msg_type_str);
        trc_printf (trace_flag, "##Q931,cr:%d,ie len:%d,msg:",
                                       q931_cr, ies_len_left);
        trc_printf (trace_flag, "++++ %s ++++\r\n", msg_type_str);
    }

    while ((ies_len_left > 1) &&(ret))
    {//如果有其他信息单元，打印信息单元
        ret = IACS_PrintQ931IE(trace_flag, q931_ies_ptr, ies_len_left, &ie_len_processed);

        ies_len_left -= ie_len_processed;
        q931_ies_ptr += ie_len_processed;
    }	
}

/*打印q931消息中其他信息单元，备注解析*/
u8 IACS_PrintQ931IE(u32 trace_flag, u8 *q931_ies_ptr, u16 ies_len_left, u16 *info_len_processed)
{
    u8 ie_id;
    u8 ie_len;
    const char *ie_name;
    u8 singal_octet_flag;
    u8 type2;
    u8 ret;
    u8 i;
    //获取信息单元鉴别符最后一位，为1表示下面的信息单元为单byte，为0表示多byte
    singal_octet_flag = q931_ies_ptr[0] & 0x80;
    ret = 1;

    if (singal_octet_flag) //单byte信息单元
    {
        ie_len = 0;
        *info_len_processed = 1;

    type2 = q931_ies_ptr[0] & 0x30; //获取鉴别符65位的值

    if (type2 == 0x20) //65位为10，则是more data和sending complete两种消息
        {
            ie_id = q931_ies_ptr[0];
            //根据整个信息单元鉴别符，标识不同的信息单元
            switch (ie_id)
            {
                case IACS_Q931_MORE_DATA_ID:
                    ie_name = IACS_Q931_MORE_DATA_NAME;
                    break;
                case IACS_Q931_SENDING_COMPLETE_ID:
                    ie_name = IACS_Q931_SENDING_COMPLETE_NAME;
                    break;
                default:
                    ie_name = IACS_Q931_UNKNOWN_IE_NAME;
                    break;
            }
        }
        else  //65位不为10，则为其他消息，这些消息1~4位不使用，因此ie_id获取方式不同
        {
            ie_id = q931_ies_ptr[0] & 0xF8;

            switch (ie_id)
            {
                case IACS_Q931_CONGESTION_LEVEL_ID:
                    ie_name = IACS_Q931_CONGESTION_LEVEL_NAME;
                    break;
                case IACS_Q931_NON_LOCKING_SHIFT_ID:
                    ie_name = IACS_Q931_NON_LOCKING_SHIFT_NAME;
                    break;
                case IACS_Q931_LOCKING_SHIFT_ID:
                    ie_name = IACS_Q931_LOCKING_SHIFT_NAME;
                    break;
                    /*
                case IACS_Q931_REPEAT_INDICATOR_ID:
                    ie_name = IACS_Q931_REPEAT_INDICATOR_NAME;
                    break;
                    */
                default:
                    ie_name = IACS_Q931_UNKNOWN_IE_NAME;
                    break;
            }
        }
    }
    else  //多byte信息单元
    {
        ie_id = q931_ies_ptr[0] & 0x7F; //获取1~7位为ie_id，第8位为0无意义
        ie_len = q931_ies_ptr[1];  //多byte信息单元 下一byte为长度
        *info_len_processed = ie_len + 2;

        if (ies_len_left >= *info_len_processed)
        {
            switch (ie_id)
            {
                case IACS_Q931_SEGMENTED_MESSAGE_ID:
                    ie_name = IACS_Q931_SEGMENTED_MESSAGE_NAME;
                    break;
                case IACS_Q931_CHANGE_STATUS_ID:
                    ie_name = IACS_Q931_CHANGE_STATUS_NAME;
                    break;
                case IACS_Q931_BEARER_CAPABILITY_ID:
                    ie_name = IACS_Q931_BEARER_CAPABILITY_NAME;
                    break;
                case IACS_Q931_CAUSE_ID:
                    ie_name = IACS_Q931_CAUSE_NAME;
                    break;
                case IACS_Q931_EXT_FACILITY_ID:
                    ie_name = IACS_Q931_EXT_FACILITY_NAME;
                    break;
                case IACS_Q931_CALL_IDENTITY_ID:
                    ie_name = IACS_Q931_CALL_IDENTITY_NAME;
                    break;
                case IACS_Q931_CALL_STATE_ID:
                    ie_name = IACS_Q931_CALL_STATE_NAME;
                    break;
                case IACS_Q931_CHANNEL_ID_ID:
                    ie_name = IACS_Q931_CHANNEL_ID_NAME;
                    break;
                case IACS_Q931_FACILITY_ID:
                    ie_name = IACS_Q931_FACILITY_NAME;
                    break;
                case IACS_Q931_PROGRESS_INDICATOR_ID:
                    ie_name = IACS_Q931_PROGRESS_INDICATOR_NAME;
                    break;
                case IACS_Q931_NETWORK_SPECIFIC_FACILITY_ID_ID:
                    ie_name = IACS_Q931_NETWORK_SPECIFIC_FACILITY_ID_NAME;
                    break;
                case IACS_Q931_NOTIFICATION_INDICATION_ID:
                    ie_name = IACS_Q931_NOTIFICATION_INDICATION_NAME;
                    break;
                case IACS_Q931_DISPLAY_ID:
                    ie_name = IACS_Q931_DISPLAY_NAME;
                    break;
                case IACS_Q931_DATE_TIME_ID:
                    ie_name = IACS_Q931_DATE_TIME_NAME;
                    break;
                case IACS_Q931_KEYPAD_FACILITY_ID:
                    ie_name = IACS_Q931_KEYPAD_FACILITY_NAME;
                    break;
                case IACS_Q931_SIGNAL_ID:
                    ie_name = IACS_Q931_SIGNAL_NAME;
                    break;
                case IACS_Q931_FEATURE_ACTIVATION_ID:
                    ie_name = IACS_Q931_FEATURE_ACTIVATION_NAME;
                    break;
                case IACS_Q931_FEATURE_INDICATION_ID:
                    ie_name = IACS_Q931_FEATURE_INDICATION_NAME;
                    break;
                case IACS_Q931_SERVICE_PROFILE_ID:
                    ie_name = IACS_Q931_SERVICE_PROFILE_NAME;
                    break;
                case IACS_Q931_ENDPOINT_IDENTIFIER_ID:
                    ie_name = IACS_Q931_ENDPOINT_IDENTIFIER_NAME;
                    break;
                case IACS_Q931_INFORMATION_RATE_ID:
                    ie_name = IACS_Q931_INFORMATION_RATE_NAME;
                    break;
                case IACS_Q931_END_TO_END_TRANSIT_DELAY_ID:
                    ie_name = IACS_Q931_END_TO_END_TRANSIT_DELAY_NAME;
                    break;
                case IACS_Q931_TRANSIT_DELAY_SELECTION_AND_INDICATION_ID:
                    ie_name = IACS_Q931_TRANSIT_DELAY_SELECTION_AND_INDICATION_NAME;
                    break;
                case IACS_Q931_PACKET_LAYER_BINARY_PARAMETERS_ID:
                    ie_name = IACS_Q931_PACKET_LAYER_BINARY_PARAMETERS_NAME;
                    break;
                case IACS_Q931_PACKET_LAYER_WINDOW_SIZE_ID:
                    ie_name = IACS_Q931_PACKET_LAYER_WINDOW_SIZE_NAME;
                    break;
                case IACS_Q931_PACKET_SIZE_ID:
                    ie_name = IACS_Q931_PACKET_SIZE_NAME;
                    break;
                case IACS_Q931_CUG_ID:
                    ie_name = IACS_Q931_CUG_NAME;
                    break;
                case IACS_Q931_REVERSE_CHARGING_INDICATION_ID:
                    ie_name = IACS_Q931_REVERSE_CHARGING_INDICATION_NAME;
                    break;
                case IACS_Q931_CONNECTED_NUMBER_ID:
                    ie_name = IACS_Q931_CONNECTED_NUMBER_NAME;
                    break;
                case IACS_Q931_CONNECTED_SUBADDRESS_ID:
                    ie_name = IACS_Q931_CONNECTED_SUBADDRESS_NAME;
                    break;
                case IACS_Q931_INFORMATION_REQUEST_ID:
                    ie_name = IACS_Q931_INFORMATION_REQUEST_NAME;
                    break;
                case IACS_Q931_CALLING_PARTY_NUMBER_ID:
                    ie_name = IACS_Q931_CALLING_PARTY_NUMBER_NAME;
                    break;
                case IACS_Q931_CALLING_PARTY_SUBADDRESS_ID:
                    ie_name = IACS_Q931_CALLING_PARTY_SUBADDRESS_NAME;
                    break;
                case IACS_Q931_CALLED_PARTY_NUMBER_ID:
                    ie_name = IACS_Q931_CALLED_PARTY_NUMBER_NAME;
                    break;
                case IACS_Q931_CALLED_PARTY_SUBADDRESS_ID:
                    ie_name = IACS_Q931_CALLED_PARTY_SUBADDRESS_NAME;
                    break;
                case IACS_Q931_REDIRECTING_NUMBER_ID:
                    ie_name = IACS_Q931_REDIRECTING_NUMBER_NAME;
                    break;
                case IACS_Q931_REDIRECTING_SUBADDRESS_ID:
                    ie_name = IACS_Q931_REDIRECTING_SUBADDRESS_NAME;
                    break;
                case IACS_Q931_REDIRECTION_NUMBER_ID:
                    ie_name = IACS_Q931_REDIRECTION_NUMBER_NAME;
                    break;
                case IACS_Q931_TRANSIT_NETWORK_SELECTION_ID:
                    ie_name = IACS_Q931_TRANSIT_NETWORK_SELECTION_NAME;
                    break;
                case IACS_Q931_RESTART_INDICATOR_ID:
                    ie_name = IACS_Q931_RESTART_INDICATOR_NAME;
                    break;
                case IACS_Q931_LLC_ID:
                    ie_name = IACS_Q931_LLC_NAME;
                    break;
                case IACS_Q931_HLC_ID:
                    ie_name = IACS_Q931_HLC_NAME;
                    break;
                case IACS_Q931_USER_USER_ID:
                    ie_name = IACS_Q931_USER_USER_NAME;
                    break;
                case IACS_Q931_PARTY_CATEGORY_ID:
                    ie_name = IACS_Q931_PARTY_CATEGORY_NAME;
                    break;
                case IACS_Q931_OPSYSACCESS_ID:
                    ie_name = IACS_Q931_OPSYSACCESS_NAME;
                    break;
                case IACS_Q931_CALL_APPEARANCE_ID:
                    ie_name = IACS_Q931_CALL_APPEARANCE_NAME;
                    break;
                case IACS_Q931_DISPLAY_TEXT_ID:
                    ie_name = IACS_Q931_DISPLAY_TEXT_NAME;
                    break;
                default:
                    ie_name = IACS_Q931_UNKNOWN_IE_NAME;
                    break;
            }
        }
        else
        {
            ret = 0;
            trc_printf(trace_flag, "Invaid IE len:%d\r\n", ie_len);
        }
    }

    if (ret)
    {
        if (ie_len > 0)
        {
            trc_printf(trace_flag,  "* %s IE, length:%d, content[0x]:", ie_name, ie_len);

            for (i = 0; (i < ie_len) && (i < 16); i++)
            {
                trc_printf(trace_flag,  "%02x ", q931_ies_ptr[2+i]);
            }

            if (i < ie_len)
            {
                trc_printf(trace_flag,  "... ");
            }

            trc_printf(trace_flag,  "\r\n");
        }
        else
        {
            trc_printf(trace_flag,  "* %s IE\r\n", ie_name);
        }
    }
    
    return (ret);
}