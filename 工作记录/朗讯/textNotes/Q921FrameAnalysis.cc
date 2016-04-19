/*921桢分析函数备注解析
*pData的格式：
*   L1~L2:  hdlc_frame_len (BYTE 0, 1) + inteface id (BYTE 2,3) + hdlc frame (sapi + tei ...)
*   
*/
void TxStatisticsFrame(unsigned char *pData)
{
    unsigned char flag = 0;
    u8 sapi = (pData[4]>>2)&0x3f;
    u8 tei  = (pData[5]>>1)&0x7f;
    char up = pData[2];
    //flag是否打印log
    if(userport_print[up] == 1) flag = true;
    else flag = false;
    //0表示该桢是呼叫控制过程，63表示该桢是第二层的管理控制
    if((sapi == 63) || (sapi ==0))   
    {//pData && EA0==0 &&EA1==1;  EA为地址扩展比特，这里需要pData[5]为最后一个地址比特组
        if (pData && (~pData[4] & 0x01) && (pData[5] & 0x01))   
        {
            //获取CR命令/响应比特（收发端）  和  control中的第三比特（判断是哪种类型的桢，I/S/U）
            switch (( (((pData[4] & 0x02)==0)? 0x02: 0) << 1) | pData[6] & 0x03) 
            {
                case 0x01:
                case 0x05:   //这两种case表示是S格式的监视桢，包含命令/响应
                    switch (pData[6] & 0xff)
                    {
                        case 0x01:   //指令：准备接收命令/响应
                            TxFramestaticsresult.rr_count++;
                            if(!flag) break;
                            char trans_nr = ( pData[7] >> 1) & 0x7f;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans RR , nr %d\n\r\n\r", up, tei, trans_nr);
                            
                            /*trc_printf(iacs_trace_g,"\n\r0x%08x:", pData);
                            for(int i=0; i<8; i++)
                            {
                                trc_printf(iacs_trace_g,"%02x ", (u8)pData[i]);
                            }
                            trc_printf(iacs_trace_g,"\n\r");*/
                            break;
                        case 0x05:   //指令：未准备好接收命令/响应
                            TxFramestaticsresult.rnr_count++;
                            if(!flag) break;
                            char rnr_nr = ( pData[7] >> 1) & 0x7f;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans RNR , nr %d\n\r\n\r", up, tei, rnr_nr);
                            break;
                        case 0x09: //指令：拒绝接收命令/响应
                            TxFramestaticsresult.rej_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans REJ \n\r\n\r", up, tei);
                            break;
                        default:
                            TxFramestaticsresult.other_count++;
                            break;
                    }
                    break;
                case 0x03: //该case表示是U格式的无编号命令桢
                    switch (pData[6] & 0xef) 
                    {
                        case 0x6F:  //指令：sabme指令，扩展异步平衡方式
                            TxFramestaticsresult.sabme_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans SABME \n\r\n\r", up, tei);
                            break;
                        case 0x03: //指令：无编号信息命令
                            TxFramestaticsresult.ui_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans UI \n\r\n\r", up, tei);
                            break;
                        case 0x43:  //指令：断开命令
                            TxFramestaticsresult.disc_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans DISC \n\r\n\r", up, tei);
                            break;
                        case 0xAF:  //指令： XID交换身份鉴别命令
                            TxFramestaticsresult.xid_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans XID \n\r\n\r", up, tei);
                            break;
                        default:
                            //Invalid Codes
                            TxFramestaticsresult.other_count++;
                            break;
                    }
                    break;
                case 0x07:  //该case表示是U格式的无编号响应桢
                    switch (pData[6] & 0xef) 
                    {
                        case 0x0F://指令：断开方式响应
                            TxFramestaticsresult.dm_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans DM \n\r\n\r", up, tei);
                            break;
                        case 0x63: //指令： 无编号确认响应
                            TxFramestaticsresult.ua_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans UA \n\r\n\r", up, tei);
                            break;
                        case 0x87://指令：桢拒绝响应，差错
                            TxFramestaticsresult.frmr_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans FRMR \n\r\n\r", up, tei);
                            break;
                        case 0xAF://指令： XID交换身份鉴别响应
                            TxFramestaticsresult.xid_count++;
                            if(!flag) break;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans XID \n\r\n\r", up, tei);
                            break;
                        default:
                            TxFramestaticsresult.other_count++;
                            break;
                    }
                    break;
                default:  //如果不是上述桢类型，默认作为I信息传送桢或其他未知桢处理
                    if (!(pData[4] & 0x02))//根据C/R比特判断，0表示是命令桢，所以是I信息桢（I只能是命令桢）
                        TxFramestaticsresult.other_count++;
                    else
                    {
                        TxFramestaticsresult.i_count++;
                        
                        if(flag == true)
                        {
                            char i_ns = ( pData[6] >> 1) & 0x7f;
                            char i_nr = ( pData[7] >> 1) & 0x7f;
                            trc_printf(q921_trace_g," Q921, UP %d, TEI %d, Trans I , ns %d, nr %d\n\r\n\r", up, tei, i_ns, i_nr);
                            trc_printf(ictt_trace_g," Q931, UP %d, TEI %d, Trans I :\n\r", up, tei);
                            u8 length = pData[0];
                            PrintPkt(ictt_trace_g, pData+4, (unsigned int)length);
                        }
                        /*u8 length = pData[0];
                        trc_printf(iacs_trace_g,"\n\r0x%08x:", pData);
                        for(int i=0; i<length+4; i++)
                        {
                            trc_printf(iacs_trace_g,"%02x ", (u8)pData[i]);
                        }
                        trc_printf(iacs_trace_g,"\n\r");*/
                        //判断Q931传来的信息桢的具体内容，即931的各种通话指令
                        /*call establishment*/
                        if(pData[11] == 0x01)
                        {
                            TxQ931staticsresult.i_alerting++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN ALERTING \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x02)
                        {
                            TxQ931staticsresult.i_callproceding++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN CALLPROCEDING \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x07)
                        {
                            TxQ931staticsresult.i_connect++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN CONNECT \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x0f)
                        {
                            TxQ931staticsresult.i_connectacknowledge++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN CONNECT_ACK \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x03)
                        {
                            TxQ931staticsresult.i_progress++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN PROGRESS \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x05)
                        {
                            TxQ931staticsresult.i_setup++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SETUP \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x0d)
                        {
                            TxQ931staticsresult.i_setupacknowledge++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SETUP_ACK \n\r\n\r", up, tei);
                        }
                        
                        /*call information*/
                        else if(pData[11] == 0x26)
                        {
                            TxQ931staticsresult.i_resume++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RESUME \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x2e)
                        {
                            TxQ931staticsresult.i_resumeacknowledge++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RESUME_ACK \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x22)
                        {
                            TxQ931staticsresult.i_resumereject++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RESUME_REJECT \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x25)
                        {
                            TxQ931staticsresult.i_suspend++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SUSPEND \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x2d)
                        {
                            TxQ931staticsresult.i_suspendacknowledge++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SUSPEND_ACK \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x21)
                        {
                            TxQ931staticsresult.i_suspendreject++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SUSPEND_REJECT \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x20)
                        {
                            TxQ931staticsresult.i_userinformation++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN USERINFORMATION \n\r\n\r", up, tei);
                        }
                        
                        /*release call*/
                        else if(pData[11] == 0x45)
                        {
                            TxQ931staticsresult.i_disconnect++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN DISCONNECTION \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x4d)
                        {
                            TxQ931staticsresult.i_release++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RELEASE \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x5a)
                        {
                            TxQ931staticsresult.i_releasecomplete++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RELEASE_COMP \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x4b)
                        {
                            TxQ931staticsresult.i_restart++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RESTART \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x4e)
                        {
                            TxQ931staticsresult.i_restartacknowledge++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN RESTART_ACK \n\r\n\r", up, tei);
                        }
                        
                        /*other information*/
                        else if(pData[11] == 0x60)
                        {
                            TxQ931staticsresult.i_segment++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN SEGMENT \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x79)
                        {
                            TxQ931staticsresult.i_congestioncontrol++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN CONGESTION_CONTROL \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x7b)
                        {
                            TxQ931staticsresult.i_information++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN INFORMATION \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x62)
                        {
                            TxQ931staticsresult.i_facility++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN FACILITY \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x6e)
                        {
                            TxQ931staticsresult.i_notify++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN NOTIFY \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x7d)
                        {
                            TxQ931staticsresult.i_status++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN STSTUS \n\r\n\r", up, tei);
                        }
                        else if(pData[11] == 0x75)
                        {
                            TxQ931staticsresult.i_statusenquiry++;
                            if(!flag) break;
                            trc_printf(call_trace_g," Q931, Uerport %d, TEI %d, DOWN STSTUSENQUIRY \n\r\n\r", up, tei);
                        }
                    }
                    break;
            }
        }
    }
}