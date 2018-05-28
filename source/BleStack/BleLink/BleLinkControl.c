/*
* (The MIT License)

* Copyright (c) 2018 Feng Xun

* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* 'Software'), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:

* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/

#include "../../../include/DhGlobalHead.h"

#define BLE_LINK_CONTROL_DEBUG

#if !defined(BLE_LINK_CONTROL_DEBUG)
#undef DEBUG_INFO
#define DEBUG_INFO(...)
#undef DEBUG_DATA
#define DEBUG_DATA(...)
#endif

#define LL_CONNECTION_UPDATE_REQ		0x00
#define LL_CHANNEL_MAP_REQ				0x01
#define LL_TERMINATE_IND				0x02
#define LL_ENC_REQ						0x03
    #define LL_ENC_RAND_LEN             (0x08)
    #define LL_ENC_EDIV_LEN             (0x02)
    #define LL_ENC_SKD_LEN              (0x08)
    #define LL_ENC_IV_LEN               (0x04)
#define LL_ENC_RSP						0x04
#define LL_START_ENC_REQ				0x05
#define LL_START_ENC_RSP				0x06
#define LL_UNKNOWN_RSP					0x07

#define LL_FEATURE_REQ					0x08
#define LL_FEATURE_RSP					0x09
	#define LL_FEATURE_SET_SIZE			0x08
	
#define LL_PAUSE_ENC_REQ				0x0A
#define LL_PAUSE_ENC_RSP				0x0B
#define LL_VERSION_IND					0x0C
	#define LL_VERSION_SIZE				0x05
#define LL_REJECT_IND					0x0D

#define FEATURE_SUPPORT_LE_ENCY         (1<<0x00)

#define LL_VERSION_COMPID				(0xFFFF)
#define LL_VERSION_VERSNR				(BLE_VERSION_NUMBER)
#define LL_VERSION_SUBVERSNR			(DH_BLE_VERSION)


#define TIMEOUT_PRT						(40000)		// 40s procedure response timeout


static u4 LinkFeatureReqHandle(u1	*pu1FearureSet, u2 len)
{
	BlkHostToLinkData rspData;
	
	if( NULL==pu1FearureSet || LL_FEATURE_SET_SIZE!=len )
	{
		return ERR_LINK_INVALID_PARAMS;
	}

	// 目前只支持4.0的加密而已,不用管对方的feature了
	memset(rspData.m_pu1HostData, 0x00, BLE_PDU_LENGTH-BLE_PDU_HEADER_LENGTH);
	rspData.m_pu1HostData[0] = LL_FEATURE_RSP;
	rspData.m_pu1HostData[1] |=  FEATURE_SUPPORT_LE_ENCY;
	rspData.m_u2Length = LL_FEATURE_SET_SIZE + 1;	//opcode
	// 链路控制目前也用这个接口好了

	rspData.m_u1PacketFlag = CONTROL_PACKET;
	DEBUG_INFO("recv feature");
	BleHostDataToLinkPush(rspData);
	
	return DH_SUCCESS;
}

static u4 LinkVersionIndHandle(u1 *peerVersion, u2 len)
{
	BlkHostToLinkData rspData;
	u2	rspLen = 0;
	
	if( NULL==peerVersion || LL_VERSION_SIZE!=len )
	{
		return ERR_LINK_INVALID_PARAMS;
	}

	// 目前只支持4.0,不用看对方version了
	memset(rspData.m_pu1HostData, 0x00, BLE_PDU_LENGTH-BLE_PDU_HEADER_LENGTH);
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_IND;
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_VERSNR;
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_COMPID&0xff;
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_COMPID>>8;
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_SUBVERSNR&0xff;
	rspData.m_pu1HostData[rspLen++] = LL_VERSION_SUBVERSNR>>8;
	
	rspData.m_u2Length = rspLen;	//opcode
	rspData.m_u1PacketFlag = CONTROL_PACKET;
	// 链路控制目前也用这个接口好了
	BleHostDataToLinkPush(rspData);
	return DH_SUCCESS;
}


static u4 LinkRspUnknown(u1 opcode)
{
	BlkHostToLinkData rspData;
	u2	rspLen = 0;
	
	memset(rspData.m_pu1HostData, 0x00, BLE_PDU_LENGTH-BLE_PDU_HEADER_LENGTH);
	rspData.m_pu1HostData[rspLen++] = LL_UNKNOWN_RSP;
	rspData.m_pu1HostData[rspLen++] = opcode;
	
	rspData.m_u2Length = rspLen;	//opcode
	rspData.m_u1PacketFlag = CONTROL_PACKET;
	// 链路控制目前也用这个接口好了
	return BleHostDataToLinkPush(rspData);
}

/**
 *@brief: 		LinkStartEncReq
 *@details:		加密启动请求
 *@param[in]	void  
 *@retval:		DH_SUCCESS
 */
static u4 LinkStartEncReq(void)
{
    BlkHostToLinkData encStartRep;

    encStartRep.m_pu1HostData[0] = LL_START_ENC_REQ;
    encStartRep.m_u1PacketFlag = CONTROL_PACKET;
    encStartRep.m_u2Length = 1;

    return BleHostDataToLinkPush(encStartRep);
}

/**
 *@brief: 		LinkStartEncRsp
 *@details:		加密启动请求
 *@param[in]	void  
 *@retval:		DH_SUCCESS
 */
static u4 LinkStartEncRsp(void)
{
    BlkHostToLinkData encStartRep;

    DEBUG_INFO("recv start enc rsp");
    encStartRep.m_pu1HostData[0] = LL_START_ENC_RSP;
    encStartRep.m_u1PacketFlag = CONTROL_PACKET;
    encStartRep.m_u2Length = 1;

    return BleHostDataToLinkPush(encStartRep);
}

/**
 *@brief: 		CheckLinkChannelMapUpdateReq
 *@details:		检查链路控制请求是否是channel map update
 *@param[in]	pu1PDU              链路PDU
 *@param[out]	pu1NewChannelMap    返回新的通道映射
 *@param[out]	u2Instant           返回通道改变的时间点

 *@note 通道更新请求需要立刻处理，虽然规范要求了对方必须需要给的缓冲时间需要多于6个间隔，
        但是链路数据都是通过下半部处理的，延迟不确定。所以不放在 BleLinkControlHandle 函数中处理
        直接在链路数据接收中断里处理
 *@retval:		DH_SUCCESS
 */
u4 CheckLinkChannelMapUpdateReq(u1 *pu1PDU, u1 *pu1NewChannelMap, u2 *pu2Instant)
{
    u1  llid;
    u1  opcode;
    u2  len;
    u1  index = 0;
    if( NULL==pu1PDU || NULL==pu1NewChannelMap || NULL==pu2Instant)
    {
        return ERR_LINK_INVALID_PARAMS;
    }

    llid = pu1PDU[index++]&0x03;
    len = pu1PDU[index++]&0x1f;
    if ( LLID_CONTROL==llid && len>0 )
    {
        opcode = pu1PDU[index++];
        if( LL_CHANNEL_MAP_REQ == opcode )
        {
            memcpy(pu1NewChannelMap, pu1PDU+index, BLE_CHANNEL_MAP_LEN);
            index += BLE_CHANNEL_MAP_LEN;
            *pu2Instant = pu1PDU[index++];
            *pu2Instant += ((pu1PDU[index]<<8)&0xFF00);
            return DH_SUCCESS;
        }
    }

    return ERR_LINK_NOT_CHANNEL_MAP_REQ;    // 不是channel map控制请求
}

/**
 *@brief: 		CheckLinkConnUpdateReq
 *@details:		链路连接参数更新，虽然规范要求了对方必须需要给的缓冲时间需要多于6个间隔，
                但是链路数据都是通过下半部处理的，延迟不确定。所以不放在 BleLinkControlHandle 函数中处理
                直接在链路数据接收中断里处理，直接检查是否是链路更新控制，如果是就直接处理
 *@param[in]	pu1PDU          链路PDU        
 *@param[in]	u1WinSize       新窗口大小    
 *@param[in]	u2WinOffset     新窗口偏移
 *@param[in]	u2Interval      新连接间隔
 *@param[in]	u2Latency       新从机延迟
 *@param[in]	u2Timeout       新超时
 *@param[in]	u2Instant       连接参数更新的时间点

 *@retval:		DH_SUCCESS
 */

u4 CheckLinkConnUpdateReq(u1 *pu1PDU, u1 *u1WinSize, u2 *u2WinOffset, u2 *u2Interval, u2 *u2Latency, u2 *u2Timeout, u2 *u2Instant)
{
    u1  llid;
    u1  opcode;
    u2  len;
    u1  index = 0;

    if( NULL==pu1PDU || NULL==u1WinSize || NULL==u2WinOffset || NULL==u2Interval || NULL==u2Latency || NULL==u2Timeout || NULL==u2Instant )
    {
        return ERR_LINK_INVALID_PARAMS;
    }
    
    llid = pu1PDU[index++]&0x03;
    len = pu1PDU[index++]&0x1f;
    if ( LLID_CONTROL==llid && len>0 )
    {
        opcode = pu1PDU[index++];
        if( LL_CONNECTION_UPDATE_REQ == opcode )
        {
            *u1WinSize = pu1PDU[index++];
            *u2WinOffset = pu1PDU[index++]; *u2WinOffset += (((u2)pu1PDU[index++]<<8)&0xFF00);
            *u2Interval = pu1PDU[index++]; *u2Interval += (((u2)pu1PDU[index++]<<8)&0xFF00);
            *u2Latency = pu1PDU[index++]; *u2Latency += (((u2)pu1PDU[index++]<<8)&0xFF00);
            *u2Timeout = pu1PDU[index++]; *u2Timeout += (((u2)pu1PDU[index++]<<8)&0xFF00);
            *u2Instant = pu1PDU[index++]; *u2Instant += (((u2)pu1PDU[index++]<<8)&0xFF00);
            return DH_SUCCESS;
        }
    }

    return ERR_LINK_NOT_CONN_UPDATE_REQ;    // 不是conn update控制请求
}


static u4 LinkTerminateHandle(u1 *pu1Data, u2 len)
{
    BleDisconnCommHandle(pu1Data[0]);
 
    return DH_SUCCESS;
}


 u4 LinkEncReqHandle(u1 *pu1Data, u2 len)
{
    u1  flag = 0;
    u1  pu1SlaveIV[LL_ENC_IV_LEN];  /* LSB */
    u1  pu1SlaveSKD[LL_ENC_SKD_LEN];/* LSB */
    u1  pu1SK[BLE_ENC_KEY_SIZE];    /* LSB 会话秘钥*/
    u1  pu1LTK[BLE_ENC_KEY_SIZE];   /* LSB */
    u1  pu1IV[BLE_ENC_IV_SIZE];     /* LSB */
    u1  pu1SKD[BLE_ENC_SKD_SIZE];   /* LSB */
    u2  rspLen;
    BlkHostToLinkData encRsp;
    BlkBleEvent bleEvent;

//    DEBUG_INFO("master RAND:");
//    DEBUG_DATA(pu1Data, 8);
//    DEBUG_INFO("master EDIV:");
//    DEBUG_DATA(pu1Data+LL_ENC_RAND_LEN, 2);
//    DEBUG_INFO("master SKD:");
//    DEBUG_DATA(pu1Data+LL_ENC_RAND_LEN+LL_ENC_EDIV_LEN, 8);
//    DEBUG_INFO("master IV:");
//    DEBUG_DATA(pu1Data+LL_ENC_RAND_LEN+LL_ENC_EDIV_LEN+LL_ENC_SKD_LEN, 4);   

    /*
        IV = IVm || IVs     The least significant octet of IVm
                            becomes the least significant octet of IV. The most significant octet of IVs
                            becomes the most significant octet of IV

        SKD = SKDm || SKDs  The least significant octet of SKDm
                            becomes the least significant octet of SKD. The most significant octet of SKDs
                            becomes the most significant octet of SKD.

        SK = Encrypt(LTK, SKD)
    */

    BleSmGetLtk(pu1Data, pu1Data+LL_ENC_RAND_LEN, pu1LTK, &flag);
    DEBUG_INFO("get LTK:");
    DEBUG_DATA(pu1LTK, 16);
    
    DhGetRand(pu1SlaveIV, LL_ENC_IV_LEN);
    memcpy(pu1IV, pu1Data+LL_ENC_RAND_LEN+LL_ENC_EDIV_LEN+LL_ENC_SKD_LEN, LL_ENC_IV_LEN);
    memcpy(pu1IV+LL_ENC_IV_LEN, pu1SlaveIV, LL_ENC_IV_LEN);

    DhGetRand(pu1SlaveSKD, LL_ENC_SKD_LEN);
    memcpy(pu1SKD, pu1Data+LL_ENC_RAND_LEN+LL_ENC_EDIV_LEN, LL_ENC_SKD_LEN);
    memcpy(pu1SKD+LL_ENC_SKD_LEN, pu1SlaveSKD, LL_ENC_SKD_LEN);

    rspLen = 0;
	encRsp.m_pu1HostData[rspLen++] = LL_ENC_RSP;
	memcpy(encRsp.m_pu1HostData+rspLen, pu1SlaveSKD, LL_ENC_SKD_LEN);
	rspLen += LL_ENC_SKD_LEN;
	memcpy(encRsp.m_pu1HostData+rspLen, pu1SlaveIV, LL_ENC_IV_LEN);
	rspLen += LL_ENC_IV_LEN;
	encRsp.m_u2Length = rspLen;
	encRsp.m_u1PacketFlag = CONTROL_PACKET;
    BleHostDataToLinkPush(encRsp);

//    DEBUG_INFO("slave SKD:");
//    DEBUG_DATA(pu1SlaveSKD, LL_ENC_SKD_LEN);
//    DEBUG_INFO("slave IV:");
//    DEBUG_DATA(pu1SlaveIV, LL_ENC_IV_LEN);

    memset(&bleEvent, 0x00, sizeof(bleEvent));
    if(flag)
    {
        DhAesEnc(pu1SKD, pu1LTK, pu1SK);
        LinkEncInfoCfg(pu1SK, pu1IV, pu1SKD);
//        DEBUG_INFO("session key:");
//        DEBUG_DATA(pu1SK, 16);

        bleEvent.m_u2EvtType = BLE_EVENT_SM_ENC_COMPLETE;
        LinkStartEncReq();
    }
    else
    {
        /* 需要上层设置 LTK */
        LinkEncInfoCfg(NULL, pu1IV, pu1SKD);
        bleEvent.m_u2EvtType = BLE_EVENT_SM_LTK_REQ;
        memcpy(bleEvent.m_event.m_blkSmLtkReq.m_pu1Rand, pu1Data, LL_ENC_RAND_LEN);
        memcpy(bleEvent.m_event.m_blkSmLtkReq.m_pu1Ediv, pu1Data+LL_ENC_RAND_LEN, LL_ENC_EDIV_LEN);
    }
    BleEventPush(bleEvent);

    return DH_SUCCESS;
}

u4 BleLinkControlHandle(u1 *pu1Data, u2 len)
{
	u1	opcode;

	opcode = pu1Data[0x00];

	switch(opcode)
	{
        case LL_CONNECTION_UPDATE_REQ:
        break;
	
		case LL_FEATURE_REQ:
			LinkFeatureReqHandle(pu1Data+0x01, len-1);
		break;
		
		case LL_CHANNEL_MAP_REQ:
		break;

		case LL_TERMINATE_IND:
			LinkTerminateHandle(pu1Data+0x01, len-1);
		break;
		
		case LL_ENC_REQ:
	        LinkEncReqHandle(pu1Data+0x01, len-1);
		break;
		
		case LL_START_ENC_REQ:
		break;
		
		case LL_START_ENC_RSP:
		    LinkStartEncRsp();
		break;
		
		case LL_PAUSE_ENC_REQ:
		break;
		
		case LL_VERSION_IND:
			LinkVersionIndHandle(pu1Data+0x01, len-1);
		break;
		
		case LL_REJECT_IND:
		break;

		default:
			LinkRspUnknown(opcode);
		break;
	}

	return DH_SUCCESS;
}
