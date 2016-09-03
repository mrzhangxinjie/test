/*<FH>*************************************************************************
* 版权所有 (C)2013 北京比邻信通
* 模块名称: CCM
* 文件名称: ccm_cim_sibroadcastcomponent.cpp
* 内容摘要: 本文件主要实现广播的配置和更新，ETWS，CMAS等告警
* 其它说明:
* 当前版本: V3.1
**    
*    
* 修改记录1:
*    
**<FH>************************************************************************/

/*****************************************************************************/
/*               #include（依次为标准库头文件、非标准库头文件）              */
/*****************************************************************************/
/* ================================== OSS头文件 ============================*/
#ifdef VS2008
extern "C"{
#include "EUTRA-RRC-Definitions.h"
#include "OSS_Timer.h"
}
#else
#include "EUTRA-RRC-Definitions.h"
#endif
#include "pub_usf_ssl.h"
#include "usf_bpl_pub.h"


/* ============================= 各个子系统公共头文件 ======================*/
#include "pub_lte_rnlc_oam_interface.h"
#include "pub_lte_rnlc_bb_interface.h"
#include "pub_lte_rnlc_rnlu_interface.h"
#include "pub_lte_rnlc_rnlu_fddv20_compatible.h"
#include "pub_lte_rnlc_rnlu_tddv20_compatible.h"
#include "pub_lte_event_def_fddv20_compatible.h"
#include "pub_lte_event_def_tddv20_compatible.h"
#include "pub_lte_rnlc_rrm_interface.h"
#include "pub_lte_dbs.h"
#include "pub_lte_event.h"
#include "pub_lte_global_def.h"

/* =============================RNLC内各个模块公共头文件 ===================*/


/* ==============================CCM模块公共头文件==========================*/
#include "ccm_common.h"
#include "ccm_cmm_struct.h"
#include "ccm_timer.h"
#include "ccm_error.h"
#include "ccm_cim_common.h"
#include "ccm_eventdef.h"
#include "ccm_debug.h"
#include "ccm_print.h"

/* ============================CIM模块公共头文件=========================== */
#include "ccm_cim_sibroadcastcomponent.h"

/* TB Size表选择{0:仅支持DCI-1A；1:仅支持DCI-1C; 2:支持DCI-1A/1C} */
extern WORD32 g_dwTbSizeMode;

/* 该全局变量是用来控制调试的时候是否调试外部系统的标志 */
extern T_SystemDbgSwitch g_tSystemDbgSwitch;

/* 打印信息开关，1:打开*/
extern WORD32 g_dwCimPrintFlag;
 /*标示sib11是按照协议的方式发送(0)还是日本需要方式发送(1)*/
extern BYTE  g_ucSib11NonSuspend; 
 /*标示用户面收到sib11时是立即发送(1) 还是按照修改周期发送(0)*/
extern BYTE  g_ucSib11WarnMsgSendType;

extern BYTE g_ucSceneCfg;//航线高铁模式标识:0，正常，1，航线，2，高铁
 
WORD32  g_dwRnlcCcmDbgSib10TimerLen = 6000;

WORD32 g_dwAcSigDebug = 0;
WORD32 g_dwAcDataDebug = 0;

WORD32 g_dwSib10SwtichMode = CIM_SIB10_STOPCBEANDTIMER;/* sib10的停止方式 */

WORD32 g_dwSysTimeAndLongCodeExist = 0; 
WORD32 g_dwDebugSysTimeAndLongCode = 0; 
    
SystemTimeInfoCDMA2000_asynchronousSystemTime g_tSystemTimeInfo_asyn;

extern WORD32 g_dwSiSchSwitch;
extern BOOLEAN g_bCcGPSLockState;
extern BYTE g_ucEpcWarningCheckFlag;
extern WORD32 g_dwS1WarnMsgLen;

extern "C" {

#define SIB8_LOG_SIZE 1000

WORD32 g_temp_dw1XrttLongCodeOffset[SIB8_LOG_SIZE];
WORD32 g_temp_dwSib8SystemTimeInfoOffset[SIB8_LOG_SIZE];
WORD32 g_temp_dwSib8OffsetCounter;
WORD16 g_wLogRtSeqStNum;


void TestProbSib8_Init()
{
    g_temp_dwSib8OffsetCounter = 0 ;
    for (WORD32 i=0 ; i<SIB8_LOG_SIZE ; i++ ) {
        g_temp_dw1XrttLongCodeOffset[i] = 0;
        g_temp_dwSib8SystemTimeInfoOffset[i] = 0;
    }
}

void TestProbeSib8_insert(unsigned long temp_dw1XrttLongCodeOffset , unsigned long dwSib8SystemTimeInfoOffset )
{
    if (g_temp_dwSib8OffsetCounter >= SIB8_LOG_SIZE) {
        return;
    }
    g_temp_dw1XrttLongCodeOffset[g_temp_dwSib8OffsetCounter] = temp_dw1XrttLongCodeOffset ;
    g_temp_dwSib8SystemTimeInfoOffset[g_temp_dwSib8OffsetCounter] = dwSib8SystemTimeInfoOffset;
    
    g_temp_dwSib8OffsetCounter++;
}

    void TestProbeSib8_Show()
    {
        for (int i=0 ; i<(int)g_temp_dwSib8OffsetCounter ; i++) 
        {
            printf("%d: %d,%d\n" , i , g_temp_dw1XrttLongCodeOffset[i] , g_temp_dwSib8SystemTimeInfoOffset[i]);
        }
    }

}

T_CimSIMsgInfoTable g_atCimSIMsgInfoTable[CIM_MAX_ENCODE_NUM] =
{
    {    /* 00 PBCH encode */
        (BROADCAST_DECODER)asn1PD_BCCH_BCH_MessageType,
        (BROADCAST_ENCODER)asn1PE_BCCH_BCH_MessageType,
        "MIB" ,
    },
    {    /* 01 SIB1 encode */
        (BROADCAST_DECODER)asn1PD_BCCH_DL_SCH_MessageType,
        (BROADCAST_ENCODER)asn1PE_BCCH_DL_SCH_MessageType,
        "SIB1" ,
    },
    {    /* 02 SIB2 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType2,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType2,
        "SIB2" ,
    },
    {    /* 03 SIB3 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType3,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType3,
        "SIB3" ,
    },
    {    /* 04 SIB4 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType4,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType4,
        "SIB4" ,
    },
    {    /* 05 SIB5 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType5,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType5,
        "SIB5" ,
    },
    {    /* 06 SIB6 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType6,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType6,
        "SIB6" ,
    },
    {    /* 07 SIB7 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType7,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType7,
        "SIB7" ,
    },
    {    /* 08 SIB8 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType8,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType8,
        "SIB8" ,
    },
    {    /* 09 SIB9 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType9,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType9,
        "SIB9" ,
    },
    {    /* 10 SIB10 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType10,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType10,
        "SIB10" ,
    },
    {    /* 11 SIB11 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType11,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType11,
        "SIB11" ,
    },
    {    /* 11 SIB12 encode */
        (BROADCAST_DECODER)asn1PD_SystemInformationBlockType12_r9,
        (BROADCAST_ENCODER)asn1PE_SystemInformationBlockType12_r9,
        "SIB12_r9" ,
    },
    {    /* 12 SI encode */
        (BROADCAST_DECODER)asn1PD_BCCH_DL_SCH_MessageType,
        (BROADCAST_ENCODER)asn1PE_BCCH_DL_SCH_MessageType,
        "SI" ,
    }
};

const BYTE S_CIM_SIB_IDLE = 0;
const BYTE S_CIM_SIB_WAITUPTRSP = 1;




/*<FUNC>***********************************************************************
* 函数名称: Idle
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Idle(Message *pMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch (pMsg->m_dwSignal)
    {
        case CMD_CIM_MASTER_TO_SLAVE:
        {
            HandleMasterToSlave(pMsg);
            break;
        }

        case CMD_CIM_CELL_SYSINFO_UPDATE_REQ:
        case EV_CMM_CIM_SYSINFO_UPDATE_REQ:
        case EV_CMM_CIM_SYSINFO_BARE_REQ:
        {
            Handle_SystemInfoUpdateReq(pMsg);
            /*载波聚合功能，如果广播更新需要通知dcm*/
            if(EV_CMM_CIM_SYSINFO_UPDATE_REQ == pMsg->m_dwSignal)
            {
                /* 获取广播构件数据区 */
                T_CimSIBroadcastVar *ptCimSIBroadcastVar = 
                (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

                CimSendToDcmBrdcastMsgUpdInd(ptCimSIBroadcastVar->wCellId);
            }
            break;

        }
        case EV_CMM_CIM_S1_WARNING_REQ:
        {
            Handle_S1WarningRsp(pMsg);
            Handle_S1WarningReq(pMsg);
            break;
        }
        case EV_T_CCM_CIM_S1_WARNING_SIB10_TIMEOUT:
        {
            Handle_S1WarningSib10Timeout();
            break;
        }
        case EV_T_CCM_CIM_S1_WARNING_SIB11_TIMEOUT:
        {
            if (NULL == OSS_GetParaFromCurTimer())
            {
                Handle_S1WarningSib11Timeout();
            }
            else
            {
                Handle_S1WarningSib12Timeout();
            }
            break;
        }
        case EV_CMM_CIM_S1_KILL_WARNING_REQ:
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"SI: Receive EV_CMM_CIM_S1_KILL_WARNING_REQ Message!");
            Handle_S1KillWarningRsp(pMsg);
            Handle_S1KillWarningReq(pMsg);
            break;
        }
        default:
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_IDLE,
                                    pMsg->m_dwSignal,
                                    0,
                                    RNLC_ERROR_LEVEL,
                                    " SIBroadcast: Idle Ststus Receive Msg ID = %d! ",
                                    pMsg->m_dwSignal);

            break;
        }
    }

    return;
}

/*<FUNC>***********************************************************************
* 函数名称: WaitSystemInfoUpdateRsp
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::WaitSystemInfoUpdateRsp(Message *pMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch (pMsg->m_dwSignal)
    {
    case EV_RNLU_RNLC_SYSINFO_RSP:
    {
        Handle_SystemInfoUpdateRsp(pMsg);
        break;
    }
    case EV_T_CCM_CIM_SYSINFO_UPDATE_TIMEOUT:
    {
        Handle_SystemInfoUpdateTimeout();
        break;
    }
    case EV_T_CCM_CIM_S1_WARNING_SIB10_TIMEOUT:
    {
        Handle_S1WarningSib10Timeout();
        break;
    }
    case EV_T_CCM_CIM_S1_WARNING_SIB11_TIMEOUT:
    {
        Handle_S1WarningSib11Timeout();
        break;
    }
    default:
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_WAIT_SYSINFO_UPD_RSP,
                                pMsg->m_dwSignal,
                                0,
                                RNLC_ERROR_LEVEL,
                                " SIBroadcast: Wait System Info Update Rsp Ststus Receive Msg ID = %d! ",
                                pMsg->m_dwSignal);
        break;
    }
    }

    return;
}

#if 0
#endif
/*<FUNC>***********************************************************************
* 函数名称: Init
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Init()
{
    memset(&(GetComponentContext()->tCimSIBroadcastVar), 0, sizeof(GetComponentContext()->tCimSIBroadcastVar));

    BYTE  ucBoardMSState = BOARD_MS_UNCERTAIN;
    
    ucBoardMSState = OSS_GetBoardMSState();
    
    if (BOARD_SLAVE == ucBoardMSState)
    {
            CimPowerOnInSlave();
            SetComponentSlave();
            return;
    }

    TranStateWithTag(CCM_CIM_SIBroadcastComponentFsm, Idle,S_CIM_SIB_IDLE);

    return;
}
/*<FUNC>***********************************************************************
* 函数名称: CimSendToDcmBrdcastMsgUpdInd
* 功能描述: 载波聚合功能需求，普通广播更新时给dcm发送更新指示
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSendToDcmBrdcastMsgUpdInd(WORD16 wCellId)
{
    T_CcmDcmCellBroadcastUpdInd tCcmDcmBrdCastMsgUpdInd;
    PID tDcmMainPid;
    WORD32  dwFuncRst = RNLC_FAIL;

    memset(&tCcmDcmBrdCastMsgUpdInd, 0x00, sizeof(tCcmDcmBrdCastMsgUpdInd));
    memset(&tDcmMainPid, 0x00, sizeof(tDcmMainPid));

    tCcmDcmBrdCastMsgUpdInd.wCellId = wCellId;
    
    dwFuncRst = CCM_OSS_GetDcmMainPid(&tDcmMainPid);

    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CCM_DCM_CELL_BROADCAST_UPD_IND, 
                                             &tCcmDcmBrdCastMsgUpdInd,
                                             sizeof(tCcmDcmBrdCastMsgUpdInd), 
                                             COMM_RELIABLE, 
                                             &tDcmMainPid);
    if (OSS_SUCCESS != dwOssStatus) 
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }
    CCM_LOG(RNLC_INFO_LEVEL, "Cim sent Msg to Dcm EV_CCM_DCM_CELL_BROADCAST_UPD_IND: %d", EV_CCM_DCM_CELL_BROADCAST_UPD_IND);
    return RNLC_SUCC;
}

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10StopSib11Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
    {
        // 配置了sib11，却没有读取到内容才需要调整SI 列表
        CimAdjustScheInfoList(&(ptAllSibList->tSib1Info.schedulingInfoList), 
                               ptAllSibList->aucSibList, sibType11);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11= 0;
        CCM_CIM_LOG(RNLC_WARN_LEVEL,"\n SI: No schedule info for Sib11!\n");
        return RNLC_SUCC;
    }

    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS SIB10 Stop SIB11 Send!");
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimCauseEtwsSib10Add
* 功能描述: a.读取数据库信息和MIB、SIB1以及其它SIB分别编码:完成参数获取和映射关系调整
*           b.根据映射关系，填写SI:填写具体的SI内容
*           c.SI编码:编码所有SI，若第一个SI(包括SIB2)编码失败，停止广播下发;
*            若其他SI编码成功或失败，打上标记，完成tAllSiEncodeInfo信息填写
*           d.比较广播内容是否更新:和之前下发的广播比较，若广播信息不变，则不需下发
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])
    {
        // 配置了sib10，却没有读取到内容才需要调整SI 列表
        CimAdjustScheInfoList(&(ptAllSibList->tSib1Info.schedulingInfoList), 
                               ptAllSibList->aucSibList, sibType10);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
        CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: No schedule info , Don't Send Sib10!\n");
        return RNLC_SUCC;
    }
    return RNLC_SUCC;
}



WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib11Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )

{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
     CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    
    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11=1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    //映射为对用户面的发送的广播更新的原因
    //ptsysInfoUpdReq->ucCellSIUpdCause = CAUSE_ETWS_START;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
    {
        // 配置了sib10，却没有读取到内容才需要调整SI 列表
        CimAdjustScheInfoList(&(ptAllSibList->tSib1Info.schedulingInfoList), 
                                           ptAllSibList->aucSibList, sibType11);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: No schedule info,Don't Send Sib11!\n");
        return RNLC_SUCC;
    }
    return RNLC_SUCC;
}


/*<FUNC>***********************************************************************
* 函数名称: CimCauseEtwsSib10Sib11Add
* 功能描述: 当epc告警包含sib10和sib11，并且调度信息都配置的情况下的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Sib11Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )

{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11=1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS SIB10 SIB11 Send!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimCauseEtwsSib10Stop
* 功能描述: ETWS 收到sib10 stop的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   //设置标志位表明此种情况下是否停止sib10
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 =0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;

    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS Receive SIB10 Stop!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimCauseEtwsSib10SIB11Stop
* 功能描述: ETWS 收到sib10 sib11 stop的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10SIB11Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   //设置标志位表明此种情况下是否停止sib10
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;

    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS Receive SIB10 SIB11 Stop!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimCauseCmasSib12Add
* 功能描述: CMAS 收到sib12的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);

    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    if(1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[MAX_CMAS_MSG -1].aucIsCurrentBroadcasting)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"SI: CMAS broading Reach MAX number (%u)!",MAX_CMAS_MSG);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12=0;
        return RNLC_FAIL;
    }
    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12=1;   
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
    {
        // 配置了sib12，却没有读取到内容才需要调整SI 列表
        CimAdjustScheInfoList(&(ptAllSibList->tSib1Info.schedulingInfoList), 
                                       ptAllSibList->aucSibList, sibType12_v920);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);        
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        CCM_CIM_LOG(RNLC_WARN_LEVEL, "SI: No SIB12 schedule info, Don't Send!");
        return RNLC_SUCC;
    }
    return RNLC_SUCC;
}


/*<FUNC>***********************************************************************
* 函数名称: CimCauseCmasSib12Upd
* 功能描述: CMAS 收到sib12更新的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Upd(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);

    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12=1; 
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
    {
        // 配置了sib12，却没有读取到内容才需要调整SI 列表
        CimAdjustScheInfoList(&(ptAllSibList->tSib1Info.schedulingInfoList), 
                                           ptAllSibList->aucSibList, sibType12_v920);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        CCM_CIM_LOG(RNLC_WARN_LEVEL, "SI: NO SIB12 Schedule Info(UPD),Don't Send!");
        return RNLC_SUCC;
     }
    return RNLC_SUCC;
}


/*<FUNC>***********************************************************************
* 函数名称: CimCauseCmasSib12Kill
* 功能描述: CMAS 收到sib12kill的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Kill(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimCauseCmasSib12Stop
* 功能描述: CMAS 收到sib12停止的消息的处理
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    //设置标志位表明此种情况下是否下发 sib10 sib11 sib12
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimDealWithWarningConditions
* 功能描述: 根据s1口过来的告警包含信息的情况，对应相关的处理函数
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimDealWithWarningConditions( T_AllSibList  *ptAllSibList, T_SysInfoUpdReq  *ptsysInfoUpdReq,
                                                                     TWarningMsgSegmentList *ptWarningMsgSegmentList,
                                                                     TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptsysInfoUpdReq);
    WORD32 dwSIIndex = RNLC_FAIL;

    /*设置变量dwCheckWarnSchedInfoResult 等于0 表示正常的返回，
    其它情况非0，并通知广播更新流程直接返回*/
    WORD32 dwCheckWarnSchedInfoResult = 0;

    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)(&ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg);

    switch(ptsysInfoUpdReq->ucCellSIUpdCause)
    {
           
    case CIM_CAUSE_ETWS_SIB10_ADD: 
    {
        if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /*1. sib10 告警来临，但是没有sib10的调度信息*/
            dwCheckWarnSchedInfoResult = 1; 
        }
        else
        {
            dwCheckWarnSchedInfoResult = 0;
            CimCauseEtwsSib10Add(ptAllSibList,ptsysInfoUpdReq);
        }
        break;
    }
    case CIM_CAUSE_ETWS_SIB11_ADD:   
    {
        if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
        {
            /*2. sib11 告警来临，但是没有sib11的调度信息*/
            dwCheckWarnSchedInfoResult = 2; 
        }
        else
        {   
            dwCheckWarnSchedInfoResult = 0;
            CimCauseEtwsSib11Add(ptAllSibList,ptsysInfoUpdReq);
        }
        break;
    }
    case CIM_CAUSE_ETWS_SIB10_SIB11_ADD: 
    {
        if((FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])&&
        (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11]))
        {
            /*3. sib10 sib11 告警来临，但是没有sib10 sib11的调度信息*/
            dwCheckWarnSchedInfoResult = 3; 
        }
        else
        {
            dwCheckWarnSchedInfoResult = 0;
            if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])
            {
                CimCauseEtwsSib11Add(ptAllSibList,ptsysInfoUpdReq);
                CCM_CIM_LOG(RNLC_WARN_LEVEL, "SI: SIB10&SIB11 From EPC,But No schedule Info for SIB10!");  
                break;
            }
            if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
            {
                CCM_CIM_LOG(RNLC_WARN_LEVEL, "SI: SIB10&SIB11 From EPC,But No schedule Info for SIB11!"); 
                CimCauseEtwsSib10Add(ptAllSibList,ptsysInfoUpdReq);
                break;
            }        
            /*sib10和sib11都有，并且都有相应的调度信息*/
            CimCauseEtwsSib10Sib11Add(ptAllSibList,ptsysInfoUpdReq);
        }
        break;
    } 
    case CIM_CAUSE_ETWS_SIB10_STOP: 
    {
        CimCauseEtwsSib10Stop(ptAllSibList,ptsysInfoUpdReq);
        break;
    }
    case CIM_CAUSE_ETWS_SIB10_SIB11_STOP: 
    {
        CimCauseEtwsSib10SIB11Stop(ptAllSibList,ptsysInfoUpdReq);
        break;
    }
    case CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD: 
    {
        CimCauseEtwsSib10StopSib11Add(ptAllSibList,ptsysInfoUpdReq);
        break;
    }
    case CIM_CAUSE_CMAS_SIB12_ADD:  
    {
        if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
        {
            /*4. sib12 告警来临，但是没有sib12的调度信息*/
            dwCheckWarnSchedInfoResult = 4; 
        }
        else
        {  
            dwCheckWarnSchedInfoResult = 0; 
            CimCauseCmasSib12Add(ptAllSibList,ptsysInfoUpdReq);
        }
        break;
    }
    case CIM_CAUSE_CMAS_SIB12_UPD: 
    {
        if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
        {
            /*4. sib12 告警更新，但是没有sib12的调度信息*/
            dwCheckWarnSchedInfoResult = 5; 
        }
        else
        {  
            dwCheckWarnSchedInfoResult = 0; 
            CimCauseCmasSib12Upd(ptAllSibList,ptsysInfoUpdReq);
        }
        break;
    }

    case CIM_CAUSE_CMAS_SIB12_KILL: 
    {
        CimCauseCmasSib12Kill(ptAllSibList,ptsysInfoUpdReq);
        break;
    }
    case CIM_CAUSE_CMAS_SIB12_STOP: 
    {
        CimCauseCmasSib12Stop(ptAllSibList,ptsysInfoUpdReq);
        break;
    }
    default: 
    {
       /*非已定义场景，什么都不做*/
        break;
    }

    }

    /*如果此次告警与etws相关，则读取sib11的si周期保存到si上下文中*/
    if((CIM_CAUSE_ETWS_SIB10_ADD <= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    &&(CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD >= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
        {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Deal SIB10 info Now...\n");
                 /*注意这里是用的此次s1口过来的告警组成的sib10信息*/
            CimWarnMsgSib10Hander(ptCpmCcmWarnMsg,ptWarningMsgSegmentList,ptsysInfoUpdReq,ptAllSibList);
        }
            /*如果此次告警包含sib11并且需要发送，则填写sib11信息*/
        if(1== ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Deal SIB11 info Now...\n");
            CimWarnMsgSib11Hander(ptCpmCcmWarnMsg,ptWarningMsgSegmentList,ptsysInfoUpdReq,ptAllSibList);
        }
            
        dwSIIndex = CimGetSchePeriodInfo(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType11);
        if (RNLC_FAIL == dwSIIndex)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Haven't Configure SIB11 shedule info !");  
        }
        else
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod = ptAllSibList->tSib1Info.schedulingInfoList.elem[dwSIIndex].si_Periodicity;
        }
    }
    /*如果此次告警与cmas相关，则读取sib12的si周期保存到si上下文中*/
    else if((CIM_CAUSE_CMAS_SIB12_ADD <= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    &&(CIM_CAUSE_CMAS_SIB12_STOP >= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        /*如果此次告警包含sib12并且需要发送，则填写sib12信息*/
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Deal SIB12 info Now...\n");
            CimWarnMsgSib12Hander(ptCpmCcmWarnMsg,ptCmasWarningMsgSegmentList,ptsysInfoUpdReq,ptAllSibList);
        }
        dwSIIndex = CimGetSchePeriodInfo(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
        if (RNLC_FAIL == dwSIIndex)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Haven't Configure SIB12 shedule info !");  
        }
        else
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucSIB12SiPeriod = ptAllSibList->tSib1Info.schedulingInfoList.elem[dwSIIndex].si_Periodicity;
        }
    }
    else
    {
       CCM_CIM_LOG(RNLC_WARN_LEVEL, "SI: Unknown Inside EPC Warn Msg !");
    }
    return dwCheckWarnSchedInfoResult;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_SystemInfoUpdateReq
* 功能描述: a.读取数据库信息和MIB、SIB1以及其它SIB分别编码:完成参数获取和映射关系调整
*           b.根据映射关系，填写SI:填写具体的SI内容
*           c.SI编码:编码所有SI，若第一个SI(包括SIB2)编码失败，停止广播下发;
*            若其他SI编码成功或失败，打上标记，完成tAllSiEncodeInfo信息填写
*           d.比较广播内容是否更新:和之前下发的广播比较，若广播信息不变，则不需下发
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_SystemInfoUpdateReq(Message *pMsg)
{
    WORD32  dwResult = FALSE;
    WORD16  wCellId = 0;
    BYTE    ucRadioMode = 0;

    T_CIMVar     *ptCIMVar = NULL;
    T_SysInfoUpdReq  *ptsysInfoUpdReq = NULL;

    static T_AllSibList tAllSibList;
    static T_BroadcastStream   tNewBroadcastStream;
    T_AllSiEncodeInfo   tAllSiEncodeInfo;
    T_SysInfoUpdRsp     tSysInfoUpdRsp;
    static TWarningMsgSegmentList tWarningMsgSegmentList;
    static  TCmasWarningMsgSegmentList tCmasWarningMsgSegmentList;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    memset((VOID*)&tAllSibList, 0, sizeof(tAllSibList));
    memset((VOID*)&tNewBroadcastStream, 0, sizeof(tNewBroadcastStream));
    memset((VOID*)&tAllSiEncodeInfo, 0, sizeof(tAllSiEncodeInfo));
    memset((VOID*)&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));
    memset(&tWarningMsgSegmentList, 0, sizeof(tWarningMsgSegmentList));
    memset(&tCmasWarningMsgSegmentList, 0, sizeof(tCmasWarningMsgSegmentList));

    CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\nSI: New Version! \n");
    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    ucRadioMode = ptCIMVar->ucRadioMode;
     /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);    
    ptsysInfoUpdReq = (T_SysInfoUpdReq *)pMsg->m_pSignalPara;
    tAllSibList.ucResv = ptsysInfoUpdReq->ucCellSIUpdCause;
    /*记录下此次广播更新的原因到广播构件数据区*/
      ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause = ptsysInfoUpdReq->ucCellSIUpdCause;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Receive SI Update Cause(%d): %s",ptsysInfoUpdReq->ucCellSIUpdCause,CIMSIUPDATECAUSE(ptsysInfoUpdReq->ucCellSIUpdCause));
    /* 初始化广播实例数据区 */
    CimBroadcastDataInit(ptsysInfoUpdReq);

    wCellId = ptsysInfoUpdReq->wCellId;

    /*将之前的valuetag取出，如果是小区建立，则取出来的为0，否则，则取历史值*/
    BYTE ucLastValueTag = GetComponentContext()->tCimSIBroadcastVar.tSIBroadcastDate.ucSib1ValueTag;
    /*此处增加广播长度为非0的判断，消除因为dbs触发建立导致的小区删建
    如果不是小区建立导致的删建，按照原来的流程执行。先发保存广播，再发更新广播*/
    if((SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)&& (0 != ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength))
    {
        /*针对修改参数引起的删建特殊处理*/
        ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSetupRspFlag = 0;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSIUpdRspFlag =0;
        ptCimSIBroadcastVar->tSIBroadcastDate.dwRspResult = 0;
        dwResult = CimCellDelAddSendOldSIToRnlu();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRnluSIUptOrNot, 
                                    dwResult, 
                                    0, 
                                    RNLC_WARN_LEVEL, 
                                    " FunctionCallFail_SendToRnluSIUptOrNot! ");
            return;
        }
    }
    /*如果没有之前的广播，这里直接更新为一次小区建立的广播*/
    if((SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)&& (0 == ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength))
    {   
        ptsysInfoUpdReq->ucCellSIUpdCause = SI_UPD_CELL_SETUP;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI_UPD_CELL_SETUP result to SI_UPD_CELL_DELADD!\n");
    }
    /* 1.读取数据库信息 */
    dwResult = CimGetAllSystemInfo(wCellId, ucRadioMode, ucLastValueTag, &tAllSibList);
    if (FALSE == dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_SYSINFO_UPD_REQ,
                                dwResult,
                                0,
                                RNLC_ERROR_LEVEL,
                                " SIBroadcast: CIM Get System Info fail! ");
        return;
    }

    ptCimSIBroadcastVar->tSIBroadcastDate.wLogRtSeqStNum = g_wLogRtSeqStNum;
    /*如果广播更新与epc的告警相关*/
    if((ptsysInfoUpdReq->ucCellSIUpdCause >= CIM_CAUSE_ETWS_SIB10_ADD)
    &&(ptsysInfoUpdReq->ucCellSIUpdCause <= CIM_CAUSE_CMAS_SIB12_STOP))
    {
        /*针对s1 不同告警情况进行不同的处理，如果没有s1告警则会返回0*/ 
        dwResult = CimDealWithWarningConditions(&tAllSibList,ptsysInfoUpdReq,&tWarningMsgSegmentList,&tCmasWarningMsgSegmentList);

        if (0 != dwResult)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: No EPC Warn Msg schedle info, SI update Stop! Situation = %d\n",dwResult);
            memset(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg, 0, sizeof(ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg));
            return ;
        }
    }
    /*普通广播更新时对是否填写sib10 信息的处理*/
    CimNmlBrdUpdDealWithSib10Info(&tAllSibList,ptsysInfoUpdReq);
    
    /* 2.MIB、SIB1-sib9 如果存在s1告警 对sib10 和 普通sib在一起编码，如果sib11，sib12 则会对分片进行编码 */
    dwResult = CimEncodeAllSibs_Warning(wCellId, &tAllSibList,
                                    &tNewBroadcastStream,
                                    &tWarningMsgSegmentList,
                                    &tCmasWarningMsgSegmentList,
                                    ptCIMVar,
                                    ptCimSIBroadcastVar);
    if (3 > dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM Encode SIBs fail!\n");
#ifndef VS2008
        return;
#endif
    }

    /* 3.根据映射关系，填写SI */
    dwResult = CimSibMappingToSi(&tAllSibList,
                            &tAllSiEncodeInfo);
    if (FALSE == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM SIB Mapping to SI fail!\n");
#ifndef VS2008
        return;
#endif
    }

    /* 4.SI编码   如果存在s1告警，同时也会对s1告警的si 进行编码 */
    dwResult = CimEncodeAllSi(&tAllSibList,
                        &tAllSiEncodeInfo,
                        &tWarningMsgSegmentList,
                        &tCmasWarningMsgSegmentList);
#ifdef VS2008
    dwResult = TRUE;
#endif
    if (FALSE == dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_SYSINFO_UPD_REQ,
                                dwResult,
                                0,
                                RNLC_ERROR_LEVEL,
                                " SIBroadcast: CIM Encode SI fail! CellId=%d! ",
                          ptCIMVar->wCellId);
        return;
    }

    /* 5.比较广播内容是否更新，如果存在s1告警，则不做检查直接返回 */
    dwResult = CimSibsCheckAndUpdte(ptsysInfoUpdReq, &tAllSibList,&tNewBroadcastStream);
#ifdef VS2008
    dwResult = TRUE;
#endif
    /*如果广播没有更新，根据情况，不发广播*/
        if (FALSE == dwResult)
        {
            CimSibsDealWithNoUpdate(ptsysInfoUpdReq,&tSysInfoUpdRsp);
            CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SI Check No Update! \n");
            return;
         }
        else
        {
            /* 如果有更新则重新编SIB1 生成四个需要发送的sib1码流*/
            dwResult = CimFourEncodeSib1Stream(ptCimSIBroadcastVar, &tAllSibList,&tNewBroadcastStream);
            if(FALSE == dwResult)
            {
                CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: ReEncode Sib1 fail! \n");
#ifndef VS2008
                return ;
#endif
            }
        }

    /* 6.生成发送给用户面的广播码流 */
    dwResult = CimComposeMsgToRnlu(ptsysInfoUpdReq,
                                &tAllSibList,
                                &tNewBroadcastStream,
                                &tAllSiEncodeInfo,
                                &tWarningMsgSegmentList,
                                &tCmasWarningMsgSegmentList);
    if (FALSE == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM Send Msg to Rnlu Fail!\n");
#ifndef VS2008
        return;
#endif
    }
    /* 7。根据不同的情况将组好的码流发送到用户面 */
    dwResult = CimSendBroadcastMsgToRnlu( );
    if (RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: send to RNLU fail!\n");
#ifndef VS2008
        return ;
#endif
    }
    /* 启动系统消息等待定时器，等响应消息 */
    dwResult = CimSetSystemInfoTimer();
    if (RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: CIM Set System Info Timer Fail!\n");
#ifndef VS2008
        return ;
#endif
    }

    /**如果不调试用户面，这里直接返回*/
    if (0 == g_tSystemDbgSwitch.ucRNLUDbgSwitch)
    {
        /*自己构造用户面的响应消息*/
        Message tNoDbgRNLU;
        T_RnluRnlcSystemInfoRsp tRnluRnlcSystemInfoRsp;
        tRnluRnlcSystemInfoRsp.dwResult = 0;
        tRnluRnlcSystemInfoRsp.wCellId = ptCimSIBroadcastVar->wCellId;
        tNoDbgRNLU.m_pSignalPara = static_cast<void*>(&tRnluRnlcSystemInfoRsp);
        Handle_SystemInfoUpdateRsp(&tNoDbgRNLU);
        return ;
    }
    /* 状态迁移到等待响应消息 */
    TranState(CCM_CIM_SIBroadcastComponentFsm, WaitSystemInfoUpdateRsp);

    return;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_SystemInfoUpdateRsp
* 功能描述: 如果是小区建立的广播消息，收到响应消息后将响应结果转发给建立构件
*           如果不是建立引起的广播更新，响应消息成功时回对应的构件响应消息
*           响应消息失败时，进入重传机制
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_SystemInfoUpdateRsp(Message *pMsg)
{
    WORD32 dwResult = 0;
    T_SysInfoUpdRsp tSysInfoUpdRsp;
    T_RnluRnlcSystemInfoRsp *ptRnluRnlcSystemInfoRsp = NULL;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_CIMVar     *ptCIMVar = NULL;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    memset((VOID*)&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));

    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* 获取CIM小区广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* 获取响应消息具体信息 */
    ptRnluRnlcSystemInfoRsp = (T_RnluRnlcSystemInfoRsp *)pMsg->m_pSignalPara;
    tSysInfoUpdRsp.dwResult = ptRnluRnlcSystemInfoRsp->dwResult;
    tSysInfoUpdRsp.wCellId =  ptRnluRnlcSystemInfoRsp->wCellId;

    /* 信令跟踪 */
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                    EV_RNLU_RNLC_SYSINFO_RSP,
                    ptCimSIBroadcastVar->wCellId,
                    RNLC_INVALID_WORD,
                    RNLC_INVALID_WORD,
                    RNLC_ENB_TRACE_RECE,
                    sizeof(T_RnluRnlcSystemInfoRsp),
                    (const BYTE *) ptRnluRnlcSystemInfoRsp);

    Message tSysInfoUpdRspMsg;
    tSysInfoUpdRspMsg.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tSysInfoUpdRspMsg.m_wLen = sizeof(tSysInfoUpdRsp);
    tSysInfoUpdRspMsg.m_pSignalPara = static_cast<void*>(&tSysInfoUpdRsp);

    /* 1.小区建立引起的广播更新 */
    if ((SI_UPD_CELL_SETUP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (SI_UPD_CELL_ADD_byUNBLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimKillSystemInfoTimer();
        tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

        /* 给建立构件回响应消息 */
        dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tSysInfoUpdRspMsg);
        if (SSL_OK != dwResult)
        {
            /* print */
       
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                dwResult, 
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_ERROR_LEVEL, 
                                "\n SI: CIM Send to CellSetupComponent Rsp Fail!Reuslt =%d, CellId=%d, SI Upd Cause: %s\n",
                                dwResult,ptCimSIBroadcastVar->wCellId,
                                CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);

            return;
        }
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Rsp! Cause:%s!\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
        /* 状态迁移到idle状态，等待小区广播更新请求 */
        TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);

        return;
    }
    else if(SI_UPD_CELL_DELADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    {
        
        if(CAUSE_CELLSETUP == ptRnluRnlcSystemInfoRsp->wCause)
        {
            ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSetupRspFlag = 1;
        }
        if(CAUSE_SIMODIFY == ptRnluRnlcSystemInfoRsp->wCause)
        {
            ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSIUpdRspFlag = 1;
        }
        ptCimSIBroadcastVar->tSIBroadcastDate.dwRspResult |=ptRnluRnlcSystemInfoRsp->dwResult;
        if(0 != ptCimSIBroadcastVar->tSIBroadcastDate.dwRspResult)
        {
            /*两条有一条失败就直接给cmm回失败响应*/
            tSysInfoUpdRsp.dwResult = 1;
            tSysInfoUpdRsp.wCellId = ptRnluRnlcSystemInfoRsp->wCellId;
                  /* 给建立构件回响应消息 */
            dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tSysInfoUpdRspMsg);         
            if (SSL_OK != dwResult)
            {
                /* print */
                 /*如果这个地方发送失败就挂掉了*/
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                    dwResult, 
                                    ptCimSIBroadcastVar->wCellId,
                                    RNLC_ERROR_LEVEL, 
                                    "\n SIBroadcast: CIM Send to CellSetupComponent Rsp Fail!Reuslt =%d, CellId=%d SI Upd Cause: %s\n",
                                    dwResult,ptCimSIBroadcastVar->wCellId,
                                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            /*增加打印判断置位状态，方便问题定位*/
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive Fail RNLU Rsp !Setup =%d ,Upd = %d,SI Upd Cause : %s\n",
                                    ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSetupRspFlag,
                                    ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSIUpdRspFlag,
                                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            return;
        }
        
        if((1 == ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSetupRspFlag)
        &&(1 == ptCimSIBroadcastVar->tSIBroadcastDate.ucCellSIUpdRspFlag))
        {
            CimKillSystemInfoTimer();
            tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

            /* 给建立构件回响应消息 */
            dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tSysInfoUpdRspMsg);
            if (SSL_OK != dwResult)
            {
                /* print */        
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                    dwResult, 
                                    ptCimSIBroadcastVar->wCellId,
                                    RNLC_ERROR_LEVEL, 
                                    "\n SI: CIM Send to CellSetupComponent Rsp Fail!Reuslt =%d, CellId=%d,SI Upd Cause : %s\n",
                                    dwResult,ptCimSIBroadcastVar->wCellId,
                                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Rsp ! SI Upd Cause = %s\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;       
        }
        else
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:  Not All message has rsp !SI Upd Cause: %s\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            return;
        }
    }

    /* 2.非小区建立引起的广播更新失败 */
    if (0 != ptRnluRnlcSystemInfoRsp->dwResult)
    {
        /* 进入重传机制 */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount++;
        if (ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount <= 3)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: Cim The %u time Send Broadcast Msg To Rnlu fail !SI Upd Cause : %s\n",
            ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount,
            CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            /* 杀死定时器 */
            CimKillSystemInfoTimer();
            dwResult = CimSendBroadcastMsgToRnlu( );
            if (RNLC_FAIL == dwResult)
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                dwResult, 
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_ERROR_LEVEL, 
                                "\n SI: CIM Send Broadcast Msg to Rnlu Fail!Reuslt =%d, CellId=%d SI Upd Cause : %s\n",
                                dwResult,ptCimSIBroadcastVar->wCellId,
                                CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }

            dwResult = CimSetSystemInfoTimer();
            if (RNLC_FAIL == dwResult)
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                dwResult, 
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_ERROR_LEVEL, 
                                "\n SI: CIM Set System Info Timer Fail!Reuslt =%d, CellId=%d,SI Upd Cause: %s\n",
                                dwResult,ptCimSIBroadcastVar->wCellId,
                                CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Response upd reason= %d,SI Upd Cause: %s!",
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause,
                        CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
    }

    /* 2.非小区建立引起的广播更新成功或者达到尝试重传次数 */
    if ((0 == ptRnluRnlcSystemInfoRsp->dwResult) || (ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount > 3))
    {
        if (SI_UPD_CELL_UNBLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
        {
            tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

            /* 给闭塞构件回响应消息 */
            dwResult = SendTo(ID_CCM_CIM_CellUnBlockComponent, &tSysInfoUpdRspMsg);
            if (SSL_OK != dwResult)
            {
                /* print */
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                dwResult, 
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_ERROR_LEVEL, 
                                "\n SI: CIM Send to CellUnBlockComponent Rsp Fail!Reuslt =%d, CellId=%d,SI Upd Cause: %s\n",
                                dwResult,ptCimSIBroadcastVar->wCellId,
                                CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
        }

        if ((SI_UPD_CELL_DBS_MODIFY == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSUNLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
            /* 给CMM主控构件回响应消息 */
            PID tCMMPid = ptCIMVar->tCIMPIDinfo.tCMMPid;
            WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_SYSINFO_UPDATE_RSP,
                                                        &tSysInfoUpdRsp,
                                                        sizeof(tSysInfoUpdRsp),
                                                        COMM_RELIABLE,
                                                        &tCMMPid);
            if (OSS_SUCCESS != dwOssStatus)
            {
                /* print */
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                dwResult, 
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_ERROR_LEVEL, 
                                "\n SI: CIM Send to MainComponent Rsp Fail! Reuslt =%d, CellId=%d, SI Upd Cause: %s\n",
                                dwResult,ptCimSIBroadcastVar->wCellId,
                                CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
        }

        /* 杀掉定时器 */
        CimKillSystemInfoTimer();
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI : Receive RNLU Response succ upd reason= %d ! SI Upd Cause: %s",
                    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause,
                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
        /* 清空统计次数 */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount = 0;
    }
    /* 状态迁移到idle状态，等待小区广播更新请求 */
    TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_SystemInfoUpdateTimeout
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_SystemInfoUpdateTimeout()
{
    WORD32 dwResult = 0;
    T_SysInfoUpdRsp tSysInfoUpdRsp;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_CIMVar     *ptCIMVar = NULL;

    memset((VOID*)&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));

    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* 获取CIM小区广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Send to Broadcast MSG to RNLU Timer out!Cell id= %d ,SI UPD CAUSE = %d\n",
                ptCimSIBroadcastVar->wCellId,ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause);
    dwResult = CcmDbgAddCellCfgResultInfoStat(ptCimSIBroadcastVar->wCellId,ERR_CCM_CIM_SIB_SIUPD_TIMEOUT);
    if (RNLC_SUCC != dwResult)
    {
        CCM_ExceptionReport(ERR_CCM_FunctionCallFail_CcmDbgAddCellCfgResultInfoStat, 
                                dwResult, 
                                0, 
                                RNLC_WARN_LEVEL, 
                                " FunctionCallFail_CcmDbgAddCellCfgResultInfoStat! ");

        // 统计函数异常，不能返回
    }

    tSysInfoUpdRsp.dwResult = 1;
    tSysInfoUpdRsp.wCellId   = ptCIMVar->wCellId;

    Message tSysInfoUpdRspMsg;
    tSysInfoUpdRspMsg.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tSysInfoUpdRspMsg.m_wLen = sizeof(tSysInfoUpdRsp);
    tSysInfoUpdRspMsg.m_pSignalPara = static_cast<void*>(&tSysInfoUpdRsp);

    /* 1.小区建立引起的广播更新,对删建由于需要发送两条消息，
      这里也不考虑重传机制，直接回响应 */
    if ((SI_UPD_CELL_SETUP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (SI_UPD_CELL_DELADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

        /* 给建立构件回响应消息 */
        dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tSysInfoUpdRspMsg);
        if (SSL_OK != dwResult)
        {
            /* print */
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwResult, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Send to CellSetupComponent Rsp Fail! Reuslt =%d, CellId=%d\n",
                        dwResult,ptCimSIBroadcastVar->wCellId);
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI_UPD_CELL_SETUP!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
        /* 状态迁移到idle状态，等待小区广播更新请求 */
        TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        return;
    }

    /* 2.非小区建立引起的广播更新 */

    /* 进入重传机制 */
    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount++;
    if (ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount <= 3)
    {
        dwResult = CimSendBroadcastMsgToRnlu( );
        if (RNLC_FAIL == dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwResult, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Send Broadcast Msg to Rnlu Fail! Reuslt =%d, CellId=%d\n",
                        dwResult,ptCimSIBroadcastVar->wCellId);
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI retrans < 3 ! time = %d Cell id= %d\n",
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount,ptCimSIBroadcastVar->wCellId);
        dwResult = CimSetSystemInfoTimer();

        if (RNLC_FAIL == dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwResult, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Set System Info Timer Fail! Reuslt =%d, CellId=%d\n",
                        dwResult,ptCimSIBroadcastVar->wCellId);
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
    }
    else
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI retrans > 3 ! time = %d Cell id= %d\n",
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount,ptCimSIBroadcastVar->wCellId);
        if (SI_UPD_CELL_UNBLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
        {
            tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI_UPD_CELL_UNBLOCK!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);

            /* 给闭塞构件回响应消息 */
            dwResult = SendTo(ID_CCM_CIM_CellUnBlockComponent, &tSysInfoUpdRspMsg);
            if (SSL_OK != dwResult)
            {
                /* print */
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwResult, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Send to CellUnBlockComponent Rsp Fail! Reuslt =%d, CellId=%d\n",
                        dwResult,ptCimSIBroadcastVar->wCellId);
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        }

        if ((SI_UPD_CELL_DBS_MODIFY == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSUNLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI_UPD_CELL_DBS_MODIFY!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
            /* 给CMM主控构件回响应消息 */
            PID tCMMPid = ptCIMVar->tCIMPIDinfo.tCMMPid;
            WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_SYSINFO_UPDATE_RSP,
            &tSysInfoUpdRsp,
            sizeof(tSysInfoUpdRsp),
            COMM_RELIABLE,
            &tCMMPid);
            if (OSS_SUCCESS != dwOssStatus)
            {
                /* print */
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwResult, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Send to MainComponent Rsp Fail! Reuslt =%d, CellId=%d\n",
                        dwResult,ptCimSIBroadcastVar->wCellId);
                /* 状态迁移到idle状态，等待小区广播更新请求 */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* 状态迁移到idle状态，等待小区广播更新请求 */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        }
        /*解决用户面不回应导致的广播构件挂死在waitsiupdate态的问题*/
        /* 状态迁移到idle状态，等待小区广播更新请求 */
        TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n not SI_UPD_CELL_DBS_MODIFY or un block!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
        /* 清空统计次数 */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount = 0;
    }
    CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n Uncontron situation !Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
    return;
}

WORD32 CCM_CIM_SIBroadcastComponentFsm::GetWarningType(TS1AP_MessageIdentifier tMessageIdentifier)
{
    WORD16 wWarningType;
    memmove(&wWarningType , tMessageIdentifier.data,2);
    if(((wWarningType >= 4352) && (wWarningType <= 4359))|| ((wWarningType >= 0xA000) && (wWarningType <= 0xAFFF)))
    {
        return PWS_TYPE_ETWS;
    }
    if((wWarningType >= 4370)  &&  (wWarningType <= 4382))
    {
        return PWS_TYPE_CMAS;
    }
    return RNLC_INVALID_DWORD;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_S1KillWarningReq
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1KillWarningReq(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    TCpmCcmKillMsgReq *ptCpmCcmKillMsgReq            = NULL;
    ptCpmCcmKillMsgReq = (TCpmCcmKillMsgReq *)pMsg->m_pSignalPara;
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    WORD16 wCmasMsgId;
    WORD16 wCmasStreamId;
    memcpy(&wCmasMsgId,ptCpmCcmKillMsgReq->tMessageIdentifier.data,2);
    memcpy(&wCmasStreamId,ptCpmCcmKillMsgReq->tSerialNumber.data,2);
    ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillMsgId = wCmasMsgId;
    ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillStreamId = wCmasStreamId;
    BOOL SIUPDFlag = FALSE;
    for (int CmasIndex=0;CmasIndex<MAX_CMAS_MSG;CmasIndex++)
    {
        if((wCmasMsgId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].wCmasMsgID)&& 
            (wCmasStreamId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].wCmasStreamId)&&
            (1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].aucIsCurrentBroadcasting))
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucWarningCause = CIM_CAUSE_CMAS_SIB12_KILL ;
            SIUPDFlag = TRUE;          
        }
    }
    if (FALSE == SIUPDFlag)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: The CMAS message want to kill this time have already stoped ,ingore this time!\n");
        return;
    }
    Message tKillCmasUpdate;
    T_SysInfoUpdReq tSysinfoUpdReq;
    tSysinfoUpdReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_KILL;

    tKillCmasUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tKillCmasUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tKillCmasUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tKillCmasUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

    SendToSelf(&tKillCmasUpdate);
   
    return ;
}


void CCM_CIM_SIBroadcastComponentFsm::Handle_S1KillWarningRsp(Message *pMsg )
{

    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);
    T_CIMVar     *ptCIMVar = NULL;
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    T_SysInfoUpdRsp tSysInfoUpdRsp;
    memset(&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));
    tSysInfoUpdRsp.wCellId = ptCimSIBroadcastVar->wCellId;
    tSysInfoUpdRsp.dwResult = RNLC_SUCC;    
    PID tCMMPid = ptCIMVar->tCIMPIDinfo.tCMMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg( EV_CIM_CMM_S1_KILL_WARNING_RSP,
                                                &tSysInfoUpdRsp,
                                                sizeof(tSysInfoUpdRsp),
                                                COMM_RELIABLE,
                                                &tCMMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,
        "\n SI: CIM Send to MainComponent Rsp Fail! CellId=%d,SI Upd Cause: %s\n",
        ptCimSIBroadcastVar->wCellId,
        CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
    }
    return ;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_S1WarningReq
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningReq(Message *pMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);



    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /*需要增加对消息长度的校验*/
    if ( pMsg->m_wLen != sizeof(TCpmCcmWarnMsgReq))
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: EPC Warn Msg Len Wrong,m_wLen = %u\n",pMsg->m_wLen);
        return;
    }
    // 整体上分两种情况来处理，首先判断是ETWS功能，还是CMAS功能
    WORD32 dwPwsType = GetWarningType(ptCpmCcmWarnMsg->tMessageIdentifier);
    /*如果 设置为检查模式*/
    if(1 == g_ucEpcWarningCheckFlag)
    {
        /**如果告警和原来的一样，可直接返回，不需要做处理*/
        if(TRUE == CheckSamePWSReq(ptCpmCcmWarnMsg))
        {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: EPC Warn Msg The Same as Last Time!\n");       
            return ;
        }
    }
    /*先将本次的告警信息保存在广播构件的数据区以备广播更新时使用*/
    memset(&ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg,0,sizeof(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg));
    memmove(&ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg,ptCpmCcmWarnMsg,sizeof(tagTCpmCcmWarnMsgReq));
    
    if (PWS_TYPE_ETWS == dwPwsType)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:Receive EPC ETWS Warning Message!\n"); 
        HandleETWSWarningReq(pMsg);
    }
    else if (PWS_TYPE_CMAS == dwPwsType)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:Receive CMAS Warning Message\n!"); 
        HandleCMASWarningReq(pMsg);
    }
    else
    {
        /*目前s1口还未有其他的告警消息，这里认为未知，报异常*/
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_IDLE,
                                    pMsg->m_dwSignal,
                                    0,
                                    RNLC_ERROR_LEVEL,
                                    " SI: Receive Unknown EPC Warn Msg ID = %d! ",
                                    pMsg->m_dwSignal);
        return;
    }
    
    memset(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg,0,sizeof(ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg));
    memmove(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg,ptCpmCcmWarnMsg,sizeof(tagTCpmCcmWarnMsgReq));
    return;
}


WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleETWSWarningReq(Message *pMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(pMsg);
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    T_CIMVar *ptCIMVar = NULL;
    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    Message tEtwsUpdate;
    T_SysInfoUpdReq tSysinfoUpdReq;
    tSysinfoUpdReq.wCellId = ptCIMVar->wCellId;

    tEtwsUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tEtwsUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tEtwsUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tEtwsUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /*如果此次告警和上次告警信息不一样，做覆盖处理*/
    BOOL  bIsIncludeSib10 =CimIsBoardIncludeSib10(ptCpmCcmWarnMsg);
    BOOL  bIsIncludeSib11 = CimIsBoardIncludeSib11(ptCpmCcmWarnMsg);
             
    if(bIsIncludeSib10&&bIsIncludeSib11)
    {
        tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_SIB11_ADD;
        SendToSelf(&tEtwsUpdate);
        return RNLC_SUCC;    
    }
    if(bIsIncludeSib10&&(!bIsIncludeSib11))
    {
        tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_ADD;
        SendToSelf(&tEtwsUpdate);
        return RNLC_SUCC;  
    }
    if((!bIsIncludeSib10)&&bIsIncludeSib11)
    {
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
        {
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD;
        }
        else
        {
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB11_ADD;
        }
        SendToSelf(&tEtwsUpdate);
        return RNLC_SUCC;  
    }
    if((!bIsIncludeSib10)&&(!bIsIncludeSib11))
    {
        tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_SIB11_STOP;
        SendToSelf(&tEtwsUpdate);
        return RNLC_SUCC;  
    }
    return RNLC_SUCC;  
   // 这个函数到此就结束了，sib10  和 sib11的处理放在update函数中处理。

}

WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleCMASWarningReq(Message *pMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(pMsg);
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    T_CIMVar *ptCIMVar = NULL;
    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;

    Message tCmasUpdate;
    T_SysInfoUpdReq tSysinfoUpdReq;
    tSysinfoUpdReq.wCellId = ptCIMVar->wCellId;

    tCmasUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tCmasUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tCmasUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tCmasUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

    /*如果存在cur字段，增加到当前告警*/
    if(1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    {
        /*如果当前发送的告警已经达到了最大的条数，则直接返回失败*/
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[MAX_CMAS_MSG -1].aucIsCurrentBroadcasting)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 broading now has reach the MAX number!%d\n",MAX_CMAS_MSG);
            return RNLC_FAIL;
        }
        /*如果此次告警没有在发送*/
        tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_ADD;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 with cur! sib12 add!\n");
        SendToSelf(&tCmasUpdate);
        return RNLC_SUCC;
    }
    else
    {
        if(TRUE == CimIsBoardIncludeSib12(ptCpmCcmWarnMsg))
        {
            /*如果不存在cur字段，并且包含sib12的内容则覆盖当前告警*/
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_UPD;
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 without cur! sib12 upd!\n");
            /*更新系统信息*/
            SendToSelf(&tCmasUpdate);
            return RNLC_SUCC;
        }
        else
        {
            /*如果不包含cur字段，并且没有sib12的内容，此时则停止所有的在发的sib12告警*/
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_STOP;
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 without cur! sib12 STOP!\n");
            /*更新系统信息*/
            SendToSelf(&tCmasUpdate);
            return RNLC_SUCC;
        }
    }
    /*lint -save -e827 */
    CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: CMAS Error Happen!\n");
    /*lint -restore*/
    return RNLC_FAIL;
}

BOOL CCM_CIM_SIBroadcastComponentFsm::CheckSamePWSReq(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    WORD32 dwResultMsgid =0;
    WORD32 dwResultSeril =0;

    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /*lint -save -e732 */
    dwResultMsgid = memcmp(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg.tMessageIdentifier,
                                                &ptCpmCcmWarnMsg->tMessageIdentifier,
                                                sizeof(TS1AP_MessageIdentifier));
    dwResultSeril = memcmp(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg.tSerialNumber,
                                                &ptCpmCcmWarnMsg->tSerialNumber,
                                                sizeof(TS1AP_SerialNumber));
    WORD32 dwWarnType = GetWarningType(ptCpmCcmWarnMsg->tMessageIdentifier);
   
    /*lint -restore*/
    if (0 == dwResultMsgid && 0 == dwResultSeril )
    {
        if(PWS_TYPE_ETWS == dwWarnType)
        {
            if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
              ||(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib11Broadcasting))
              {
                return TRUE;
              }
        }
        else
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL CCM_CIM_SIBroadcastComponentFsm::CimIsThisCmasSendingNow(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    WORD16 wCmasMsgId;
    WORD16 wCmasStreamId;
    wCmasMsgId = ptCpmCcmWarnMsg->tMessageIdentifier.data[0]*256 +  ptCpmCcmWarnMsg->tMessageIdentifier.data[1];
    wCmasStreamId =ptCpmCcmWarnMsg->tSerialNumber.data[0]*256 + ptCpmCcmWarnMsg->tSerialNumber.data[1];

    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    for (int CmasIndex=0;CmasIndex<MAX_CMAS_MSG;CmasIndex++)
        {
        if((wCmasMsgId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].wCmasMsgID)&& 
            (wCmasStreamId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].wCmasStreamId)&&
            (1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[CmasIndex].aucIsCurrentBroadcasting))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_S1WarningRsp
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningRsp(Message *pMsg)
{

    /* 获取CIM小区实例信息 */
    T_CIMVar     *ptCIMVar = NULL;
    BYTE   aucSibList[DBS_SI_SPARE] = {0};

    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    WORD32 dwPwsType = GetWarningType(ptCpmCcmWarnMsg->tMessageIdentifier);
    /*需要增加对消息长度的校验*/
    if ( pMsg->m_wLen != sizeof(TCpmCcmWarnMsgReq))
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: Handle_S1WarningReq Recv Msg Len Wrong,pMsg->m_wLen = %u\n",
        pMsg->m_wLen);
        return;
    }

    T_SysInfoUpdRsp tSysInfoUpdRsp;
    memset(&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));
    tSysInfoUpdRsp.wCellId = ptCimSIBroadcastVar->wCellId;
    tSysInfoUpdRsp.dwResult = RNLC_SUCC;

    /*读取数据库获取sib1，从而得到调度信息,通过aucSibList 返回出来*/
    {
        SchedulingInfoList tSchedulingInfoList;
        memset((VOID*)&tSchedulingInfoList, 0, sizeof(tSchedulingInfoList));

        T_DBS_GetSib1Info_REQ tGetSib1InfoReq;
        T_DBS_GetSib1Info_ACK tGetSib1InfoAck;

        memset((VOID*)&tGetSib1InfoReq, 0, sizeof(tGetSib1InfoReq));
        memset((VOID*)&tGetSib1InfoAck, 0, sizeof(tGetSib1InfoAck));

        /* 2.获取SIB1信息 */
        tGetSib1InfoReq.wCallType = USF_MSG_CALL;
        tGetSib1InfoReq.wCellId   = ptCimSIBroadcastVar->wCellId;
        BYTE ucResult = UsfDbsAccess(EV_DBS_GetSib1Info_REQ, (VOID *)&tGetSib1InfoReq, (VOID *)&tGetSib1InfoAck);
        if ((FALSE == ucResult) || (0 != tGetSib1InfoAck.dwResult))
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB1 Info fail! CellId=%d\n",
                        ptCimSIBroadcastVar->wCellId);
        }
        /* 修改SISCH信息 */
        CimDbgConfigSISch(&tGetSib1InfoAck);

        /* 3.组装调度信息 */
        WORD32 dwResult = CimGetSIScheInfo(&tGetSib1InfoAck, aucSibList, &tSchedulingInfoList);
        if (FALSE == dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SISCHE Info fail! CellId=%d\n",
                        ptCimSIBroadcastVar->wCellId);
        }
    }
    /*如果是cmas告警，并且已经达到了最大的发送条数，则直接返回失败*/
    if(PWS_TYPE_CMAS == dwPwsType)
    {
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[MAX_CMAS_MSG -1].aucIsCurrentBroadcasting)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 broading now has reach the MAX number : %d !\n",MAX_CMAS_MSG);
            tSysInfoUpdRsp.dwResult = RNLC_FAIL;
        }
        if(FALSE == aucSibList[DBS_SI_SIB12])
        {
            tSysInfoUpdRsp.dwResult = RNLC_FAIL;
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: No CMAS Schedule info,Deal with CMAS warning Fail!Cell id : %d !\n",ptCimSIBroadcastVar->wCellId);
        }
    }
    /*对etws告警，如果没有对应的调度信息则回复失败*/
    /*lint -save -e690 */
    if(PWS_TYPE_ETWS == dwPwsType)
    {
        BOOL  bIsIncludeSib10 =CimIsBoardIncludeSib10(ptCpmCcmWarnMsg);
        BOOL  bIsIncludeSib11 = CimIsBoardIncludeSib11(ptCpmCcmWarnMsg);
        if(bIsIncludeSib11&&bIsIncludeSib10)
        {
            if((FALSE == aucSibList[DBS_SI_SIB11])&&(FALSE == aucSibList[DBS_SI_SIB10]))
            {
                tSysInfoUpdRsp.dwResult = RNLC_FAIL;
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: No ETWS Schedule info(SIB10,SIB11),Deal with ETWSwarning Fail!Cell id : %d !\n",ptCimSIBroadcastVar->wCellId);
            }
        }
        if((!bIsIncludeSib11)&&bIsIncludeSib10)
        {
            if(FALSE == aucSibList[DBS_SI_SIB10])
            {
                tSysInfoUpdRsp.dwResult = RNLC_FAIL;
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: No ETWS Schedule info(SIB10),Deal with ETWS warning Fail!Cell id : %d !\n",ptCimSIBroadcastVar->wCellId);
            }
        }
        if(bIsIncludeSib11&&(!bIsIncludeSib10))
        {
            if(FALSE == aucSibList[DBS_SI_SIB11])
            {
                tSysInfoUpdRsp.dwResult = RNLC_FAIL;
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI:No ETWS Schedule info(SIB11),Deal with ETWS warning Fail!Cell id : %d !\n",ptCimSIBroadcastVar->wCellId);
            }
        }
    }
    /*lint -restore*/
    /*如果来临的即不是etws也不是cams，在没有新的功能扩展前，给核心网回复失败*/
    if((PWS_TYPE_ETWS != dwPwsType)&&(PWS_TYPE_CMAS != dwPwsType))
    {
        tSysInfoUpdRsp.dwResult = RNLC_FAIL;
    }
    PID tCMMPid = ptCIMVar->tCIMPIDinfo.tCMMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_S1_WARNING_RSP,
                                                                                        &tSysInfoUpdRsp,
                                                                                        sizeof(tSysInfoUpdRsp),
                                                                                        COMM_RELIABLE,
                                                                                        &tCMMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: Send CMM warning Rsp Fail! CellId=%d \n",ptCimSIBroadcastVar->wCellId);
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL," SI: Send CMM warning Rsp Succ! CellId=%d ",ptCimSIBroadcastVar->wCellId);
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_S1WarningSib10Timeout
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningSib10Timeout()
{
    /* 收到定时器超时，给CIM回失败响应 */
    T_SysInfoUpdReq tSysinfoUpdReq;
    Message message;
    memset((BYTE *)&tSysinfoUpdReq, 0x00, sizeof(tSysinfoUpdReq));
    Message tEtwsUpdate;
    tEtwsUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tEtwsUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tEtwsUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tEtwsUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

     /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar); 
    tSysinfoUpdReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_STOP;
    SendToSelf(&tEtwsUpdate);
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = INVALID_TIMER_ID;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB10 Timer out happen!\n");
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: Handle_S1WarningSib11Timeout
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningSib11Timeout()
{
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
       /*对sib11的发送停止定时器超时并不会触发广播更新，此处是唯一置位发送标示为0的地方*/
    ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib11Broadcasting = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer = INVALID_TIMER_ID;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB11 Timer out happen!\n");
    return;
}

void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningSib12Timeout()
{
    WORD32 dwCmasFlag =0;
    WORD16 wCmasMsgId;
    WORD16 wCmasStreamId;
    dwCmasFlag = OSS_GetParaFromCurTimer();
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: OSS_GetParaFromCurTimer() = %d\n",dwCmasFlag);
    wCmasMsgId = (WORD16) (dwCmasFlag&0xFFFF);
    dwCmasFlag = dwCmasFlag>>16;
    wCmasStreamId = (WORD16) (dwCmasFlag&0xFFFF);
    BYTE ucOperatType =0;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Sib12 Time out!wCmasStreamId = %d, wCmasMsgId = %d\n",wCmasStreamId,wCmasMsgId);
    CimUpdateCmasSendList( wCmasMsgId,  wCmasStreamId,ucOperatType);   
}

/*<FUNC>***********************************************************************
* 函数名称: CimBroadcastDataInit
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CCM_CIM_SIBroadcastComponentFsm::CimBroadcastDataInit(T_SysInfoUpdReq  *ptsysInfoUpdReq)
{
    T_CIMVar *ptCIMVar = NULL;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    ptCimSIBroadcastVar->wCellId = ptCIMVar->wCellId;
    ptCimSIBroadcastVar->dwSysInfoTimerId   = INVALID_TIMER_ID;
    memset(ptCimSIBroadcastVar->tSib1Buf,0,sizeof(ptCimSIBroadcastVar->tSib1Buf));
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    /*只考虑mac 调度字节映射后的总长度*/
    ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len = 0;
    /*四字节对齐后的总长度*/
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib11MsgLength = 0;
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength = 0;
/*普通广播更新时默认设为不需要填写sib10信息*/
    ptCimSIBroadcastVar->tSIBroadcastDate.ucNormalSibWithSib10Info = 0;
    /* 小区建立时初始化*/
    if (SI_UPD_CELL_SETUP == ptsysInfoUpdReq->ucCellSIUpdCause)
    {
        memset(&ptCimSIBroadcastVar->tSIBroadcastDate, 0 ,sizeof(ptCimSIBroadcastVar->tSIBroadcastDate));
        memset(&ptCimSIBroadcastVar->tCimWarningInfo, 0 ,sizeof(ptCimSIBroadcastVar->tCimWarningInfo));
        memset(&ptCIMVar->tRNLUSiInfoRecord[0],0,SI_TO_RNLU_INFO_NUM*sizeof(TCimSiInfoRecord));
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = INVALID_TIMER_ID;
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer = INVALID_TIMER_ID;
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib12RepeatTimer = INVALID_TIMER_ID;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: BroadCast Data Initial:Cell SetUp!\n");
    }
    return;
}


/*<FUNC>***********************************************************************
* 函数名称: CimNormalBroadMsgWithSib10
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimNormalBroadMsgWithSib10()
{
        T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
        if((SI_UPD_CELL_DBS_MODIFY == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
            (SI_UPD_CELL_GPSLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
            (SI_UPD_CELL_GPSUNLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
            if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
}





/*<FUNC>***********************************************************************
* 函数名称: CimSibMappingToSi
* 功能描述:将SIB映射到SI中
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSibMappingToSi(T_AllSibList *ptAllSibList,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo)
{
    SchedulingInfoList       *ptScheInfo  = NULL;   /* 调度信息 */
    SystemInformation_r8_IEs *ptSysInfoR8 = NULL;   /* SI码流结构 */
    BYTE                     ucSibType;             /* SIB名 */
    WORD32                   dwSibNum;              /* 每个SI中SIB个数 */
    WORD32                   dwSiNum;               /* SI数目 */
    WORD32                   dwSibNumLoop;          /* 每个SI中SIB遍历 */
    WORD32                   dwSiNumLoop;           /* SI遍历 */
    SchedulingInfoList  tSchedulingInfoList;        /*调度信息列表*/
    memset(&tSchedulingInfoList,0,sizeof(SchedulingInfoList));
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

    /* 调度信息 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    ptScheInfo = &(ptAllSibList->tSib1Info.schedulingInfoList);
    /*对sib11和sib12方案要求是单独的si，这里不需要做处理，如果配置中有相应的配置，这里应该处理掉*/
    /*注意这里因为CimUpdateSib1Sche内部处理的逻辑，这里千万不可直接用ptScheInfo代替*/
    tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
    CimAdjustScheInfoList(&tSchedulingInfoList,ptAllSibList->aucSibList, sibType11);
    CimAdjustScheInfoList(&tSchedulingInfoList,ptAllSibList->aucSibList, sibType12_v920);
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    
    dwSiNum = ptScheInfo->n;
    if (CIM_SI_NUM < dwSiNum)
    {
        ptAllSiEncodeInfo->dwSiNum = CIM_SI_NUM ;
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SI num error num %d \n",dwSiNum);
        return FALSE;
    }
    else
    {
        ptAllSiEncodeInfo->dwSiNum = dwSiNum;
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB Maping To SI Now...\n");
    /* 依据SI个数填写 */
    for (dwSiNumLoop = 0; dwSiNumLoop < dwSiNum; dwSiNumLoop++)
    {
        ptSysInfoR8 = &(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].systemInformation_r8);
        ptSysInfoR8->m.nonCriticalExtensionPresent = FALSE;
        ptSysInfoR8->sib_TypeAndInfo.n = 0;

        ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].tSiPeriod = ptScheInfo->elem[dwSiNumLoop].si_Periodicity;
        dwSibNum = ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.n;

        if (0 == dwSiNumLoop)
        {
            /* 第一个SI中包含SIB2 */
            dwSibNum += 1;
        }
        /*标识si中映射的sib，换算成二进制，从低位到高位的10个bit，依次对应sib2--sib10*/
        WORD16 wSISibFlag =0;
        /* 依据每个SI中SIB个数填写 */
        for (dwSibNumLoop = 0; dwSibNumLoop < dwSibNum; dwSibNumLoop++)
        {
            if ((0 == dwSiNumLoop) && (0 == dwSibNumLoop))
            {
                ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 1; /* SIB2 */
                ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib2 =
                &(ptAllSibList->tSib2Info);
                ptSysInfoR8->sib_TypeAndInfo.n++;
                wSISibFlag |= 0x1;
                continue;
            }

            /* 解决第一个SI不仅仅配置SIB2的情况 */
            if ((0 == dwSiNumLoop) && (dwSibNumLoop > 0))
            {
                ucSibType = (BYTE)(ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.elem[dwSibNumLoop-1]);
            }
            else
            {
                ucSibType = (BYTE)(ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.elem[dwSibNumLoop]);
            }

            switch (ucSibType)
            {
                case sibType3:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 2; /* SIB3 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib3 =
                    &(ptAllSibList->tSib3Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x2;
                    break;
                }
                case sibType4:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 3; /* SIB4 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib4 =
                    &(ptAllSibList->tSib4Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x4;
                    break;
                }
                case sibType5:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 4; /* SIB5 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib5 =
                    &(ptAllSibList->tSib5Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x8;
                    break;
                }
                case sibType6:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 5; /* SIB6 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib6 =
                    &(ptAllSibList->tSib6Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x10;
                    break;
                }
                case sibType7:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 6; /* SIB7 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib7 =
                    &(ptAllSibList->tSib7Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x20;
                    break;
                }
                case sibType8:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 7; /* SIB8 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib8 =
                    &(ptAllSibList->tSib8Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x40;
                    break;
                }
                case sibType9:
                {
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 8; /* SIB9 */
                    ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib9 =
                    &(ptAllSibList->tSib9Info);
                    ptSysInfoR8->sib_TypeAndInfo.n++;
                    wSISibFlag |= 0x800;
                    break;
                }
                case sibType10:
                {
                    /*除告警外未导致小区删建的广播更新的时候*/
                    if(TRUE == CimNormalBroadMsgWithSib10())
                    {
                        ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 9; /* SIB10 */
                        ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib10 =
                        &(ptAllSibList->tSib10Info);
                        ptSysInfoR8->sib_TypeAndInfo.n++;                    
                        wSISibFlag |= 0x100;
                    }
                    else
                    {
                    /*bResult = CimIsBoardIncludeSib10(ptCpmCcmWarnMsgReq, ptAllSibList);*/
                    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
                    {
                        ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].t = 9; /* SIB10 */
                        ptSysInfoR8->sib_TypeAndInfo.elem[dwSibNumLoop].u.sib10 =
                        &(ptAllSibList->tSib10Info);
                        ptSysInfoR8->sib_TypeAndInfo.n++;
                            wSISibFlag |= 0x100;
                    }
                    }
                    break;
                }
                default :
                {
                    CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: ucSibType = %d error! \n",ucSibType);
                    break;
                }
            }
        }
        CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId, "\n SI: Sib Map To SI[%d].SIBFLAG:%d ",dwSiNumLoop,wSISibFlag);
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB Maping To SI Finish!\n");
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimEncodeAllSi
* 功能描述: SI编码失败是否需要调整调度信息，先暂时不考虑，新的数据流是否变化
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllSi(T_AllSibList *ptAllSibList,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo,TWarningMsgSegmentList *ptWarningMsgSegmentList,
        TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation          tSystemInfo;  /* 系统信息SI编码结构 */
    ASN1CTXT                   tCtxt;        /* 编解码上下文结构 */
    WORD32                                 dwSiNumLoop            =  0;  /* SI个数遍历 */
    WORD16                                 wMsgLen                   =  0;      /* SI码流长度 */
    WORD16                                 wTbSize                    =  0;      /* SI码流匹配的TB块大小 */
    WORD32                                 dwSiNumInSchList      = 0;
    WORD32                     dwResult = FALSE;
    SystemInformation_r8_IEs         *ptSystemInfoR8       =  NULL;
    SchedulingInfoList         *ptSchedulingInfoList = NULL;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

      /*初始化*/
    memset((VOID*)&tBcchDlSchMsg, 0, sizeof(tBcchDlSchMsg));
    memset((VOID*)&tBcchDlSchMsgC1, 0, sizeof(tBcchDlSchMsgC1));
    memset((VOID*)&tSystemInfo, 0, sizeof(tSystemInfo));
    memset((VOID*)&tCtxt, 0, sizeof(tCtxt));
    
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI Encoding Now...\n");
    
   /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   
    ptSchedulingInfoList = &(ptAllSibList->tSib1Info.schedulingInfoList);
    dwSiNumInSchList = ptSchedulingInfoList->n;

    /* SI数目为循环次数 */
    for (dwSiNumLoop = 0; dwSiNumLoop < (ptAllSiEncodeInfo->dwSiNum); dwSiNumLoop++)
    {
        tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
        tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
        tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_c1_systemInformation;
        tSystemInfo.criticalExtensions.t = T_SystemInformation_criticalExtensions_systemInformation_r8;
        ptSystemInfoR8 = &(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].systemInformation_r8);
        tSystemInfo.criticalExtensions.u.systemInformation_r8 = ptSystemInfoR8;
        tBcchDlSchMsgC1.u.systemInformation = &tSystemInfo;

        dwResult = CimEncodeFunc(CIM_SI_SI, &tCtxt, (VOID *)&tBcchDlSchMsg);
        if (FALSE == dwResult)
        {
            if (0 == dwSiNumLoop)
            {
                /* SIB2在第一个SI中，若编码失败则停止下发广播 */
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SI-1 which include SIB2 encode fail!\n");
                return FALSE;
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Encode SI-%lu failed!\n",dwSiNumLoop + 1);
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid = FALSE;
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen = 0;
            }
        }
        else
        {
            TestProbeSib8_insert(tCtxt.dw1XrttLongCodeOffset, tCtxt.dwSib8SystemTimeInfoOffset);
            if((0 != tCtxt.dwSib8SystemTimeInfoOffset)||(0 != tCtxt.dw1XrttLongCodeOffset))
            {
                ptAllSibList->dw1XrttLongCodeOffset = tCtxt.dw1XrttLongCodeOffset;            
                ptAllSibList->dwSib8SystemTimeInfoOffset = tCtxt.dwSib8SystemTimeInfoOffset;
            }            
            /* SI 编码成功处理,首先判断其码流长度是否符合调度限制 */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI-%d stream len(=%u) beyond TB size boundary!\n",
                            dwSiNumLoop + 1, wMsgLen);
                if (0 == dwSiNumLoop)
                {
                    /* SIB2在第一个SI中，若编码失败则停止下发广播 */
                    CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: SI-1 can't be scheduled! So stop broadcasting!\n");
                    return FALSE;
                }
                else
                {
                    ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid = FALSE;
                    ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen = 0;
                }
            }
            else
            {
                /* SI编码成功且CMAC可以正常调度，则保存码流 */
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid = TRUE;
                memcpy(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg,
                       &(tCtxt.buffer.data[0]), wMsgLen);

                /* SI的长度取为匹配的TB块大小 */
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen = wTbSize;
                
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Normal SI-%lu Broadcast Stream! len = %d, MAC Tb Size = %lu", dwSiNumLoop+ 1,wMsgLen,wTbSize);
                /* 打印出码流信息 */
                if (1 == g_dwCimPrintFlag)
                {
                     CimDbgPrintMsg(&(tCtxt.buffer.data[0]), wMsgLen);
                }
            }
        }
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Normal SI Encode Finish!\n");
    /*处理告警信息的编码*/
    BOOL bAllSIENcodeFlag = FALSE;
    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: ETWS SIB11 SegmentList Encoding...\n");  
        bAllSIENcodeFlag = CimEncodeAllSegSi(ptWarningMsgSegmentList);
        /*如果所有分片都编码失败，则不发送sib11*/
        if(FALSE == bAllSIENcodeFlag)
        {
          ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        }
        else
        {
          CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB11 SI Encode Finish!\n");
        }
    }

    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CMAS SIB12 SegmentList Encoding...\n");  
        bAllSIENcodeFlag = CimEncodeAllCmasSegSi(ptCmasWarningMsgSegmentList);
        /*如果所有分片都编码失败，则不发送sib12*/
        if(FALSE == bAllSIENcodeFlag)
        {
          ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        }
        else
        {
          CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB12 SI Encode Finish!\n");
        }
    }
    
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: All SI encode completed! Normal SI num = %d \n",dwSiNumInSchList);
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSibsCheckAndUpdte
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSibsCheckAndUpdte(T_SysInfoUpdReq  *ptsysInfoUpdReq,
        T_AllSibList *ptAllSibList,
        T_BroadcastStream *ptNewBroadcastStream)
{
    WORD32   dwResult    = FALSE;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_CIMVar     *ptCIMVar = NULL;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptNewBroadcastStream);

    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    if ((SI_UPD_CELL_SETUP == ptsysInfoUpdReq->ucCellSIUpdCause)
       ||(SI_UPD_CELL_UNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause)
       ||(SI_UPD_CELL_ADD_byUNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause))
    {
        /*小区建立不需要进行广播更新的判断*/
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: Cell SetUp/UnBlcock/DelAdd ,SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    if(SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)
    {
       /* SIB列表有变化，ValueTag++ */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag++;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag
        = (BYTE)((ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag) % (31 + 1));
        /*    */
        ptCIMVar->ucSib1ValueTagCopy = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: Cell DelAdd Update SI Update!! \n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    if ((CIM_CAUSE_ETWS_SIB10_ADD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_ETWS_SIB11_ADD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_ETWS_SIB10_SIB11_ADD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_ETWS_SIB10_STOP == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||( CIM_CAUSE_CMAS_SIB12_ADD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_CMAS_SIB12_UPD == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_CMAS_SIB12_KILL == ptsysInfoUpdReq->ucCellSIUpdCause)
            ||(CIM_CAUSE_CMAS_SIB12_STOP == ptsysInfoUpdReq->ucCellSIUpdCause))
    {
        /*EPC 告警不需要进行广播更新的判断*/
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: EPC warning SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 1.比较SIB列表 ,只比较sib1--sib10部分*/
    /*lint -save -e732 */
    dwResult = memcmp(ptAllSibList->aucSibList, ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList, sizeof(BYTE) * (CIM_MAX_SIB_LIST_LEN - 2));
    /*lint -restore*/
    if (0 != dwResult)
    {
        /* SIB列表有变化，ValueTag++ */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag++;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag
        = (BYTE)((ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag) % (31 + 1));
        /*    */
        ptCIMVar->ucSib1ValueTagCopy = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB list Update SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 2.如果下发的SIB列表没有变化，则先比较SIB2及SIB2之后的码流 */
    /*lint -save -e732 */
    dwResult = memcmp(ptNewBroadcastStream->aucAllSibBuf,
                      ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucAllSibBuf,
                      sizeof(BYTE) * CIM_ALL_SIB_BUF_LEN);
    /*lint -restore*/
    if (0 != dwResult)
    {
        /* SIB列表有变化，ValueTag++ */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag++;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag
        = (BYTE)((ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag) % (31 + 1));

        /*    */
        ptCIMVar->ucSib1ValueTagCopy = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB Stream Update SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 3.比较MIB 码流长度*/
    if(ptNewBroadcastStream->ucMibLen != ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucMibLen)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: MIB Length different  SI Update Check TRue!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    /*4. 比较MIB码流*/
    /*lint -save -e732 */
    dwResult = memcmp(ptNewBroadcastStream->aucMibBuf,
                      ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucMibBuf,
                      ptNewBroadcastStream->ucMibLen);
    /*lint -restore*/
    if (0 != dwResult)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: MIB Stream different  SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 5.比较SIB1 码流长度*/
    if(ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen != (WORD16) ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucSib1Len)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB1 Length different  SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    /*6.比较sib1 码流*/
    /*lint -save -e732 */
    dwResult = memcmp(ptCimSIBroadcastVar->tSib1Buf[0].aucSib1Buf,
                      ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucSib1Buf,
                      ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen);
    /*lint -restore*/
    if (0 != dwResult)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB1 Stream different  SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    /*其他情况，返回没有更新*/
    return FALSE;
}
/*<FUNC>***********************************************************************
* 函数名称: CimSibsDealWithNoUpdate
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSibsDealWithNoUpdate(T_SysInfoUpdReq  *ptsysInfoUpdReq,
        T_SysInfoUpdRsp    *ptSysInfoUpdRsp)
{

    T_CIMVar     *ptCIMVar = NULL;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdRsp);

    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* 表示未更新，不需要下发广播 */
    CCM_CIM_LOG(RNLC_WARN_LEVEL,
            "\n SI: CIM Check System not Update! CellId=%d! \n",
            ptCIMVar->wCellId);
    /* 如果系统消息没有更新，那个也给CIM 或者 CMM回一个响应消息，这个是架构的原因 */
    if ((SI_UPD_CELL_SETUP == ptsysInfoUpdReq->ucCellSIUpdCause) ||
        (SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause) ||
        (SI_UPD_CELL_ADD_byUNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause))
    {
        memset(ptSysInfoUpdRsp, 0, sizeof(T_SysInfoUpdRsp));
        ptSysInfoUpdRsp->dwResult = 0;
        ptSysInfoUpdRsp->wCellId   = ptCIMVar->wCellId;

        CimSendMsgToOtherComponent((BYTE *)ptSysInfoUpdRsp, sizeof(T_SysInfoUpdRsp),
                                                   CMD_CIM_CELL_SYSINFO_UPDATE_RSP, ID_CCM_CIM_CellSetupComponent);

    }
    else if (SI_UPD_CELL_UNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause)
    {
        memset(ptSysInfoUpdRsp, 0, sizeof(T_SysInfoUpdRsp));
        ptSysInfoUpdRsp->dwResult = 0;
        ptSysInfoUpdRsp->wCellId   = ptCIMVar->wCellId;

        CimSendMsgToOtherComponent((BYTE *)ptSysInfoUpdRsp, sizeof(T_SysInfoUpdRsp),
                                   CMD_CIM_CELL_SYSINFO_UPDATE_RSP, ID_CCM_CIM_CellUnBlockComponent);
    }
    else if (SI_UPD_CELL_DBS_MODIFY == ptsysInfoUpdReq->ucCellSIUpdCause)
    {
        ptSysInfoUpdRsp->dwResult = 0;
        ptSysInfoUpdRsp->wCellId   = ptCIMVar->wCellId;

        PID tCMMPid = ptCIMVar->tCIMPIDinfo.tCMMPid;
        WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_SYSINFO_UPDATE_RSP,
                             ptSysInfoUpdRsp,
                             sizeof(T_SysInfoUpdRsp),
                             COMM_RELIABLE,
                             &tCMMPid);
        if (OSS_SUCCESS != dwOssStatus)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_SEND_MSG_TO_RNLU_ADAPT,
                                    dwOssStatus,
                                    0,
                                    RNLC_ERROR_LEVEL,
                                    "\n SIBroadcast: CIM Send to MainComponent Rsp Fail! CellId=%d\n",
                                    ptCIMVar->wCellId);
        }
    }
    else
    {
    // nothing!
    }
  return RNLC_SUCC;
}



WORD32 CCM_CIM_SIBroadcastComponentFsm::CimComposeSib11MsgToRnlu(T_RnlcRnluSystemInfoHead  *ptRnlcRnluSystemInfoHead,
                                                                                                                               TWarningMsgSegmentList *ptWarningMsgSegmentList,
                                                                                                                               T_AllSibList *ptNewSibList)
{
    CCM_NULL_POINTER_CHECK(ptRnlcRnluSystemInfoHead);
    CCM_NULL_POINTER_CHECK(ptWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptNewSibList);
    /*处理存在sib11告警的情况si*/
    BYTE    ucSiTransNum  = 0;        /* 在SI window内SI传输次数 */   
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_SystemInfoETWS_up        *ptSystemInfoETWS_up = NULL;
    T_RnlcRnluSystemInfoHead  tRnlcRnluHeadInfo;       /* 发送到USM的结构 */
    memset(&tRnlcRnluHeadInfo, 0, sizeof(tRnlcRnluHeadInfo));
    memmove(&tRnlcRnluHeadInfo,ptRnlcRnluSystemInfoHead,sizeof(T_RnlcRnluSystemInfoHead));
    tRnlcRnluHeadInfo.m.dwSib11Present =1;
    tRnlcRnluHeadInfo.m.dwModiPeriodCoeffPresent  =1;
    tRnlcRnluHeadInfo.m.dwSystemInfoMasterPresent = 0;
    tRnlcRnluHeadInfo.m.dwSystemInfoListPresent = 0;
    tRnlcRnluHeadInfo.m.dwSystemInfo1Present  = 0;
    tRnlcRnluHeadInfo.m.dwSib10Present  = 0;
    tRnlcRnluHeadInfo.m.dwSib12Present = 0;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Composing SIB11 SI Msg to RNLU now...\n");
    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /*如果是与sib11 停止有关的消息，此时也是需要发送sib11消息的，不过原因为sib11 stop*/
    if((CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
    (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        tRnlcRnluHeadInfo.wSystemInfoCause = CAUSE_SIB11_STOP;
        tRnlcRnluHeadInfo.m.dwSib11Present =0;
    }
    else
    {
        /*否则，则是告警包含sib11 ，需要发送真实码流*/
        tRnlcRnluHeadInfo.wSystemInfoCause = CAUSE_SIB11_START;
    }
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));   
    /*将消息头信息拷入*/
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff, &tRnlcRnluHeadInfo, sizeof(T_RnlcRnluSystemInfoHead));
    /*转换指针类型*/
    ptSystemInfoETWS_up =(T_SystemInfoETWS_up *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff + sizeof(T_RnlcRnluSystemInfoHead));   

    ptSystemInfoETWS_up->dwETWSTransNum = (WORD32)ptWarningMsgSegmentList->wNumberofBroadcastRequest;

    if(RNLC_INVALID_DWORD == ptWarningMsgSegmentList->dwExtendedRepetitionPeriod)
    {
        ptSystemInfoETWS_up->dwETWSPeriod =(WORD32)ptWarningMsgSegmentList->wRepetitionPeriod;
    }
    else
    {
        ptSystemInfoETWS_up->dwETWSPeriod  = ptWarningMsgSegmentList->dwExtendedRepetitionPeriod;
    }

    ptSystemInfoETWS_up->dwSiPeriod =  (WORD32) ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod;
    ptSystemInfoETWS_up->wMsgLen = ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len;
    ptSystemInfoETWS_up->wSegNum = (WORD16)ptWarningMsgSegmentList->dwSib11SegNum;
    ptSystemInfoETWS_up->ucSiTransNum = ucSiTransNum;
    ptSystemInfoETWS_up->wFirstSegSize = (WORD16)ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[0].wMsgLen;
    if(1 < ptWarningMsgSegmentList->dwSib11SegNum)
    {
        ptSystemInfoETWS_up->wSecondSegSize = (WORD16)ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[1].wMsgLen;    
    }
    else
    {
        ptSystemInfoETWS_up->wSecondSegSize = 0;
    }
    /*增加一个用户面用于判断协议中几种特殊情况判断的字段*/
    if((1 == ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.m.bitConcurrentWarningMessageIndicatorPresent)
    &&(0 == ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.wNumberofBroadcastRequest))
    {

        ptSystemInfoETWS_up->ucInfiniteSend = 1;
    }
    else
    {
        ptSystemInfoETWS_up->ucInfiniteSend = 0;
    }
    ptSystemInfoETWS_up->ucSib11NonSuspend = g_ucSib11NonSuspend; 
        
    return TRUE;
}


WORD32 CCM_CIM_SIBroadcastComponentFsm::CimComposeSib12MsgToRnlu(T_RnlcRnluSystemInfoHead  *ptRnlcRnluSystemInfoHead,
                                                                                                                               TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList,
                                                                                                                               T_AllSibList *ptNewSibList)
{
    CCM_NULL_POINTER_CHECK(ptRnlcRnluSystemInfoHead);
    CCM_NULL_POINTER_CHECK(ptCmasWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptNewSibList);
    /*处理存在sib11告警的情况si*/

    WORD16  wAlignmentLen = 0;        /* 4字节对齐后长度 */
    WORD16 dwSib12HeaderLen =0;
    BYTE    ucSiTransNum  = 0;        /* 在SI window内SI传输次数 */
    static BYTE    aucSISib12MsgBuff[CIM_SYSINFO_MSG_SIB11_BUF_LEN];  /*存放 sib12 告警的广播码流缓存区 当前大小修改修订*/
    T_RnlcRnluSystemInfoHead  tRnlcRnluHeadInfo;       /* 发送到USM的结构 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_SystemInfoCMAS_up        *ptSystemInfoCMAS_up = NULL;
    T_RnlcRnluSystemInfoHead  *ptSystemInfoCMAS_Head_up =NULL;
    ptSystemInfoCMAS_Head_up = (T_RnlcRnluSystemInfoHead *)aucSISib12MsgBuff;

    memset(aucSISib12MsgBuff, 0, sizeof(aucSISib12MsgBuff));
    memset(&tRnlcRnluHeadInfo, 0, sizeof(tRnlcRnluHeadInfo));
    memmove(&tRnlcRnluHeadInfo,ptRnlcRnluSystemInfoHead,sizeof(T_RnlcRnluSystemInfoHead));
    tRnlcRnluHeadInfo.m.dwSib12Present =1;
    tRnlcRnluHeadInfo.m.dwSib11Present =0;
    tRnlcRnluHeadInfo.m.dwModiPeriodCoeffPresent  =1;
    tRnlcRnluHeadInfo.m.dwSystemInfoMasterPresent = 0;
    tRnlcRnluHeadInfo.m.dwSystemInfoListPresent = 0;
    tRnlcRnluHeadInfo.m.dwSystemInfo1Present  = 0;
    tRnlcRnluHeadInfo.m.dwSib10Present  = 0;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Composing SIB12 SI Msg to RNLU now...\n");   
    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));
       
    /*将消息头信息拷入*/
    memcpy(ptSystemInfoCMAS_Head_up, &tRnlcRnluHeadInfo, sizeof(T_RnlcRnluSystemInfoHead));
    /*转换指针类型*/
    ptSystemInfoCMAS_up =(T_SystemInfoCMAS_up *)(aucSISib12MsgBuff+ sizeof(T_RnlcRnluSystemInfoHead));
    /*si周期填写原来保存在构件数据区的大小*/
    ptSystemInfoCMAS_up->dwSiPeriod = (WORD32)ptCimSIBroadcastVar->tCimWarningInfo.ucSIB12SiPeriod;
    ptSystemInfoCMAS_up->dwCMASTransNum = (WORD32)ptCmasWarningMsgSegmentList->wNumberofBroadcastRequest;
    if(RNLC_INVALID_DWORD == ptCmasWarningMsgSegmentList->dwExtendedRepetitionPeriod)
    {
        ptSystemInfoCMAS_up->dwCMASPeriod = (WORD32)ptCmasWarningMsgSegmentList->wRepetitionPeriod;
    }
    else
    {
        ptSystemInfoCMAS_up->dwCMASPeriod  = ptCmasWarningMsgSegmentList->dwExtendedRepetitionPeriod;
    }
    ptSystemInfoCMAS_up->wCmasDelNum = 0; 
    ptSystemInfoCMAS_up->wCmasAddNum = 1;
    /*增加一个用户面用于判断协议中几种特殊情况判断的字段*/
    ptSystemInfoCMAS_up->ucInfiniteSend = ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.m.bitConcurrentWarningMessageIndicatorPresent;
    /*************************************************************************************/
    WORD16 wAllSiLen = 0;
    for (WORD32 dwSegLoop = 0;(dwSegLoop < ptCmasWarningMsgSegmentList->dwSib12SegNum)
        && (dwSegLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwSegLoop++)
    {
        if (FALSE == ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSegLoop].ucValid)
        {
            /* 此SI编码失败或者不能调度，不下发 */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SI-%lu not be send!\n", dwSegLoop + 1);
            continue;
        }
        WORD16 wSiLen = ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSegLoop].wMsgLen;
        wAllSiLen += wSiLen;
    }
    ptSystemInfoCMAS_up->tCmasAddList.wCmasMsgLen = wAllSiLen;
    ptSystemInfoCMAS_up->tCmasAddList.wCmasSegNum =(WORD16) ptCmasWarningMsgSegmentList->dwSib12SegNum;
    ptSystemInfoCMAS_up->tCmasAddList.wFirstSegSize = (WORD16) ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[0].wMsgLen;
    if(1 < ptCmasWarningMsgSegmentList->dwSib12SegNum)
    {
        ptSystemInfoCMAS_up->tCmasAddList.wSecondSegSize = ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[1].wMsgLen;
    }
    else
    {
        ptSystemInfoCMAS_up->tCmasAddList.wSecondSegSize = 0;
    }
    WORD16 wStreamId;
    memmove(&wStreamId , ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.tSerialNumber.data,2);
    ptSystemInfoCMAS_up->tCmasAddList.wCmasStreamId = wStreamId;
    WORD16 wCmasMsgId;
    memmove(&wCmasMsgId , ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.tMessageIdentifier.data,2);
    ptSystemInfoCMAS_up->tCmasAddList.wCmasMsgID = wCmasMsgId;
    memset(ptSystemInfoCMAS_up->atCmasDelList,0,MAX_CMAS_MSG*sizeof(T_CmasDelInfo));
    if(CIM_CAUSE_CMAS_SIB12_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    {
        /*将更新发送的sib12 列表放在这里更新，可以保证数据区维护的发送列表是可靠的*/
        BYTE ucOperatType = 1;
        CimUpdateCmasSendList(wCmasMsgId, wStreamId,ucOperatType);
    }
    if(CIM_CAUSE_CMAS_SIB12_KILL == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    {       
        ptSystemInfoCMAS_up->wCmasDelNum = 1;
        ptSystemInfoCMAS_up->wCmasAddNum = 0;
        ptSystemInfoCMAS_up->atCmasDelList[0].wCmasMsgID = ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillMsgId;
        ptSystemInfoCMAS_up->atCmasDelList[0].wCmasStreamId = ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillStreamId;
        BYTE ucOperatType = 0;
        CimUpdateCmasSendList(ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillMsgId, ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillStreamId,ucOperatType);
        /*如果是cmas kill的消息，只需要填写删除列表，然后直接返回*/
        //dwSib12HeaderLen = 3*sizeof(WORD32) +4*sizeof(BYTE) + 2*sizeof(WORD16) + MAX_CMAS_MSG*sizeof(T_CmasDelInfo) + 5*sizeof(WORD16) + 2*sizeof(BYTE) + sizeof(T_RnlcRnluSystemInfoHead);
        dwSib12HeaderLen = sizeof(T_SystemInfoCMAS_up) + sizeof(T_RnlcRnluSystemInfoHead);
        memmove(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff, aucSISib12MsgBuff, dwSib12HeaderLen);
        ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength = dwSib12HeaderLen ;
        return RNLC_SUCC;
    }
    if(CIM_CAUSE_CMAS_SIB12_STOP== ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    {
        ptSystemInfoCMAS_up->wCmasAddNum = 0;
        ptSystemInfoCMAS_up->wCmasDelNum = 0;
        BYTE ucOperatType = 0;
        for (WORD16 wCmasIndex = 0 ; wCmasIndex <MAX_CMAS_MSG; wCmasIndex++)
        {
            if (1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting)
            {
                ptSystemInfoCMAS_up->atCmasDelList[wCmasIndex].wCmasMsgID 
                = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID;
                ptSystemInfoCMAS_up->atCmasDelList[wCmasIndex].wCmasStreamId 
                = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId;
                ptSystemInfoCMAS_up->wCmasDelNum = ptSystemInfoCMAS_up->wCmasDelNum +1;
                CimUpdateCmasSendList(ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillMsgId, ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillStreamId,ucOperatType);
            }
        }
        /*如果是cmas stop的消息，只需要填写删除列表，然后直接返回*/
        //dwSib12HeaderLen = 3*sizeof(WORD32) +4*sizeof(BYTE) + 2*sizeof(WORD16) + MAX_CMAS_MSG*sizeof(T_CmasDelInfo) + 5*sizeof(WORD16) + 2*sizeof(BYTE) + sizeof(T_RnlcRnluSystemInfoHead);
        dwSib12HeaderLen = sizeof(T_SystemInfoCMAS_up) + sizeof(T_RnlcRnluSystemInfoHead);
        memmove(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff, aucSISib12MsgBuff, dwSib12HeaderLen);
        ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength = dwSib12HeaderLen ;
        return RNLC_SUCC;
    }
    if(CIM_CAUSE_CMAS_SIB12_UPD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    {
        ptSystemInfoCMAS_up->wCmasDelNum = 0;
        BYTE ucOperatType = 0;
        for (WORD16 wCmasIndex = 0 ; wCmasIndex <MAX_CMAS_MSG; wCmasIndex++)
        {
            if (1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting)
            {
                ptSystemInfoCMAS_up->atCmasDelList[wCmasIndex].wCmasMsgID 
                = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID;
                ptSystemInfoCMAS_up->atCmasDelList[wCmasIndex].wCmasStreamId 
                = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId;
                ptSystemInfoCMAS_up->wCmasDelNum = ptSystemInfoCMAS_up->wCmasDelNum +1;
                CimUpdateCmasSendList(ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillMsgId, ptCimSIBroadcastVar->tCimWarningInfo.wCmasKillStreamId,ucOperatType);
            }
        }
    }
    /**************************************************************************************/
    
    WORD16 wSib12Len =0;
    //dwSib12HeaderLen = 3*sizeof(WORD32) +4*sizeof(BYTE) + 2*sizeof(WORD16) + MAX_CMAS_MSG*sizeof(T_CmasDelInfo) + 5*sizeof(WORD16) + 2*sizeof(BYTE) + sizeof(T_RnlcRnluSystemInfoHead);
    dwSib12HeaderLen = sizeof(T_SystemInfoCMAS_up) + sizeof(T_RnlcRnluSystemInfoHead);
    /*拷贝对应si的码流到缓存区*/
    for (WORD16 wSib12SegNum =0;wSib12SegNum<(WORD16)ptCmasWarningMsgSegmentList->dwSib12SegNum;wSib12SegNum++)
    {   
        if ( TRUE == ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].ucValid)
        {
            wAlignmentLen = (WORD16)CimEncodeAlignment(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);  /* 对应si码流对齐后长度 */
            /*这里暂时先做一个保证不溢出的保护，cmas后面要重构*/
            if((dwSib12HeaderLen + wSib12Len + wAlignmentLen) > CIM_SYSINFO_MSG_SIB11_BUF_LEN)
            {
               CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 buf over flow! \n");
               continue;
            }
            memcpy(ptSystemInfoCMAS_up->tCmasAddList.aucData+ wSib12Len, ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].aucMsg, 
            ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);
            wSib12Len += wAlignmentLen;
            /* 先打印出码流，方便问题定位后续可以在此处设置一个开关*/
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CMAS SI-%lu Broadcast Stream! len = %d\n", wSib12SegNum + 1,ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);

            /* 打印出码流信息 */
            if (1 == g_dwCimPrintFlag)
            {
                CimDbgPrintMsg(ptSystemInfoCMAS_up->tCmasAddList.aucData, wSib12Len );
            }  
        }
        else
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CMAS SI-%lu Broadcast Stream Encode fail! not be send!\n", wSib12SegNum + 1);
        }
    }

    wSib12Len += dwSib12HeaderLen;
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength = wSib12Len ;
    memmove(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff, aucSISib12MsgBuff, wSib12Len );
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSaveMsgSendToRnlu
* 功能描述: 保存生成的广播码流到si构件的上下文数据区用于发送
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSaveMsgSendToRnlu(T_SysInfoUpdReq *ptSysInfoUpdReq,
                                                                        WORD16 wMsgLen,
                                                                        BYTE *ptBroadCastMsgStream)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptBroadCastMsgStream);
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause = ptSysInfoUpdReq->ucCellSIUpdCause;
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength = wMsgLen;
    memmove(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff, ptBroadCastMsgStream, wMsgLen);
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimComposeMsgToRnlu
* 功能描述: 广播适配FDD
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimComposeMsgToRnlu(T_SysInfoUpdReq *ptSysInfoUpdReq,
                                                                T_AllSibList *ptNewSibList,
                                                                T_BroadcastStream *ptNewBroadcastStream,
                                                                T_AllSiEncodeInfo *ptAllSiEncodeInfo,
                                                                TWarningMsgSegmentList *ptWarningMsgSegmentList,
                                                                TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptNewSibList);
    CCM_NULL_POINTER_CHECK(ptNewBroadcastStream);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

    WORD16  wSib8SiNo     = 0;        /* 描述SIB8信息 */
    static BYTE    aucSIBroadcastMsgBuff[CIM_SYSINFO_MSG_BUF_LEN];  /*存放普通广播码流缓存区*/  
    WORD32  dwSiNumLoop   = 0;        /* 遍历SI 编码结构 */
    WORD16  wMsgLen       = 0;        /* 需要发送给RNLU 的消息长度，4字节对齐 */
    WORD16  wMaxLen       = 0;        /* 最大缓冲区长度 */
    WORD16  wMibLen       = 0;        /* MIB长度 */
    WORD16  wSib1TbSize   = 0;        /* SIB1匹配的TB块大小 */
    WORD16  wAlignmentLen = 0;        /* 4字节对齐后长度 */
    BYTE    ucSiTransNum  = 0;        /* 在SI window内SI传输次数 */
    BYTE    ucValidSiNum  = 0;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_RnlcRnluSystemInfoHead  *ptRnlcRnluSystemInfoHead =NULL;    
    T_SystemInfoMaster_up      *ptSystemInfoMaster_up = NULL;
    T_SystemInfo1_up              *ptSystemInfo1_up = NULL;
    T_SystemInfo_up                *ptSystemInfo_up = NULL;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Composing Normal SI Msg to RNLU now...\n");
    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    wMaxLen = sizeof(aucSIBroadcastMsgBuff) - 16; /* 预留空区域，解决pc-lint */  
    memset(aucSIBroadcastMsgBuff, 0, sizeof(aucSIBroadcastMsgBuff));
    
    /* 发送过来的广播消息实际格式如下：
    TBcmUsmSystemInfoHead ＋ TSystemInfoMster_up（变长）＋TSystemInfo1_up（变长）
    ＋TSystemInfoList_up（变长） */

    /***START***********************填写TBcmUsmSystemInfoHead  **********************************/
    ptRnlcRnluSystemInfoHead = (T_RnlcRnluSystemInfoHead  *)aucSIBroadcastMsgBuff;
    ptRnlcRnluSystemInfoHead->m.dwModiPeriodCoeffPresent = 1;
    ptRnlcRnluSystemInfoHead->m.dwSystemInfoMasterPresent =1 ;
    ptRnlcRnluSystemInfoHead->m.dwSystemInfo1Present =1;
    ptRnlcRnluSystemInfoHead->m.dwSystemInfoListPresent = 0;
    ptRnlcRnluSystemInfoHead->m.dwReserved =0;
    if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
    {
        ptRnlcRnluSystemInfoHead->m.dwSib10Present =1 ;
    }
    ptRnlcRnluSystemInfoHead->m.dwSib11Present =0;
    ptRnlcRnluSystemInfoHead->m.dwSib12Present =0;
    if (1 <= ptAllSiEncodeInfo->dwSiNum)
    {
        /* 存在SI信息 */
        ptRnlcRnluSystemInfoHead->m.dwSystemInfoListPresent = 1;
    }
    if(0 == g_ucSib11WarnMsgSendType)
    {
        ptRnlcRnluSystemInfoHead->m.dwWarningMsgSendType = 0;
    }
    else
    {
        ptRnlcRnluSystemInfoHead->m.dwWarningMsgSendType = 1;
    }
    // 查找板内索引
    T_DBS_GetCellIndexBPL_REQ tGetCellIndexBPLReq;
    memset(&tGetCellIndexBPLReq, 0, sizeof(tGetCellIndexBPLReq));
    tGetCellIndexBPLReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tGetCellIndexBPLReq.wCallType = USF_MSG_CALL;

    T_DBS_GetCellIndexBPL_ACK tGetCellIndexBPLAck;
    memset(&tGetCellIndexBPLAck, 0, sizeof(tGetCellIndexBPLAck));

    BOOL bDbsRet = UsfDbsAccess(EV_DBS_GetCellIndexBPL_REQ, &tGetCellIndexBPLReq, &tGetCellIndexBPLAck);
    if ((!bDbsRet) || (0 != tGetCellIndexBPLAck.dwResult))
    {
        return 1;
    }
    ptRnlcRnluSystemInfoHead->wCellId = ptCimSIBroadcastVar->wCellId;
    /*ptRnlcRnluSystemInfoHead->ucCellIdInBoard = (BYTE) tGetCellIndexBPLAck.wCellIndexBPL;   */
        ptRnlcRnluSystemInfoHead->ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    ptRnlcRnluSystemInfoHead->wTimeStamp = 0;  /* sib8信息暂时填为0 */
    ptRnlcRnluSystemInfoHead->dwModiPeriod = ptNewSibList->tSib2Info.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff;
    ptRnlcRnluSystemInfoHead->dwSiWndSize = ptNewSibList->tSib1Info.si_WindowLength;
    /*补充填写SIB8的相关内容begin*/
    CimFillSib8No(&(ptNewSibList->tSib1Info.schedulingInfoList), &wSib8SiNo);
    if(0xffff == wSib8SiNo)
    {
         ptRnlcRnluSystemInfoHead->dwSib8CfgInfo = 0xffffffff;
    }
    else
    {
        //  ptRnlcRnluSystemInfoHead->dwSib8CfgInfo = g_dwSysTimeAndLongCodeExist<<31;/*系统时间和长码状态填写指示*/
        /*sib8cfginfo中的高两个bit 表示与用户名的约定，00 系统时间和长码状态都不填 01 填写系统时间 10 两则都填*/
        if(0 == g_dwSysTimeAndLongCodeExist)
        {
            ptRnlcRnluSystemInfoHead->dwSib8CfgInfo = 0;
        }
        if(1 == g_dwSysTimeAndLongCodeExist)
        {
            ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= 0x40000000;
        }
        if(2 == g_dwSysTimeAndLongCodeExist)
        {
            ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= 0x80000000;
        }    
        /*填充消息头中的sib8信息*/
        if ( g_bCcGPSLockState )
        {
             ptRnlcRnluSystemInfoHead->dwSib8CfgInfo &= 0xDFFFFFFF;/*高位第三个bit赋值为0*/
        }
        else
        {
             ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= 1<<29;/*第三个bit赋值为1*/
        }

        DWORD dTmp = wSib8SiNo;
        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= dTmp<<25;/*第四bit到第7比特赋值,已经移位处理过了*/

        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= ptNewSibList->dwSib8SystemTimeInfoOffset<<13;

        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= ptNewSibList->dw1XrttLongCodeOffset;
    }
    /*补充填写SIB8的相关内容end*/
    /*si个数是重新规划的接口结构中移过来的，在头信息中填写*/
    ucValidSiNum = 0;
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (TRUE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            ucValidSiNum++;
        }
    } 
    ptRnlcRnluSystemInfoHead->ucSiNum = ucValidSiNum;

    /* 填写SystemInfoCause  ，为实现方便，默认填为告警，如果是普通更新，则更新为普通更新*/ 
    if ((SI_UPD_CELL_SETUP == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (SI_UPD_CELL_ADD_byUNBLOCK == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (SI_UPD_CELL_UNBLOCK == ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_CELLSETUP ;
    }

    if ((SI_UPD_CELL_DBS_MODIFY == ptSysInfoUpdReq->ucCellSIUpdCause)
        || (SI_UPD_CELL_DELADD == ptSysInfoUpdReq->ucCellSIUpdCause)
        || (SI_UPD_CELL_RECFG_DEL == ptSysInfoUpdReq->ucCellSIUpdCause)
        || (SI_UPD_CELL_GPSLOCK == ptSysInfoUpdReq->ucCellSIUpdCause)
        || (SI_UPD_CELL_GPSUNLOCK == ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIMODIFY;
    }
    if((CIM_CAUSE_ETWS_SIB10_ADD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_ADD == ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        /*如果有告警发生先默认是系统广播更新*/
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIMODIFY;
        /*在有调度信息，并且确定可以下发的时候修改更新原因为告警*/
        if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10) ||
            (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11))
        {
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIB10_START ;
        }
    }
    if((CIM_CAUSE_ETWS_SIB10_STOP == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        /*只有sib11发送的场景是不会发送普通广播的，这里为了
           防止有意外情况加上这个原因*/
        (CIM_CAUSE_ETWS_SIB11_ADD == ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
        {
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIB10_STOP ;
        }
        else
        {
            /*可填写为si更新，不影响，因为在真正发给用户面的时候此时不发送普通广播*/
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIMODIFY;
        }
    }
    if((CIM_CAUSE_CMAS_SIB12_ADD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_UPD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_KILL== ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_STOP== ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        /*如果有告警发生先默认是系统广播更新*/
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_CMAS_INFO;
    }
    if (wMsgLen >= wMaxLen)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: send to RNLU Buffer area is very minor. CellId=%d\n",ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    wMsgLen += sizeof(T_RnlcRnluSystemInfoHead);
    
        /* ***END***********************填写TBcmUsmSystemInfoHead  **********************************/

        /* ***START***********************填写TSystemInfoMster_up  **********************************/
        ptSystemInfoMaster_up = (T_SystemInfoMaster_up *)(aucSIBroadcastMsgBuff + wMsgLen );
        wMibLen = ptNewBroadcastStream->ucMibLen;
        ptSystemInfoMaster_up->wMsgLen = (WORD16)wMibLen;
        wAlignmentLen = (WORD16)CimEncodeAlignment(wMibLen);  /* 对齐后长度 */
        /*加上mib后的当前长度*/
        wMsgLen  = wMsgLen + sizeof(WORD16) + 2*sizeof(BYTE) + wAlignmentLen;
        if (wMsgLen >= wMaxLen)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,
                                        "\n SI: send to RNLU Buffer area is very minor. CellId=%d\n",
                                        ptCimSIBroadcastVar->wCellId);
                                        return FALSE;
        }
        memcpy(ptSystemInfoMaster_up->aucData, ptNewBroadcastStream->aucMibBuf, wMibLen);
        /*保存一份到广播的数据区，用于比较更新*/
        memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucMibBuf, ptNewBroadcastStream->aucMibBuf, wMibLen);
        ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucMibLen = (BYTE)wMibLen;
   
        /* ***END***********************填写TSystemInfoMster_up  **********************************/

        /* ***START***********************填写TSystemInfo1_up  ************************************/
        ptSystemInfo1_up = (T_SystemInfo1_up *)(aucSIBroadcastMsgBuff + wMsgLen );
    for (int Sib1Index =0; Sib1Index<4;Sib1Index++)
    {
        wSib1TbSize = (WORD16)CimToMacAlignment(ptCimSIBroadcastVar->tSib1Buf[Sib1Index].wMsgLen);
        if (0 == wSib1TbSize)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: SIB1 stream len(=%lu) beyond TB size boundary!\n",wSib1TbSize);
            return FALSE;
        }

        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: Fill SIB1(%d) stream len(=%lu) with TB size(=%lu)!\n",Sib1Index,ptCimSIBroadcastVar->tSib1Buf[Sib1Index].wMsgLen,wSib1TbSize);

        wAlignmentLen = (WORD16)CimEncodeAlignment(wSib1TbSize);  /* 对齐后长度 */ 
        /*加上sib1后的当前长度*/
        wMsgLen = wMsgLen + sizeof(WORD16) + 2*sizeof(BYTE) + wAlignmentLen;

        if (wMsgLen >= wMaxLen)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: send to RNLU Buffer area is very minor.\n",ptCimSIBroadcastVar->wCellId);
            return FALSE;
        }
        ptSystemInfo1_up->wMsgLen = (WORD16)wSib1TbSize; 
        memcpy(ptSystemInfo1_up->aucData,
        ptCimSIBroadcastVar->tSib1Buf[Sib1Index].aucSib1Buf,
        ptSystemInfo1_up->wMsgLen); 
        /*更新指针到填入后的sib1 的末尾*/
        ptSystemInfo1_up = (T_SystemInfo1_up *)(aucSIBroadcastMsgBuff + wMsgLen );
    }
    /*保存一份sib1 到广播的数据区，用于比较更新*/
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[0].aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen);
    ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucSib1Len = (BYTE)ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen;
    /* ***END***********************填写TSystemInfo1_up  ************************************/

    ptSystemInfo_up = (T_SystemInfo_up *)(aucSIBroadcastMsgBuff + wMsgLen );
    /****************填写每个si信息  start**************************/
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            /* 此SI编码失败或者不能调度，不下发 */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SI-%lu not be send!\n",dwSiNumLoop + 1);
            continue;
        }
        /*处理普通si*/
        ptSystemInfo_up->dwSiPeriod = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].tSiPeriod;   /* 调度周期 */
        ptSystemInfo_up->ucSiTransNum = ucSiTransNum;   /* SI最大传输次数 */
        ptSystemInfo_up->wMsgLen = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen; /*  SI码流长度 */
        ptSystemInfo_up->wSegNum = 1;  /*  分片数量暂没有考虑分片 */
        ptSystemInfo_up->wSegSize = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen;
        /*拷贝对应si的码流到缓存区*/
        memcpy(ptSystemInfo_up->aucData, ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg, ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen);
        wAlignmentLen = (WORD16)CimEncodeAlignment(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen);  /* 对应si码流对齐后长度 */

        /*加上当前si 后的当前长度*/
        wMsgLen = wMsgLen + sizeof(T_SystemInfo_up) + wAlignmentLen;
        /*更新指针到当前sib1的末尾处，为下一个sib1拷贝做准备*/
        ptSystemInfo_up = (T_SystemInfo_up *)(aucSIBroadcastMsgBuff + wMsgLen );       

    }
  
    /* 打印出当前调度信息 */
    CimDbgShowCurBroadcastInfo(&(ptNewSibList->tSib1Info.schedulingInfoList));

    /*全部码流已经生成，保存发送给用户面的消息到cim的数据区 用来发送*/
    WORD32 dwResult = CimSaveMsgSendToRnlu(ptSysInfoUpdReq,wMsgLen,aucSIBroadcastMsgBuff);
    if(RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,
                        "\n SI: Save BroadCast Msg To SI Date Fail! CellId=%d\n",
                        ptCimSIBroadcastVar->wCellId);
    }
    /*在发送sib11 或者 停止sib11的场景需要组sib11的码流并发送*/
    if (( 1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimComposeSib11MsgToRnlu(ptRnlcRnluSystemInfoHead,ptWarningMsgSegmentList,ptNewSibList);                                                                                                                         
    }
    /*在发送sib12 或者 停止sib12的场景需要组sib12的码流并发送*/
    if (( 1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)||
        (CIM_CAUSE_CMAS_SIB12_KILL == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause )||
        (CIM_CAUSE_CMAS_SIB12_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimComposeSib12MsgToRnlu(ptRnlcRnluSystemInfoHead,ptCmasWarningMsgSegmentList,ptNewSibList);                                                                                                                         
    }
    
    CimSaveAllSibsStream(ptNewBroadcastStream, ptAllSiEncodeInfo, ptNewSibList);

    /* 打印出码流信息 */
    if (1 == g_dwCimPrintFlag)
    {
        CimDbgBroadcastStreamPrint(ptNewBroadcastStream, ptAllSiEncodeInfo);
    }
    return TRUE;
}



/*<FUNC>***********************************************************************
* 函数名称: CimSaveAllSibsStream
* 功能描述: 保存码流到实例数据区
* 算法描述:
* 全局变量:
* Input参数:
* 返 回 值:
**    
* 完成日期:
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CCM_CIM_SIBroadcastComponentFsm::CimSaveAllSibsStream(T_BroadcastStream *ptNewBroadcastStream,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo,
        T_AllSibList *ptAllSibList)
{
    BYTE                   bySibName;             /* SIB名 */
    WORD32                 dwSibNum;              /* 每个SI中SIB个数 */
    WORD32                 dwSibNumLoop;          /* 每个SI中SIB遍历 */
    WORD32                 dwSiNumLoop;           /* SI遍历 */
    BYTE                   aucSibList[DBS_SI_SPARE] = {0};
    SchedulingInfoList    *ptScheInfo = NULL;   /* 调度信息 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_VOID(ptNewBroadcastStream);
    CCM_NULL_POINTER_CHECK_VOID(ptAllSiEncodeInfo);
    CCM_NULL_POINTER_CHECK_VOID(ptAllSibList);

    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* 初始化sib List*/
    memmove((void*)aucSibList, (void*)ptAllSibList->aucSibList, CIM_MAX_SIB_LIST_LEN*sizeof(BYTE));

    /* 调度信息 */
    ptScheInfo = &(ptAllSibList->tSib1Info.schedulingInfoList);

    /* 依据SI个数填写 */
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            dwSibNum = ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.n;

            for (dwSibNumLoop = 0; dwSibNumLoop < dwSibNum; dwSibNumLoop++)
            {
                if ((0 == dwSiNumLoop) && (0 == dwSibNumLoop))
                {
                    /* SIB2所在的SI由于编码失败，不下发，在SIB列表中剔除 */
                    aucSibList[DBS_SI_SIB2] = FALSE;
                }

                bySibName = (BYTE)ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.elem[dwSibNumLoop];

                switch (bySibName)
                {
                case sibType3:
                {
                    aucSibList[DBS_SI_SIB3] = FALSE; /* SIB3 */

                    break;
                }
                case sibType4:
                {
                    aucSibList[DBS_SI_SIB4] = FALSE; /* SIB4 */

                    break;
                }
                case sibType5:
                {
                    aucSibList[DBS_SI_SIB5] = FALSE; /* SIB5 */

                    break;
                }
                case sibType6:
                {
                    aucSibList[DBS_SI_SIB6] = FALSE; /* SIB6 */

                    break;
                }
                case sibType7:
                {
                    aucSibList[DBS_SI_SIB7] = FALSE; /* SIB7 */

                    break;
                }
                case sibType8:
                {
                    aucSibList[DBS_SI_SIB8] = FALSE; /* SIB8 */

                    break;
                }
                case sibType9:
                {
                    aucSibList[DBS_SI_SIB9] = FALSE; /* SIB9 */

                    break;
                }
                case sibType10:
                {
                    aucSibList[DBS_SI_SIB10] = FALSE; /* SIB10 */

                    break;
                }

                case sibType11:
                {
                    aucSibList[DBS_SI_SIB11] = FALSE; /* SIB11 */

                    break;
                }
                 case sibType12_v920 :
                {
                    aucSibList[DBS_SI_SIB12] = FALSE; /* SIB12 */

                    break;
                }
                default :
                {
                        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: bySibName=%d error!\n",bySibName);
                    break;
                }
                }
            }
        }
    }

    /* 更新SIB列表 */
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList, aucSibList, sizeof(ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList));

    /* 保存码流 */
    memcpy(&(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream), ptNewBroadcastStream, sizeof(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream));

    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSendBroadcastMsgToRnlu
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSendBroadcastMsgToRnlu( )
{

    T_CIMVar     *ptCIMVar = NULL;
    BYTE           * pucMsg = NULL;
    WORD16      wMsgLe = 0;
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /* 获取CIM小区实例信息 */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    PID tRNLUPid = ptCIMVar->tCIMPIDinfo.tRNLUPid;
#ifdef VS2008
    tRNLUPid.wUnit = 1;
    tRNLUPid.wModule = 1;
    tRNLUPid.ucSUnit = 2;
    tRNLUPid.ucSubSystem = 2;
    tRNLUPid.dwPno = 0x30000001;
    tRNLUPid.dwDevId = 0x10000001;
    tRNLUPid.ucRouteType = 2;
#endif
    /*sib12 告警发送。*/
    if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)||
        (CIM_CAUSE_CMAS_SIB12_KILL == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause )||
        (CIM_CAUSE_CMAS_SIB12_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        if((CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
            (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {        
            if (( RNLC_INVALID_TIMERID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib12RepeatTimer)&&
                (INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib12RepeatTimer))
            {
                //CimKillWarningSib12Timer();
            }
        }
        else
        {
            /*启动告警定时器*/    
            CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),12);
        }
        pucMsg = (BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff);
        wMsgLe = ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength;
        /*当需要真实的用户面回应的时候*/
        if (1 == g_tSystemDbgSwitch.ucRNLUDbgRSPSwitch)
        {
            WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RNLU_SYSINFO_REQ,
                                                        pucMsg,
                                                        wMsgLe,
                                                        COMM_RELIABLE,
                                                        &tRNLUPid);
            if (OSS_SUCCESS != dwOssStatus)
            {
                CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: Cim Send Broadcast Msg To Rnlu Fail, CellId=%d!\n",
                ptCimSIBroadcastVar->wCellId);
                return RNLC_FAIL;
            }
        }
        /* 信令跟踪 */
        RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                        EV_RNLC_RNLU_SYSINFO_REQ,
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_INVALID_WORD,
                        RNLC_INVALID_WORD,
                        RNLC_ENB_TRACE_SENT,
                        wMsgLe ,
                        pucMsg);
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Send CMAS Msg To Rnlu!!. ValueTag=%lu CellId=%d size=%d\n",
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
        ptCimSIBroadcastVar->wCellId, wMsgLe);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        return RNLC_SUCC;
    }  

    /*普通广播发送。当需要真实的用户面回应的时候*/
    if (1 == g_tSystemDbgSwitch.ucRNLUDbgRSPSwitch)
    {
        pucMsg = (BYTE*) ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff;
        wMsgLe = (WORD16) ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength;
        /*如果是告警中下面情况引起的广播更新 判断是否有必要发送普通广播*/
        if((CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        /*只有sib11初始增加的场景是不应该发送普通广播的所以这里加上这个原因*/
        (CIM_CAUSE_ETWS_SIB11_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
            if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
            {
                WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RNLU_SYSINFO_REQ,
                                                            pucMsg,
                                                            wMsgLe,
                                                            COMM_RELIABLE,
                                                            &tRNLUPid);
                if (OSS_SUCCESS != dwOssStatus)
                {
                    CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: send to RNLU FAIL(SIB10 STOP), CellId=%d!\n",ptCimSIBroadcastVar->wCellId);
                    return RNLC_FAIL;
                }
                /*将发送的标志位置0*/
                ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting = 0;
                /*杀掉原来的定时器*/
                if(INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer)
                {
                    CimKillWarningSib10Timer();
                }
                /* 信令跟踪 */
                RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                                EV_RNLC_RNLU_SYSINFO_REQ,
                                ptCimSIBroadcastVar->wCellId,
                                RNLC_INVALID_WORD,
                                RNLC_INVALID_WORD,
                                RNLC_ENB_TRACE_SENT,
                                wMsgLe,
                                pucMsg);
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI send to RNLU SUCCESS( Include SIB10 STOP))!. ValueTag=%lu CellId=%d size=%d\n",
                ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
                ptCimSIBroadcastVar->wCellId, wMsgLe);
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive SIB10 Stop message and sib10 already stoped!. ValueTag=%lu CellId=%d \n",
                            ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
                            ptCimSIBroadcastVar->wCellId);
            }
        }
        else
        /*对普通广播引起的广播更新则直接下发普通广播*/
        { 
            WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RNLU_SYSINFO_REQ,
            pucMsg,
            wMsgLe,
            COMM_RELIABLE,
            &tRNLUPid);
            if (OSS_SUCCESS != dwOssStatus)
            {
                CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: SI send to RNLU FAIL!, CellId=%d!\n",
                ptCimSIBroadcastVar->wCellId);
                return RNLC_FAIL;
            }
            /* 信令跟踪 */
            RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                            EV_RNLC_RNLU_SYSINFO_REQ,
                            ptCimSIBroadcastVar->wCellId,
                            RNLC_INVALID_WORD,
                            RNLC_INVALID_WORD,
                            RNLC_ENB_TRACE_SENT,
                            wMsgLe,
                            pucMsg);
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Normal SI send to RNLU SUCCESS!. ValueTag=%lu CellId=%d size=%d\n",
            ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
            ptCimSIBroadcastVar->wCellId, wMsgLe);
            /*统计记录最近几次的普通si更新情况*/
            CimRecordToRNLUDbgInfo();
        }
    /*如果是上面esle 并且包含 sib10的情况，这里打印报出并置位*/
    if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
    { 
            CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: This SI To RNLU Include SIB10, CellId=%d!\n",ptCimSIBroadcastVar->wCellId);
        /*启动告警定时器*/
        CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),10);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
            /*对sib10发送标志位的置1 只能在这里处理，以最终的发送为依据*/
        ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting = 1;
    }
    }

    
    /*sib11 告警发送。*/
    /*标识这次广播已经下发下去，需要把本次的发送的标志位置0 等待下次告警来临,并将相关告警发送表示置1*/
    if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)||
     (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
     (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        if((CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
            if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib11Broadcasting)
            {
                if(INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer)
                {
                    CimKillWarningSib11Timer();
                }
                ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib11Broadcasting = 0;
            }
        }
        else
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib11Broadcasting = 1;
            /*启动告警定时器*/    
            CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),11);
        }
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len = 0;
        pucMsg = (BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff);
        wMsgLe = ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib11MsgLength + sizeof(T_RnlcRnluSystemInfoHead) + sizeof(T_SystemInfoETWS_up);

        /*当需要真实的用户面回应的时候*/
        if (1 == g_tSystemDbgSwitch.ucRNLUDbgRSPSwitch)
        {
        WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RNLU_SYSINFO_REQ,
                                                    pucMsg,
                                                    wMsgLe,
                                                    COMM_RELIABLE,
                                                    &tRNLUPid);
        if (OSS_SUCCESS != dwOssStatus)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: Send SI (SIB11) To Rnlu Fail, CellId=%d!\n",
            ptCimSIBroadcastVar->wCellId);
            return RNLC_FAIL;
        }
        }
        /* 信令跟踪 */
        RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                        EV_RNLC_RNLU_SYSINFO_REQ,
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_INVALID_WORD,
                        RNLC_INVALID_WORD,
                        RNLC_ENB_TRACE_SENT,
                        wMsgLe,
                        pucMsg);

        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: send SI (SIB11) to RNLU SUCCESS!. ValueTag=%lu CellId=%d size=%d\n",
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
        ptCimSIBroadcastVar->wCellId,
        wMsgLe);
    }
    return RNLC_SUCC;
}


WORD32 CCM_CIM_SIBroadcastComponentFsm::CimUpdateCmasSendList(WORD16 wCmasMsgId, WORD16 wCmasStreamId,BYTE ucOperatType)
{
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /*0 标示删除对应的告警信息*/
    if(0 == ucOperatType)
    {
        for (WORD16 wCmasIndex = 0; wCmasIndex<MAX_CMAS_MSG; wCmasIndex++)
        {
            if((wCmasMsgId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID)&&
            (wCmasStreamId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId))
            {
                /*相应标示置0*/
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting = 0;
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId = 0;
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID = 0;
                /*更新发送列表*/
                for (WORD16 wTemIndex = wCmasIndex; wTemIndex<MAX_CMAS_MSG -1; wTemIndex++)
                {
                    ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex].wCmasMsgID 
                    = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex+1].wCmasMsgID;
                    ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex].wCmasStreamId 
                    = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex+1].wCmasStreamId;
                    ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex].aucIsCurrentBroadcasting
                    = ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wTemIndex+1].aucIsCurrentBroadcasting;
                }
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d del succ!\n",
            wCmasMsgId,wCmasStreamId);
            return RNLC_SUCC;
        }
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d is not include in cmas list,can not remove !\n",
        wCmasMsgId,wCmasStreamId);
        return RNLC_FAIL;
    }
    /*1 标示增加对应的告警信息*/
    if(1 == ucOperatType)
    {
        for(WORD16 wCmasIndex = 0; wCmasIndex<MAX_CMAS_MSG; wCmasIndex++)
        {
            if((1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting)&&
            ( wCmasIndex == MAX_CMAS_MSG -1))
            {
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d can not ADD, cmas list  is full!\n",
                wCmasMsgId,wCmasStreamId);
                return RNLC_FAIL;
            }
            if((1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting)&&
            ( wCmasIndex < MAX_CMAS_MSG -1))
            {
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex+1].aucIsCurrentBroadcasting = 1; 
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex+1].wCmasMsgID = wCmasMsgId;
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex+1].wCmasStreamId = wCmasStreamId;
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d ADD to cmas list succ!\n",
                wCmasMsgId,wCmasStreamId);
                return RNLC_SUCC;
            }
        }
        if(0 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[0].aucIsCurrentBroadcasting)
        {
            ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[0].aucIsCurrentBroadcasting = 1; 
            ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[0].wCmasMsgID = wCmasMsgId;
            ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[0].wCmasStreamId = wCmasStreamId;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d ADD to first of cmas list succ!\n",
            wCmasMsgId,wCmasStreamId);
            return RNLC_SUCC;
        }
    }
    CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: wCmasMsgId =%d , wCmasStreamId=%d  no this situation!\n",
    wCmasMsgId,wCmasStreamId);
    return RNLC_FAIL;   
}

/*<FUNC>***********************************************************************
* 函数名称: CimSetSystemInfoTimer
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSetSystemInfoTimer(VOID)
{
    WORD32   dwTimerId = INVALID_TIMER_ID;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* 获取CIM广播信息 */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* 根据小区ID获取TimerId */
    dwTimerId = USF_OSS_SetRelativeTimer((BYTE)TIMER_CCM_CIM_SYSTEMINFO, (WORD32)1000,
                                        (WORD32)ptCimSIBroadcastVar->wCellId);

    if (INVALID_TIMER_ID != dwTimerId)
    {
        /* print */
        ptCimSIBroadcastVar->dwSysInfoTimerId = dwTimerId;

        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CIM Set Relative Timer Success! TimerId=%d code = %d\n",
                    dwTimerId,ERR_CCM_CIM_SIBROADCAST_SET_SYSINFO_TIMER);

        return RNLC_SUCC;
    }
    else
    {
        /* print */
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM Set Relative Timer Fail!code = %d\n",
                ERR_CCM_CIM_SIBROADCAST_SET_SYSINFO_TIMER);
        return RNLC_FAIL;
    }

}

/*<FUNC>***********************************************************************
* 函数名称: CimKillSystemInfoTimer
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimKillSystemInfoTimer()
{
    WORD32 dwResult;
    dwResult = OSS_KillTimerByTimerId(GetComponentContext()->tCimSIBroadcastVar.dwSysInfoTimerId);
    if (OSS_SUCCESS != dwResult)
    {
        /* print */
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM Kill System Info Timer Fail!\n");
        return RNLC_FAIL;
    }
    GetComponentContext()->tCimSIBroadcastVar.dwSysInfoTimerId = INVALID_TIMER_ID;
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSetWarningTimer
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSetWarningTimer(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsgReq, BYTE ucTimerType)
{
    T_CIMVar     *ptCIMVar = NULL;
    WORD32       dwResult  = RNLC_SUCC;
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarnMsgReq);
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    switch (ucTimerType)
    {
        case 10:
        {
            if(PWS_TYPE_ETWS == GetWarningType(ptCpmCcmWarnMsgReq->tMessageIdentifier))
            {
                /* 如果发送的是主通知信息，
                按Repetition Period=0和Number of Broadcast Request=1来处理Primary通知
                因此不启动主通知定时器 */

                if ((1 == ptCpmCcmWarnMsgReq->m.bitWarningTypePresent)
                    && ( CIM_SIB10_STOP_CBE == g_dwSib10SwtichMode)
                    &&(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10))
                {
                    ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = INVALID_TIMER_ID;
                    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: (CBE Contorl mode)Not Start Timer!\n");
                }
                else if ((1 == ptCpmCcmWarnMsgReq->m.bitWarningTypePresent)
                        && ( CIM_SIB10_STOPCBEANDTIMER == g_dwSib10SwtichMode )
                        &&(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10))
                {
                    dwResult = CimSetSib10SpecialTimer(ptCimSIBroadcastVar, ptCpmCcmWarnMsgReq);
                }
            }
            break;
        }
        case 11:
        {
            if(PWS_TYPE_ETWS == GetWarningType(ptCpmCcmWarnMsgReq->tMessageIdentifier))
            {
                /* 如果发送的是辅通知信息，启动辅通知定时器 */
                if ((1 == ptCpmCcmWarnMsgReq->m.bitWarningMsgContentsPresent)
                    && (1 == ptCpmCcmWarnMsgReq->m.bitDataCodingSchemePresent)
                    && (0 != ptCpmCcmWarnMsgReq->wRepetitionPeriod)
                    && (0 != ptCpmCcmWarnMsgReq->wNumberofBroadcastRequest)
                    &&(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11))
                {
                    dwResult = CimSetSib11Timer(ptCimSIBroadcastVar, ptCpmCcmWarnMsgReq);
                }
                /*    */
                else if ((1 == ptCpmCcmWarnMsgReq->m.bitWarningMsgContentsPresent)
                        && (1 == ptCpmCcmWarnMsgReq->m.bitDataCodingSchemePresent)
                        && (0 == ptCpmCcmWarnMsgReq->wRepetitionPeriod)
                        && (1 == ptCpmCcmWarnMsgReq->wNumberofBroadcastRequest)
                        &&(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11))
                {
                    dwResult = CimSetSib11SpecialTimer(ptCimSIBroadcastVar, ptCpmCcmWarnMsgReq);
                }
            }
            break;
        }
        case 12:
        {
            if((PWS_TYPE_CMAS == GetWarningType(ptCpmCcmWarnMsgReq->tMessageIdentifier))
                &&(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12))
            {
                dwResult = CimSetSib12Timer(ptCimSIBroadcastVar, ptCpmCcmWarnMsgReq);
            }
            break;
        }
        default :
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: This Type Timer have not defined!\n");
            dwResult = RNLC_FAIL;
            break;
        }
    }
    return dwResult;
}

/*<FUNC>***********************************************************************
* 函数名称: CimKillWarningSib10Timer
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimKillWarningSib10Timer()
{
    WORD32   dwResult = RNLC_SUCC;
     /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);    
    if ( INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer )
    {
        dwResult = OSS_KillTimerByTimerId(ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer);
        if (OSS_SUCCESS != dwResult)
        {
            /* print */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: CIM Kill sib10 System Info Timer Fail!Error Code = %d\n",ERR_CCM_CIM_SIBROADCAST_KILL_SYSINFO_TIMER);
            return FALSE;
        }
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = INVALID_TIMER_ID;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: CIM Kill sib10 System Info Timer SUCCESS!\n");
    }
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimKillWarningSib11Timer
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimKillWarningSib11Timer()
{
    WORD32   dwResult = RNLC_SUCC;
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);    
    if ( INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer )
    {
        dwResult = OSS_KillTimerByTimerId(ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer);
        if (OSS_SUCCESS != dwResult)
        {
            /* print */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: CIM Kill sib11 System Info Timer Fail!Error Code = %d\n",ERR_CCM_CIM_SIBROADCAST_KILL_SYSINFO_TIMER);
            return FALSE;
        }
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer = INVALID_TIMER_ID;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: CIM Kill sib11 System Info Timer SUCCESS!\n");
    }
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSendMsgToOtherComponent
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CCM_CIM_SIBroadcastComponentFsm::CimSendMsgToOtherComponent(BYTE *pucMsg,
        WORD16 wMsgLen,
        WORD16 wEvent,
        WORD16 wDestComponentId)
{
    WORD32                  dwResult               = SSL_OK;
    Message                 tSysInfoUpdRspMsg;
    tSysInfoUpdRspMsg.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tSysInfoUpdRspMsg.m_wLen = wMsgLen;
    tSysInfoUpdRspMsg.m_pSignalPara = static_cast<void*>(pucMsg);
    tSysInfoUpdRspMsg.m_dwSignal = wEvent;

    /* 给建立构件回响应消息 */
    dwResult = SendTo(wDestComponentId, &tSysInfoUpdRspMsg);
    if (SSL_OK != dwResult)
    {
        /* print */
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_SEND_MSG_TO_RNLU_ADAPT,
                                    0,
                                    0,
                                    RNLC_ERROR_LEVEL,
                                    "\n SIBroadcast: CIM Send to CellSetupComponent Rsp Fail!\n");
    }
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimNmlBrdUpdDealWithSib10Info
* 功能描述: 处理普通广播更新时是否填写sib10信息的问题
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimNmlBrdUpdDealWithSib10Info(T_AllSibList  *ptAllSibList, T_SysInfoUpdReq  *ptSysInfoUpdReq)
{
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptSysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    WORD32 dwResult = 0;

    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
      /*只有当普通广播更新，并且之前sib10正在发送时才处理*/
    if(TRUE == CimNormalBroadMsgWithSib10())
    {
        /*不确定此次普通广播更新是否是修改的sib10的调度信息，所以这里需要判断*/
        if(TRUE == ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /*注意这里使用的是上次成功发送的告警的信息填写的sib10 信息*/
            TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg);
            BOOL bResultSib10 = CimFillSib10Info(ptCpmCcmWarnMsg, ptSysInfoUpdReq, ptAllSibList);
            if ( !bResultSib10)
            {
                ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
                CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: Normal SI Update with sib10 sending  Fill SIB10 info fail!\n");  
                CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType10);

                tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
                CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
                /*这里借用调度信息对码流的影响的值返回sib10发送时普通广播更新
                中对sib10信息的填写情况*/
                dwResult = 1;
            }
            ptCimSIBroadcastVar->tSIBroadcastDate.ucNormalSibWithSib10Info = 1;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Normal SI Update with sib10 sending  Fill SIB10 info succful!\n");  
        }
        else
        {
            dwResult = 2;
            CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: Normal SI Update with sib10 sending  But No SIB10 schedule Info!\n");  
        }
    }
    return dwResult;
}
/*<FUNC>***********************************************************************
* 函数名称: CimWarnMsgHander
* 功能描述: 处理etws告警信息
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimWarnMsgSib10Hander(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TWarningMsgSegmentList *ptWarningMsgSegList,
        T_SysInfoUpdReq *ptSysInfoUpdReq,
        T_AllSibList *ptAllSibList)
{
    BOOL           bResultSib10           = FALSE;
    SchedulingInfoList tSchedulingInfoList;
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK(ptWarningMsgSegList);
    CCM_NULL_POINTER_CHECK(ptAllSibList);

    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    memset(&tSchedulingInfoList, 0x00, sizeof(tSchedulingInfoList));
    bResultSib10 = CimIsBoardIncludeSib10(ptCpmCcmWarnMsg);
    if (bResultSib10)
    {
        bResultSib10 = CimFillSib10Info(ptCpmCcmWarnMsg, ptSysInfoUpdReq, ptAllSibList);
        if ( !bResultSib10)
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib10Hander Fill SIB10 FAIL!(%u)\n",
                        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10);  
            CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType10);

            tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
            CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        }
    }
    else
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib10Hander Fill SIB10 FAIL! No sib10 info!(%u)\n",
                        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10);  
        CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType10);

        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    } 
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimWarnMsgHander
* 功能描述: 处理etws告警信息
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimWarnMsgSib11Hander(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
                                                                                                                            TWarningMsgSegmentList *ptWarningMsgSegList,
                                                                                                                            T_SysInfoUpdReq *ptSysInfoUpdReq,
                                                                                                                            T_AllSibList *ptAllSibList)
{
    BOOL           bResultSib11           = FALSE;
    SchedulingInfoList tSchedulingInfoList;
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK(ptWarningMsgSegList);
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    memset(&tSchedulingInfoList, 0x00, sizeof(tSchedulingInfoList));
    bResultSib11= CimIsBoardIncludeSib11(ptCpmCcmWarnMsg);
    if ( bResultSib11 )
    {
        bResultSib11 = CimSecondaryWarningInfoHandle( ptCpmCcmWarnMsg, ptWarningMsgSegList, ptSysInfoUpdReq, ptAllSibList );
        if ( !bResultSib11 )
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimSecondaryWarningInfoHandle FAIL!(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);  
            /* 除掉调度信息 */
            CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType11);

            tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
            CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        }
    }
    else
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib11Hander FAIL ,because there is no sib11 info!(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);  
        /* 除掉调度信息 */
        CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType11);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    }
    return RNLC_SUCC;
}


/*<FUNC>***********************************************************************
* 函数名称: CimWarnMsgHander
* 功能描述: 处理etws告警信息
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimWarnMsgSib12Hander(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
                                                                                                                            TCmasWarningMsgSegmentList  *ptCmasWarningMsgSegmentList,
                                                                                                                            T_SysInfoUpdReq *ptSysInfoUpdReq,
                                                                                                                            T_AllSibList *ptAllSibList)
{
    SchedulingInfoList tSchedulingInfoList;
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK(ptCmasWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    memset(&tSchedulingInfoList, 0x00, sizeof(tSchedulingInfoList));
    /*在s1口的告警处理中已经进行了是否有sib12的内容判断，这里可以直接填写*/
    BOOL  bResult = CimFillSib12Info(ptCpmCcmWarnMsg, ptCmasWarningMsgSegmentList);
    if ( !bResult)
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib11Hander FAIL ,because there is no sib11 info!(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);  
        /* 除掉调度信息 */
        CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        return RNLC_FAIL ;
    }
    /* 拷贝出来sib12的第一片信息，好像没啥用，暂时先放这 */
    memmove(&ptAllSibList->tSib12Info, &ptCmasWarningMsgSegmentList->atSib12[0], sizeof(SystemInformationBlockType12_r9));
    return RNLC_SUCC;
    
}

/*<FUNC>***********************************************************************
* 函数名称: CimIsBoardIncludeSib10
* 功能描述:判断是否含有sib10
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimIsBoardIncludeSib10(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg)
{
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    if (1 == ptCpmCcmWarnMsg->m.bitWarningTypePresent)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*<FUNC>***********************************************************************
* 函数名称: CimFillSib10Info
* 功能描述:填充sib10的内容
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimFillSib10Info(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        T_SysInfoUpdReq *ptSysInfoUpdReq,
        T_AllSibList *ptAllSibList)
{
    SystemInformationBlockType10 tSib10;

    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptSysInfoUpdReq);

    if (1 == ptCpmCcmWarnMsg->m.bitWarningTypePresent)
    {
        /* 填写主通知 */
        if (1 == ptCpmCcmWarnMsg->m.bitWarningSecurityInfoPresent)
        {
            ptAllSibList->tSib10Info.m.warningSecurityInfoPresent = 1;
            memmove(&ptAllSibList->tSib10Info.warningSecurityInfo,
            &ptCpmCcmWarnMsg->tWarningSecurityInfo,
            sizeof(SystemInformationBlockType10_warningSecurityInfo));
        }
        else
        {
            ptAllSibList->tSib10Info.m.warningSecurityInfoPresent = 0;
        }

        memmove(&ptAllSibList->tSib10Info.messageIdentifier,
                    &ptCpmCcmWarnMsg->tMessageIdentifier,
                    sizeof(SystemInformationBlockType10_messageIdentifier));

        memmove(&ptAllSibList->tSib10Info.serialNumber,
                    &ptCpmCcmWarnMsg->tSerialNumber,
                    sizeof(SystemInformationBlockType10_serialNumber));

        memmove(&ptAllSibList->tSib10Info.warningType,
                &ptCpmCcmWarnMsg->tWarningType,
                sizeof(SystemInformationBlockType10_warningType));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    /* 填写主通知 */
    memmove(&ptAllSibList->tSib10Info, &tSib10, sizeof(SystemInformationBlockType10));
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSecondaryWarningInfoHandle
* 功能描述:填充sib11内容
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimSecondaryWarningInfoHandle(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TWarningMsgSegmentList *ptWarningMsgSegList,
        T_SysInfoUpdReq *ptSysInfoUpdReq,
        T_AllSibList *ptAllSibList)
{
    BOOL  bResult = FALSE;
    ASN1CTXT  tCtxt;
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptWarningMsgSegList);
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptSysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    memset(&tCtxt,0,sizeof(ASN1CTXT));
    /* 判断是否存在sib11 */
    if ( (1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
        && ( 1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent))
    {
        bResult = CimFillSib11Info(ptCpmCcmWarnMsg, ptWarningMsgSegList);
        if ( !bResult)
        {
            ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI:CimFillSib11Info return FALSE(%u)\n ",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);
            return FALSE;
        }
        /* 拷贝出来sib11信息 */
        memmove(&ptAllSibList->tSib11Info, &ptWarningMsgSegList->atSib11[0], sizeof(SystemInformationBlockType11));
        return TRUE;
        }
    else
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CimSecondaryWarningInfoHandle ,no sib11 info(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);
        return FALSE;
    }

}

/*<FUNC>***********************************************************************
* 函数名称: CimIsBoardIncludeSib11
* 功能描述:是否含有sib11
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimIsBoardIncludeSib11(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg)
{
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);

    if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
        &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
        && (0 != ptCpmCcmWarnMsg->wRepetitionPeriod)
        && (0 != ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {

        return TRUE;
    }
    else if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
        &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
        && (0 == ptCpmCcmWarnMsg->wRepetitionPeriod)
        && (1 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {
        return TRUE;
    }
    /*增加协议413添加的对无限发送场景的支持*/
    if((1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    &&(0 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest)
    &&(0 != ptCpmCcmWarnMsg->wRepetitionPeriod))
    {
        return TRUE;
    }
    /*对r9协议升级新加的这个字段需要增加判断*/
    if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
    &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
    &&(1 == ptCpmCcmWarnMsg->m.bitExtendedRepetitionPeriodPresent)
    && (0 != ptCpmCcmWarnMsg->dwExtendedRepetitionPeriod)
    && (0 != ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {
        return TRUE;
    }
    return FALSE;
}


/*<FUNC>***********************************************************************
* 函数名称: CimIsBoardIncludeSib12
* 功能描述:是否含有sib12
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimIsBoardIncludeSib12(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg)
{
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
        &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
        && (0 != ptCpmCcmWarnMsg->wRepetitionPeriod)
        && (0 != ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {

        return TRUE;
    }
    else if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
         &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
         && (0 == ptCpmCcmWarnMsg->wRepetitionPeriod)
         && (1 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {
        return TRUE;
    }
    /*增加协议413添加的对无限发送场景的支持*/
    if((1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    &&(0 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest)
    &&(0 != ptCpmCcmWarnMsg->wRepetitionPeriod))
    {
        return TRUE;
    }
    /*对r9协议升级新加的这个字段需要增加判断*/
    if ((1 == ptCpmCcmWarnMsg->m.bitWarningMsgContentsPresent)
    &&(1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
    &&(1 == ptCpmCcmWarnMsg->m.bitExtendedRepetitionPeriodPresent)
    && (0 != ptCpmCcmWarnMsg->dwExtendedRepetitionPeriod)
    && (0 != ptCpmCcmWarnMsg->wNumberofBroadcastRequest))
    {
        return TRUE;
    }
    return FALSE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib11Info
* 功能描述:填充sib11
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimFillSib11Info(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TWarningMsgSegmentList *ptWarningMsgSegList)
{
    WORD32                   dwSegNum                        = 0;/* 分片数量 */
    WORD32                   dwLastSegSize                   = 0;/* 最后一片大小 */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptWarningMsgSegList);
    if (0==ptCpmCcmWarnMsg->tWarningMessageContents.numocts)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Warning Message Contents is 0!\n");
        return FALSE;
    }
    /* 分片数量 */
    dwSegNum = (ptCpmCcmWarnMsg->tWarningMessageContents.numocts + g_dwS1WarnMsgLen - 1) / g_dwS1WarnMsgLen;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB11 SegNum is %d! \n", dwSegNum);
    if (RNLC_CCM_MAX_SEGMENT_NUM<dwSegNum)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 seg num is %d!\n", dwSegNum);
        return FALSE;
    }
    /* 最后一个分片的大小 */
    dwLastSegSize = ptCpmCcmWarnMsg->tWarningMessageContents.numocts%g_dwS1WarnMsgLen;
    if ( (0 == dwLastSegSize) && (0 != ptCpmCcmWarnMsg->tWarningMessageContents.numocts) )
    {
        dwLastSegSize = g_dwS1WarnMsgLen;
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:dwLastSegSize = %u \n",dwLastSegSize);
    /* 将分片信息填写 */
    ptWarningMsgSegList->dwSib11SegNum = dwSegNum;
    ptWarningMsgSegList->dwSegSize     = g_dwS1WarnMsgLen;
    ptWarningMsgSegList->dwLastSegSize = dwLastSegSize;
    ptWarningMsgSegList->wNumberofBroadcastRequest = ptCpmCcmWarnMsg->wNumberofBroadcastRequest;
    ptWarningMsgSegList->wRepetitionPeriod = ptCpmCcmWarnMsg->wRepetitionPeriod;
    if(1 == ptCpmCcmWarnMsg->m.bitExtendedRepetitionPeriodPresent)
    {
        ptWarningMsgSegList->dwExtendedRepetitionPeriod = ptCpmCcmWarnMsg->dwExtendedRepetitionPeriod;
    }
    else
    {
        ptWarningMsgSegList->dwExtendedRepetitionPeriod = RNLC_INVALID_DWORD;
    }

    for ( WORD32 dwLoop = 0;(dwLoop < dwSegNum) && ( dwLoop < RNLC_CCM_MAX_SEGMENT_NUM);dwLoop ++)
    {
        /* 第一个分片必须填写codingscheme */
        if ( 0 == dwLoop)
        {
            ptWarningMsgSegList->atSib11[dwLoop].m.dataCodingSchemePresent =
            ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent;
            if (1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
            {
                ptWarningMsgSegList->atSib11[dwLoop].dataCodingScheme.numocts = 1;
                ptWarningMsgSegList->atSib11[dwLoop].dataCodingScheme.data[0] =
                ptCpmCcmWarnMsg->tWdataCodingScheme.data[0];
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib11Info dwLoop = %d\n",dwLoop);
        }
        else
        {
            ptWarningMsgSegList->atSib11[dwLoop].m.dataCodingSchemePresent = 0;
        }

        /* 填写messageidentifier和serial number */
        memmove(&ptWarningMsgSegList->atSib11[dwLoop].messageIdentifier,
        &ptCpmCcmWarnMsg->tMessageIdentifier,
        sizeof(SystemInformationBlockType11_messageIdentifier));
        memmove(&ptWarningMsgSegList->atSib11[dwLoop].serialNumber,
        &ptCpmCcmWarnMsg->tSerialNumber,
        sizeof(SystemInformationBlockType11_serialNumber));

        ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegmentNumber = (ASN1INT)dwLoop;
        if (dwLoop == (dwSegNum-1))
        {
            ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegmentType = lastSegment;
            ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegment.numocts = dwLastSegSize;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib11Info lastSegment dwLoop = %d\n",dwLoop);
        }
        else
        {
            ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegmentType = notLastSegment;
            ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegment.numocts = g_dwS1WarnMsgLen;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib11Info notLastSegment dwLoop = %d\n",dwLoop);
        }
        ptWarningMsgSegList->atSib11[dwLoop].warningMessageSegment.data =
        &ptCpmCcmWarnMsg->tWarningMessageContents.data[g_dwS1WarnMsgLen * dwLoop];
    }
    return TRUE;
}



/*<FUNC>***********************************************************************
* 函数名称: CimFillSib12Info
* 功能描述:填充sib11
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimFillSib12Info(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{
    WORD32                   dwSegNum                        = 0;/* 分片数量 */
    WORD32                   dwLastSegSize                   = 0;/* 最后一片大小 */

    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptCmasWarningMsgSegmentList);
    if (0==ptCpmCcmWarnMsg->tWarningMessageContents.numocts)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CMAS Warning Message Contents is 0!\n");
        return FALSE;
    }
    /* 分片数量 */
    dwSegNum = (ptCpmCcmWarnMsg->tWarningMessageContents.numocts + g_dwS1WarnMsgLen - 1) / g_dwS1WarnMsgLen;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB12 SegNum is %d!\n", dwSegNum);
    if (RNLC_CCM_MAX_SEGMENT_NUM<dwSegNum)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 seg num is %d!\n", dwSegNum);
        return FALSE;
    }
    /* 最后一个分片的大小 */
    dwLastSegSize = ptCpmCcmWarnMsg->tWarningMessageContents.numocts%g_dwS1WarnMsgLen;
    if ( (0 == dwLastSegSize) && (0 != ptCpmCcmWarnMsg->tWarningMessageContents.numocts) )
    {
        dwLastSegSize = g_dwS1WarnMsgLen;
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:dwLastSegSize = %u \n",dwLastSegSize);

    /* 将分片信息填写 */
    ptCmasWarningMsgSegmentList->dwSib12SegNum = dwSegNum;
    ptCmasWarningMsgSegmentList->dwSegSize     = g_dwS1WarnMsgLen;
    ptCmasWarningMsgSegmentList->dwLastSegSize = dwLastSegSize;
    ptCmasWarningMsgSegmentList->wNumberofBroadcastRequest = ptCpmCcmWarnMsg->wNumberofBroadcastRequest;
    ptCmasWarningMsgSegmentList->wRepetitionPeriod = ptCpmCcmWarnMsg->wRepetitionPeriod;
    if(1 == ptCpmCcmWarnMsg->m.bitExtendedRepetitionPeriodPresent)
    {
        ptCmasWarningMsgSegmentList->dwExtendedRepetitionPeriod = ptCpmCcmWarnMsg->dwExtendedRepetitionPeriod;
    }
    else
    {
        ptCmasWarningMsgSegmentList->dwExtendedRepetitionPeriod = RNLC_INVALID_DWORD;
    }

    for ( WORD32 dwLoop = 0;(dwLoop < dwSegNum) && ( dwLoop < RNLC_CCM_MAX_SEGMENT_NUM);dwLoop ++)
    {
        /* 第一个分片必须填写codingscheme */
        if ( 0 == dwLoop)
        {
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].m.dataCodingScheme_r9Present =
            ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent;
            if (1 == ptCpmCcmWarnMsg->m.bitDataCodingSchemePresent)
            {
                ptCmasWarningMsgSegmentList->atSib12[dwLoop].dataCodingScheme_r9.numocts = 1;
                ptCmasWarningMsgSegmentList->atSib12[dwLoop].dataCodingScheme_r9.data[0] =
                ptCpmCcmWarnMsg->tWdataCodingScheme.data[0];
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib12Info dwLoop = %d\n",dwLoop);
        }
        else
        {
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].m.dataCodingScheme_r9Present = 0;
        }

        /* 填写messageidentifier和serial number */
        memmove(&ptCmasWarningMsgSegmentList->atSib12[dwLoop].messageIdentifier_r9,
        &ptCpmCcmWarnMsg->tMessageIdentifier,
        sizeof(SystemInformationBlockType12_r9_messageIdentifier_r9));
        memmove(&ptCmasWarningMsgSegmentList->atSib12[dwLoop].serialNumber_r9,
        &ptCpmCcmWarnMsg->tSerialNumber,
        sizeof(SystemInformationBlockType12_r9_serialNumber_r9));

        ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegmentNumber_r9 = (ASN1INT)dwLoop;
        if (dwLoop == (dwSegNum-1))
        {
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegmentType_r9 = lastSegment_1;
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegment_r9.numocts = dwLastSegSize;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib12Info lastSegment dwLoop = %d\n ",dwLoop);
        }
        else
        {
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegmentType_r9 = notLastSegment_1;
            ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegment_r9.numocts = g_dwS1WarnMsgLen;
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimFillSib12Info notLastSegment dwLoop = %d\n",dwLoop);
        }
        ptCmasWarningMsgSegmentList->atSib12[dwLoop].warningMessageSegment_r9.data =
        &ptCpmCcmWarnMsg->tWarningMessageContents.data[g_dwS1WarnMsgLen * dwLoop];
    }
    return TRUE;
}



/*<FUNC>***********************************************************************
* 函数名称: CimEncodeAllSegSi
* 功能描述: 编码分片
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllSegSi(TWarningMsgSegmentList   *ptWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation_r8_IEs   *ptSystemInfoR8 = NULL;
    SystemInformation          tSystemInfo;  /* 系统信息SI编码结构 */
    ASN1CTXT                   tCtxt;         /* 编解码上下文结构 */
    WORD32                     dwSiNumLoop;  /* SI个数遍历 */
    WORD16                     wMsgLen;   /* SI码流长度 */
    WORD16                     wTbSize;   /* SI码流匹配的TB块大小 */
    WORD32                     dwResult = 0;
    WORD16                     wCurETWSLen = 0;
    WORD16                      wAlignmentLen = 0;
    /*这个长度是根据结构计算，是固定的*/
    CCM_NULL_POINTER_CHECK_BOOL(ptWarningMsgSegmentList);
    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /*设置一个标志位，标识是否全部分片的si都编码失败*/
    BOOL bALLSIEncodeFail = TRUE;
    for (dwSiNumLoop = 0; (dwSiNumLoop < ptWarningMsgSegmentList->dwSib11SegNum)
            && (dwSiNumLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwSiNumLoop++ )
    {
        tBcchDlSchMsg.t = 1;
        tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
        tBcchDlSchMsgC1.t = 1;
        tSystemInfo.criticalExtensions.t = 1;
        ptSystemInfoR8 = &(ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwSiNumLoop].systemInformation_r8);
        ptSystemInfoR8->sib_TypeAndInfo.elem[0].t = 10;
        /**/
        ptSystemInfoR8->sib_TypeAndInfo.elem[0].u.sib11 = 
        (SystemInformationBlockType11 *)&ptWarningMsgSegmentList->atSib11[dwSiNumLoop];
        /**/
        //ptSystemInfoR8->sib_TypeAndInfo.elem[0].u.sib11 = 
        //(SystemInformationBlockType11 *)&ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwSiNumLoop].aucMsg[0];
        ptSystemInfoR8->sib_TypeAndInfo.n = 1;
        tSystemInfo.criticalExtensions.u.systemInformation_r8 = ptSystemInfoR8;
        tBcchDlSchMsgC1.u.systemInformation = &tSystemInfo;

        dwResult = CimEncodeFunc(CIM_SI_SI, &tCtxt, (VOID *)&tBcchDlSchMsg);
        if (FALSE == dwResult)
        {
            /* 异常探针上报 */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Encoded. SI-%d which include SIB11 encode fail!\n",dwSiNumLoop);
            continue ;
        }
        else
        {
            /* SI 编码成功处理,首先判断其码流长度是否符合调度限制 */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                /* SI编码后的码流长度超过了CMAC的调度限制 */
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Encoded. SegNum-%d stream len(=%u) beyond TB size boundary!\n",
                dwSiNumLoop + 1, wMsgLen);
                continue;
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB11 Encoded. SegNum-%lu stream len(=%lu) with TB size(=%lu)\n",
                dwSiNumLoop + 1, wMsgLen, wTbSize);

                /* SI编码成功且CMAC可以正常调度，则按照实际的编码保存码流，四字节对齐的问题靠后面计算的长度保证 */
                ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwSiNumLoop].ucValid = TRUE;
                wAlignmentLen = (WORD16)CimEncodeAlignment(wTbSize); 
                /*这里一定要一下内存越界的保护*/
                if((wCurETWSLen + wAlignmentLen) > CIM_SYSINFO_MSG_SIB11_BUF_LEN)
                {
                  /*如果超过了长度，这里直接返回错误*/
                  CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: WARN MSg BUF OVER FLOW!\n");
                  return FALSE;
                }
                /*设计变更*/
                memmove((BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff + sizeof(T_RnlcRnluSystemInfoHead ) + sizeof(T_SystemInfoETWS_up)  + wCurETWSLen ),
                        &(tCtxt.buffer.data[0]), wMsgLen);

                /* 打印出码流信息 */
                if (1 == g_dwCimPrintFlag)
                {
                    CimDbgPrintMsg(&(tCtxt.buffer.data[0]),wMsgLen);
                }  
                
                wCurETWSLen = wCurETWSLen + wAlignmentLen;

                /* SI的长度取为匹配的TB块大小 */
                ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwSiNumLoop].wMsgLen = wTbSize;
                ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len  = wTbSize + ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len;
            }
            bALLSIEncodeFail = FALSE;
        }
    }
    if(TRUE == bALLSIEncodeFail)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 ALL SI Seg Encode Fail!\n");
        return FALSE;
    }
    else
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB11 Msg Length = %d\n",wCurETWSLen);
        ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib11MsgLength = wCurETWSLen;
        return TRUE;
    }
}


/*<FUNC>***********************************************************************
* 函数名称: CimEncodeAllCmasSegSi
* 功能描述: 编码分片
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllCmasSegSi(TCmasWarningMsgSegmentList  *ptCmasWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation_r8_IEs   *ptSystemInfoR8 = NULL;
    SystemInformation          tSystemInfo;  /* 系统信息SI编码结构 */
    ASN1CTXT                   tCtxt;         /* 编解码上下文结构 */
    WORD32                     dwSiNumLoop;  /* SI个数遍历 */
    WORD16                     wMsgLen;   /* SI码流长度 */
    WORD16                     wTbSize;   /* SI码流匹配的TB块大小 */
    WORD32                     dwResult = 0;

    CCM_NULL_POINTER_CHECK_BOOL(ptCmasWarningMsgSegmentList);
   /*设置一个标志位，标识是否全部分片的si都编码失败*/
    BOOL bALLSIEncodeFail = TRUE;
    for (dwSiNumLoop = 0; (dwSiNumLoop < ptCmasWarningMsgSegmentList->dwSib12SegNum)
        && (dwSiNumLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwSiNumLoop++ )
    {
        tBcchDlSchMsg.t = 1;
        tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
        tBcchDlSchMsgC1.t = 1;
        tSystemInfo.criticalExtensions.t = 1;
        ptSystemInfoR8 = &(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].systemInformation_r8);
        ptSystemInfoR8->sib_TypeAndInfo.elem[0].t = 11;
        ptSystemInfoR8->sib_TypeAndInfo.elem[0].u.sib12_v920 = &ptCmasWarningMsgSegmentList->atSib12[dwSiNumLoop];
        ptSystemInfoR8->sib_TypeAndInfo.n = 1;
        tSystemInfo.criticalExtensions.u.systemInformation_r8 = ptSystemInfoR8;
        tBcchDlSchMsgC1.u.systemInformation = &tSystemInfo;

        dwResult = CimEncodeFunc(CIM_SI_SI, &tCtxt, (VOID *)&tBcchDlSchMsg);
        if (RNLC_SUCC == dwResult)
        {
            /* 异常探针上报 */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 Encoded. SI-%d which include SIB12 encode fail!\n",dwSiNumLoop);
        }
        else
        {
            /* SI 编码成功处理,首先判断其码流长度是否符合调度限制 */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                /* SI编码后的码流长度超过了CMAC的调度限制 */
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 Encoded. SegNum-%d stream len(=%u) beyond TB size boundary!\n",
                dwSiNumLoop + 1, wMsgLen);
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 Encoded. SegNum-%lu stream len(=%lu) with TB size(=%lu)\n",
                dwSiNumLoop + 1, wMsgLen, wTbSize);

                /* SI编码成功且CMAC可以正常调度，则保存码流 */
                ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].ucValid = TRUE;
                memcpy(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].aucMsg,
                &(tCtxt.buffer.data[0]), wMsgLen);

                /* SI的长度取为匹配的TB块大小 */
                ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].wMsgLen = wTbSize;
            }
            bALLSIEncodeFail = FALSE;
        }
        }
    if(TRUE == bALLSIEncodeFail)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 ALL SI Seg Encode Fail!\n");
        return FALSE;
    }
    else
    {
         return TRUE;
    }
}

WORD32 CCM_CIM_SIBroadcastComponentFsm::DbgSendToSelfSysInfoRsp(VOID)
{
    T_RnluRnlcSystemInfoRsp tRnluRnlcSystemInfoRsp;
    memset(&tRnluRnlcSystemInfoRsp, 0, sizeof(tRnluRnlcSystemInfoRsp));

    /* 小区ID */
    tRnluRnlcSystemInfoRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnluRnlcSystemInfoRsp.dwResult = 0;

    Message tRnluRnlcSystemInfoRspMsg;
    tRnluRnlcSystemInfoRspMsg.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tRnluRnlcSystemInfoRspMsg.m_dwSignal = EV_RNLU_RNLC_SYSINFO_RSP;
    tRnluRnlcSystemInfoRspMsg.m_wLen = sizeof(tRnluRnlcSystemInfoRsp);
    tRnluRnlcSystemInfoRspMsg.m_pSignalPara = static_cast<void*>(&tRnluRnlcSystemInfoRsp);

    WORD32 dwResult = SendToSelf(&tRnluRnlcSystemInfoRspMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL,
                                " FunctionCallFail_SDF_SendToSelf! ");

        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CCM_CIM_SIBroadcastComponentFsm Send Message %s, MessageID: %X ",
                "EV_RNLU_RNLC_SYSINFO_RSP", EV_RNLU_RNLC_SYSINFO_RSP);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RNLU_RNLC_SYSINFO_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER,
                     sizeof(tRnluRnlcSystemInfoRsp),
                     (const BYTE *)(&tRnluRnlcSystemInfoRsp));

    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimCellDelAddSendOldSIToRnlu(VOID)
* 功能描述: 小区删建时发送一次之前保留在si构建上下文中的旧广播码流
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCellDelAddSendOldSIToRnlu(VOID)
{
    T_CIMVar *ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar =
    (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n  SI: Send to RNLU First Time!. ValueTag=%lu CellId=%d size=%d SysInfoSendCount=%lu\n",
                    ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag,
                    ptCimSIBroadcastVar->wCellId,  ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength,
                    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount);

    if ((ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength > CIM_SYSINFO_MSG_BUF_LEN) ||
        (NULL == ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff))
    {

        CCM_CIM_ExceptionReport(ERR_CCM_CIM_InValidPara,
                                ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength,
                                0,
                                RNLC_ERROR_LEVEL,
                                " ERR_CCM_CIM_InValidPara! ");

        return ERR_CCM_CIM_FunctionCallFail_SendToRnluSIUpt;
    }

    if ((USF_BPLBOARDTYPE_BPL1 == ptCIMVar->wBoardType)||
        (USF_BPLBOARDTYPE_BPL0 == ptCIMVar->wBoardType))
    {
        /*不区分制式全部更新接口头信息中的si upd cause 和 cellinbroad字段*/
        T_RnlcRnluSystemInfoHead *ptSystemInfo = (T_RnlcRnluSystemInfoHead*)ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff;
        ptSystemInfo->wSystemInfoCause = SI_UPD_CELL_SETUP;
        ptSystemInfo->ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;

        WORD32 dwResult = CimSendBroadcastMsgToRnlu();
        if (RNLC_FAIL == dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIBROADCAST_SEND_MSG_TO_RNLU_ADAPT,
                                    dwResult,
                                    ptCimSIBroadcastVar->wCellId,
                                    RNLC_ERROR_LEVEL,
                                    " SIBroadcast: send to RNLU fail! ");

            return ERR_CCM_CIM_SIBROADCAST_SEND_MSG_TO_RNLU_ADAPT;
        }
    }
    else
    {
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_InValidPara,
                            ptCIMVar->wBoardType,
                            ptCIMVar->ucRadioMode,
                            RNLC_FATAL_LEVEL,
                            " ERR_CCM_CIM_InValidPara! ");

        return ERR_CCM_CIM_InValidPara;
    }
    return RNLC_SUCC;
}

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimPowerOnInSlave()
{
    return RNLC_SUCC;
}

VOID CCM_CIM_SIBroadcastComponentFsm::SetComponentSlave()
{
    TranStateWithTag(CCM_CIM_SIBroadcastComponentFsm, Handle_InSlaveState,CCM_ALLCOMPONET_SLAVE_STATE);
}

VOID CCM_CIM_SIBroadcastComponentFsm::Handle_InSlaveState(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg)
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_SYNC_COMPONENT:
        {
            /*将主板构建的状态保存起来*/
            TCimComponentInfo     *ptCimComponentInfo = \
                               (TCimComponentInfo *)pMsg->m_pSignalPara;

            ucMasterSateCpy = ptCimComponentInfo->ucState;
            CCM_CIM_LOG(DEBUG_LEVEL,"\n SI: CCM_CIM_SIBroadcastComponentFsm get State %d.\n",ucMasterSateCpy);
            /* 获取CIM广播信息,将广播发送的valuetag保存到备板的广播的上下文中 */
            T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
            ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag = (BYTE)ptCimComponentInfo->padding[0];
            break;
        }

        case CMD_CIM_SLAVE_TO_MASTER:
        {
            CCM_CIM_LOG(DEBUG_LEVEL,"\n SI: CCM_CIM_SIBroadcastComponentFsm SLAVE_TO_MASTER.\n");
            HandleSlaveToMaster(pMsg);
            break;
        }
        default:
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "CCM_CIM_SIBroadcastComponentFsm Handle_InSlaveState Error Event.\n");
            break;
        }
    }
    return ;
}

/*<FUNC>***********************************************************************
* 函数名称: HandleMasterToSlave
* 功能描述: 主转备，构件进入slave态
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleMasterToSlave(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_LOG(DEBUG_LEVEL,"\n SI: CCM_CIM_SIBroadcastComponentFsm HandleMasterToSlave.\n");
    SetComponentSlave();
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: HandleSlaveToMaster
* 功能描述: 备转主时的操作，重新读取数据库生成广播信息
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleSlaveToMaster(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);

        /*先将SI 构建状态转到IDLE*/
        TranStateWithTag(CCM_CIM_SIBroadcastComponentFsm, Idle,S_CIM_SIB_IDLE);
    
    Message tCCMasterSlave;
    T_SysInfoUpdReq tSysinfoUpdReq;

    /* 获取广播构件数据区 */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    tSysinfoUpdReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tCCMasterSlave.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tCCMasterSlave.m_wLen = sizeof(tSysinfoUpdReq);
    tCCMasterSlave.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tCCMasterSlave.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;
    /*暂时没有考虑将告警的状态由主板同步到备板*/
    Handle_SystemInfoUpdateReq(&tCCMasterSlave);    
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimRecordToRNLUDbgInfo
* 功能描述: 备转主时的操作，重新读取数据库生成广播信息
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimRecordToRNLUDbgInfo(void)
{
    WORD32 dwIndex = 0;
    WORD32 dwTemIndex = 0;
    T_CIMVar *ptCIMVar                       = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    
    /*找到发送码流的头信息*/
    T_RnlcRnluSystemInfoHead *ptSystemInfo   = (T_RnlcRnluSystemInfoHead*)ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff;
    /*如果是小区建立原因，并且不是小区删减引起的建立，则将这次保存在数组的第一个元素中*/
    if((CAUSE_CELLSETUP == ptSystemInfo->wSystemInfoCause)&&
       (SI_UPD_CELL_DELADD != ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {    
        ptCIMVar->tRNLUSiInfoRecord[0].ucCellInBroad   = ptSystemInfo->ucCellIdInBoard;
        ptCIMVar->tRNLUSiInfoRecord[0].wCellId         = ptSystemInfo->wCellId;
        ptCIMVar->tRNLUSiInfoRecord[0].wRNLUSIUpdCause = ptSystemInfo->wSystemInfoCause;
        ptCIMVar->tRNLUSiInfoRecord[0].ucUsedFlag      = 1;
    }
    else
    {
        /*非小区建立的原因，则依次存储最近四次的记录*/
        for(dwIndex = 1; dwIndex < SI_TO_RNLU_INFO_NUM; dwIndex++)
        {
            if(0 == ptCIMVar->tRNLUSiInfoRecord[dwIndex].ucUsedFlag)
            {
               break;
            }
            
        }
        CCM_LOG(RNLC_INFO_LEVEL, "\n dw Index = %d\n",dwIndex);
        if(SI_TO_RNLU_INFO_NUM == dwIndex)
        {      
            /*如果记录已经填满，则1.前移最后一个记录之前的记录*/
            for(dwTemIndex = 1; dwTemIndex < (SI_TO_RNLU_INFO_NUM - 1); dwTemIndex++)
            {
                CCM_LOG(RNLC_INFO_LEVEL,"\n move %d to %d\n",dwTemIndex + 1,dwTemIndex);
                if((dwTemIndex + 1) < SI_TO_RNLU_INFO_NUM)
                {
                    ptCIMVar->tRNLUSiInfoRecord[dwTemIndex].ucCellInBroad   = ptCIMVar->tRNLUSiInfoRecord[dwTemIndex + 1].ucCellInBroad;
                    ptCIMVar->tRNLUSiInfoRecord[dwTemIndex].wCellId         = ptCIMVar->tRNLUSiInfoRecord[dwTemIndex + 1].wCellId;
                    ptCIMVar->tRNLUSiInfoRecord[dwTemIndex].wRNLUSIUpdCause = ptCIMVar->tRNLUSiInfoRecord[dwTemIndex + 1].wRNLUSIUpdCause;
                    ptCIMVar->tRNLUSiInfoRecord[dwTemIndex].ucUsedFlag      = ptCIMVar->tRNLUSiInfoRecord[dwTemIndex + 1].ucUsedFlag;
                }
            }
           
            /*最后一个记录，记录为本次的记录*/
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].ucCellInBroad   = ptSystemInfo->ucCellIdInBoard;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].wCellId         = ptSystemInfo->wCellId;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].wRNLUSIUpdCause = ptSystemInfo->wSystemInfoCause;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].ucUsedFlag      = 1;      
        }
        else     
        {
            /*如果记录还没有满，则在最后的一个空白的记录填写本次记录*/
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].ucCellInBroad   = ptSystemInfo->ucCellIdInBoard;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].wCellId         = ptSystemInfo->wCellId;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].wRNLUSIUpdCause = ptSystemInfo->wSystemInfoCause;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].ucUsedFlag      = 1;
        }
    }

    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimGetAllSystemInfo
* 功能描述: a.若MIB/SIB1/SIB2的获取失败，停止广播发送流程，调度信息在SIB1中
*           b.读取调度信息成功，但是不包括SIB2，停止广播发送
*           c.依据调度信息读取SIB3~SIb9相关的数据表，
*             若失败更新调度信息，不下发对应的SIB，打印告警信息，广播信息正常发送
*           d.调整SIB1下的调度列表:若数据库中调度信息下的所有SIB由于获取信息失败，
*             需要将调度列表下的此SI删除
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetAllSystemInfo(WORD16 wCellId, BYTE ucRadioMode, BYTE ucLastValueTag, T_AllSibList *ptAllSibList)
{
    CCM_NULL_POINTER_CHECK(ptAllSibList);

    BYTE   aucSibList[DBS_SI_SPARE] = {0};
    BYTE   ucResult = FALSE;
    WORD32 dwResult = FALSE;
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId, "\n SI: Fill Normal SIB Info now...!\n");
    SchedulingInfoList tSchedulingInfoList;
    memset((VOID*)&tSchedulingInfoList, 0, sizeof(tSchedulingInfoList));

    {
        T_DBS_GetMibInfo_REQ  tGetMibInfoReq;
        T_DBS_GetMibInfo_ACK  tGetMibInfoAck;

        memset((VOID*)&tGetMibInfoReq,  0, sizeof(tGetMibInfoReq));
        memset((VOID*)&tGetMibInfoAck,  0, sizeof(tGetMibInfoAck));

        /* 1.获取MIB信息 */
        tGetMibInfoReq.wCallType = USF_MSG_CALL;
        tGetMibInfoReq.wCellId   = wCellId;
        ucResult = UsfDbsAccess(EV_DBS_GetMibInfo_REQ, (VOID *)&tGetMibInfoReq, (VOID *)&tGetMibInfoAck);
        if ((FALSE == ucResult) || (0 != tGetMibInfoAck.dwResult))
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get MIB Info fail! CellId=%d\n",wCellId);
            return FALSE;
        }        
        CimFillMibInfo(&tGetMibInfoAck, ptAllSibList);
    }

    {
        T_DBS_GetSib1Info_REQ tGetSib1InfoReq;
        T_DBS_GetSib1Info_ACK tGetSib1InfoAck;

        memset((VOID*)&tGetSib1InfoReq, 0, sizeof(tGetSib1InfoReq));
        memset((VOID*)&tGetSib1InfoAck, 0, sizeof(tGetSib1InfoAck));

        /* 2.获取SIB1信息 */
        tGetSib1InfoReq.wCallType = USF_MSG_CALL;
        tGetSib1InfoReq.wCellId   = wCellId;
        ucResult = UsfDbsAccess(EV_DBS_GetSib1Info_REQ, (VOID *)&tGetSib1InfoReq, (VOID *)&tGetSib1InfoAck);
        if ((FALSE == ucResult) || (0 != tGetSib1InfoAck.dwResult))
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB1 Info fail! CellId=%d\n",wCellId);
            return FALSE;
        }
        /* 修改SISCH信息 */
        CimDbgConfigSISch(&tGetSib1InfoAck);

        /* 3.组装调度信息 */
        dwResult = CimGetSIScheInfo(&tGetSib1InfoAck, aucSibList, &tSchedulingInfoList);
        if (FALSE == dwResult)
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: CIM Get SISCHE Info fail!\n");
            return FALSE;
        }

        /* 如果没有Sib2，则不下发广播 */
        if (FALSE == aucSibList[DBS_SI_SIB2])
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SISCHE Info No Sib2! CellId=%d\n",wCellId);
            return FALSE;
        }

        CimFillSib1Info(ucRadioMode,ucLastValueTag,&tGetSib1InfoAck, ptAllSibList);
    }


    {
        T_DBS_GetSib2Info_REQ tGetSib2InfoReq;
        T_DBS_GetSib2Info_ACK tGetSib2InfoAck;

        memset((VOID*)&tGetSib2InfoReq, 0, sizeof(tGetSib2InfoReq));
        memset((VOID*)&tGetSib2InfoAck, 0, sizeof(tGetSib2InfoAck));

        /* 因为setup是一个指针，所以这块需要重新指定具体的指向，否则为空指针 */
        ptAllSibList->tSib2Info.radioResourceConfigCommon.soundingRS_UL_ConfigCommon.u.setup =
            &(ptAllSibList->tSibPointStruct.tSrsUlCfgSetUp);

        /* 4.获取SIB2信息 */
        tGetSib2InfoReq.wCallType = USF_MSG_CALL;
        tGetSib2InfoReq.wCellId   = wCellId;
        ucResult = UsfDbsAccess(EV_DBS_GetSib2Info_REQ, (VOID *)&tGetSib2InfoReq, (VOID *)&tGetSib2InfoAck);
        if ((FALSE == ucResult) || (0 != tGetSib2InfoAck.dwResult))
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB2 Info Fail! CellId=%d\n",wCellId);
            return FALSE;
        }

        g_wLogRtSeqStNum = tGetSib2InfoAck.tSib2.tRadioResourceConfigCommon.tPrach_Config.wLogRtSeqStNum;

        CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI: Dbs wLogRtSeqStNum: %d \n ",g_wLogRtSeqStNum);  
        
        CimFillSib2Info(ucRadioMode,&tGetSib2InfoAck, ptAllSibList,wCellId);
    }

    {
        T_DBS_GetSib3Info_REQ tGetSib3InfoReq;
        T_DBS_GetSib3Info_ACK tGetSib3InfoAck;

        memset((VOID*)&tGetSib3InfoReq, 0, sizeof(tGetSib3InfoReq));
        memset((VOID*)&tGetSib3InfoAck, 0, sizeof(tGetSib3InfoAck));

        /* 5.获取SIB3信息 */
        if (TRUE == aucSibList[DBS_SI_SIB3])
        {
            tGetSib3InfoReq.wCallType = USF_MSG_CALL;
            tGetSib3InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib3Info_REQ, (VOID *)&tGetSib3InfoReq, (VOID *)&tGetSib3InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib3InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB3 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType3);
            }
            CimFillSib3Info(wCellId, ucRadioMode, &tGetSib3InfoAck, ptAllSibList);
        }
    }


    {
        T_DBS_GetSib4Info_REQ tGetSib4InfoReq;
        T_DBS_GetSib4Info_ACK tGetSib4InfoAck;

        memset((VOID*)&tGetSib4InfoReq, 0, sizeof(tGetSib4InfoReq));
        memset((VOID*)&tGetSib4InfoAck, 0, sizeof(tGetSib4InfoAck));

        /* 6.获取SIB4信息 *//*lint -save -e690 */
        if (TRUE == aucSibList[DBS_SI_SIB4])
        {
            tGetSib4InfoReq.wCallType = USF_MSG_CALL;
            tGetSib4InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib4Info_REQ, (VOID *)&tGetSib4InfoReq, (VOID *)&tGetSib4InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib4InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB4 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType4);
            }

            dwResult = CimFillSib4Info(wCellId, ucRadioMode, &tGetSib4InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType4);
            }
        }
    /*lint -restore*/
    }


    {
        T_DBS_GetSib5Info_REQ tGetSib5InfoReq;
        T_DBS_GetSib5Info_ACK tGetSib5InfoAck;

        memset((VOID*)&tGetSib5InfoReq, 0, sizeof(tGetSib5InfoReq));
        memset((VOID*)&tGetSib5InfoAck, 0, sizeof(tGetSib5InfoAck));

        /* 7.获取SIB5信息 */
        if (TRUE == aucSibList[DBS_SI_SIB5])
        {
            tGetSib5InfoReq.wCallType = USF_MSG_CALL;
            tGetSib5InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib5Info_REQ, (VOID *)&tGetSib5InfoReq, (VOID *)&tGetSib5InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib5InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB5 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType5);
            }

            dwResult = CimFillSib5Info(wCellId, ucRadioMode, &tGetSib5InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType5);
            }
        }
    }

    {
        T_DBS_GetSib6Info_REQ tGetSib6InfoReq;
        T_DBS_GetSib6Info_ACK tGetSib6InfoAck;

        memset((VOID*)&tGetSib6InfoReq, 0, sizeof(tGetSib6InfoReq));
        memset((VOID*)&tGetSib6InfoAck, 0, sizeof(tGetSib6InfoAck));

        /* 8.获取SIB6信息 */
        if (TRUE == aucSibList[DBS_SI_SIB6])
        {
            tGetSib6InfoReq.wCallType = USF_MSG_CALL;
            tGetSib6InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib6Info_REQ, (VOID *)&tGetSib6InfoReq, (VOID *)&tGetSib6InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib6InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB6 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType6);
            }

            CimFillSib6Info(wCellId,ucRadioMode, &tGetSib6InfoAck, ptAllSibList);
        }
    }

    {
        T_DBS_GetSib7Info_REQ tGetSib7InfoReq;
        T_DBS_GetSib7Info_ACK tGetSib7InfoAck;

        memset((VOID*)&tGetSib7InfoReq, 0, sizeof(tGetSib7InfoReq));
        memset((VOID*)&tGetSib7InfoAck, 0, sizeof(tGetSib7InfoAck));

        /* 9.获取SIB7信息 */
        if (TRUE == aucSibList[DBS_SI_SIB7])
        {
            tGetSib7InfoReq.wCallType = USF_MSG_CALL;
            tGetSib7InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib7Info_REQ, (VOID *)&tGetSib7InfoReq, (VOID *)&tGetSib7InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib7InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB7 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType7);
            }

            dwResult = CimFillSib7Info(wCellId, &tGetSib7InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType7);
            }
        }
    }

    {
        T_DBS_GetSib8Info_REQ tGetSib8InfoReq;
        T_DBS_GetSib8Info_ACK tGetSib8InfoAck;

        memset((VOID*)&tGetSib8InfoReq, 0, sizeof(tGetSib8InfoReq));
        memset((VOID*)&tGetSib8InfoAck, 0, sizeof(tGetSib8InfoAck));

        /* 10.获取SIB8信息 */
        if (TRUE == aucSibList[DBS_SI_SIB8])
        {
            tGetSib8InfoReq.wCallType = USF_MSG_CALL;
            tGetSib8InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib8Info_REQ, (VOID *)&tGetSib8InfoReq, (VOID *)&tGetSib8InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib8InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB8 Info Fail! CellId=%d\n",wCellId);
                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType8);
            }

            CimFillSib8Info(wCellId, &tGetSib8InfoAck, ptAllSibList);
        }
    }

    {
        T_DBS_GetSib9Info_REQ tGetSib9InfoReq;
        T_DBS_GetSib9Info_ACK tGetSib9InfoAck;

        memset((VOID*)&tGetSib9InfoReq, 0, sizeof(tGetSib9InfoReq));
        memset((VOID*)&tGetSib9InfoAck, 0, sizeof(tGetSib9InfoAck));

        /* 11.获取SIB9信息 */
        if (TRUE == aucSibList[DBS_SI_SIB9])
        {
            tGetSib9InfoReq.wCallType = USF_MSG_CALL;
            tGetSib9InfoReq.wCellId   = wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetSib9Info_REQ, (VOID *)&tGetSib9InfoReq, (VOID *)&tGetSib9InfoAck);
            if ((FALSE == ucResult) || (0 != tGetSib9InfoAck.dwResult))
            {
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        wCellId, 
                        RNLC_INVALID_DWORD,
                        RNLC_ERROR_LEVEL, 
                        "\n SIBroadcast: CIM Get SIB9 Info Fail! CellId=%d\n",wCellId);

                /* 调整调度列表 */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType9);
            }

            CimFillSib9Info(&tGetSib9InfoAck, ptAllSibList);
        }
    }

    /* 更新调度信息 */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    memcpy(ptAllSibList->aucSibList, aucSibList, sizeof(aucSibList));
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId, "\n SI: Fill Normal SIB Info Finish!\n");
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimEncodeAllSibs_Warning
* 功能描述: a.若编码MIB/SIB1/SIB2失败，则停止广播下发
*           b.若编码SIB3~SIB9时，若出现某个SIB编码失败，则调整调度列表，不下发该SIB
*           c.调整SIB1下的调度列表:若数据库中调度信息下的所有SIB由于编码失败，
*             需要将调度列表下的此SI删除
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: WORD32
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeAllSibs_Warning(WORD16 wCellId, T_AllSibList *ptAllSibList,
                                T_BroadcastStream *ptNewBroadcastStream,
                                TWarningMsgSegmentList *ptWarningMsgSegmentList,
                                TCmasWarningMsgSegmentList  *ptCmasWarningMsgSegmentList,
                                T_CIMVar *ptCimData,
                                T_CimSIBroadcastVar *ptCimSIBroadcastVar)
{
    WORD32    dwResult   = FALSE;
    WORD16    wAllSibLen = 0;
    WORD16    wTempLen   = 0;
    WORD32    dwReturn = RNLC_INVALID_DWORD;
    ASN1CTXT  tCtxt;
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SchedulingInfoList tSchedulingInfoList;

    memset(&tCtxt, 0, sizeof(tCtxt));
    memset(&tBcchDlSchMsg, 0, sizeof(BCCH_DL_SCH_MessageType));
    memset(&tBcchDlSchMsgC1, 0, sizeof(BCCH_DL_SCH_MessageType_c1));
    memset((VOID*)&tSchedulingInfoList, 0, sizeof(tSchedulingInfoList));

    /* MIB编码 */
    dwResult = CimEncodeFunc(CIM_SI_MIB, &tCtxt, (VOID *)(&ptAllSibList->tMibInfo));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucMibLen = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucMibBuf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucMibLen);

        if (CIM_MIB_BUF_LEN < ptNewBroadcastStream->ucMibLen)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: MIB_LEN=%d beyond RNLC_BUF_LEN_4_MIB.\n",ptNewBroadcastStream->ucMibLen);      
            dwReturn = 0;
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: ENCODE_MIB fail and BcmiEncodeAllSibs return false.\n"); 
        dwReturn = 0;
        return dwReturn;
    }

    /* SIB1编码 */
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);

        /* ucSib1ValueTagOffset表示ValueTag从SIB1码流最后一个字节到ValueTag起始位置的偏移值，
        8表示每个字节的bit数，5表示ValueTag所占的bit位数 */
        ptNewBroadcastStream->ucSib1ValueTagOffset = (BYTE)((8 - tCtxt.buffer.bitOffset) + 5);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);
        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: SIB1_LEN=%d beyond RNLC_BUF_LEN_30_SIB1.\n", ptNewBroadcastStream->ucSib1Len); 
            dwReturn = 1;
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: ENCODE_SIB1 fail and BcmiEncodeAllSibs return false.\n"); 
        dwReturn = 1;
        return dwReturn;
    }

    /* SIB2编码 */
    dwResult = CimEncodeFunc(CIM_SI_SIB2, &tCtxt, (VOID *)(&ptAllSibList->tSib2Info));
    if (TRUE == dwResult)
    {
        wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
        ptNewBroadcastStream->ucSibNum++;
        memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
        wAllSibLen += sizeof(wTempLen);
        memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
        wAllSibLen += wTempLen;

        if (CIM_SIB2_BUF_LEN< wTempLen)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB2_LEN=%d beyond RNLC_BUF_LEN_64_SIB2.\n", wTempLen); 
            dwReturn = 2;
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: ENCODE_SIB2 fail and BcmiEncodeAllSibs return false.\n");
        dwReturn = 2;
        return dwReturn;
    }

    tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;

    /* 其它SIB编码 */
    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB3])
    {
        /* SIB3编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB3, &tCtxt, (VOID *)(&ptAllSibList->tSib3Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB3_LEN=%d over CIM_MAX_STREAM_LEN.\n", wTempLen);
                dwReturn = 3;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: NCODE_SIB3 fail.\n");
            /* 调整调度列表 */
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType3);
            dwReturn = 3;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB4])
    {
        /* SIB4编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB4, &tCtxt, (VOID *)(&ptAllSibList->tSib4Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB4_LEN=%d over CIM_MAX_STREAM_LEN.\n", wTempLen);
                dwReturn = 4;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB4 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType4);
            dwReturn = 4;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB5])
    {
        /* SIB5编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB5, &tCtxt, (VOID *)(&ptAllSibList->tSib5Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB5_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 5;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB5 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType5);
            dwReturn = 5;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB6])
    {
        /* SIB6编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB6, &tCtxt, (VOID *)(&ptAllSibList->tSib6Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB6_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 6;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB6 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType6);
            dwReturn = 6;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB7])
    {
        /* SIB7编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB7, &tCtxt, (VOID *)(&ptAllSibList->tSib7Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB7_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 7;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB7 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType7);
            dwReturn = 7;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB8])
    {
        /* SIB8编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB8, &tCtxt, (VOID *)(&ptAllSibList->tSib8Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB8_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 8;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB8 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType8);
            dwReturn = 8;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB9])
    {
        /* SIB9编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB9, &tCtxt, (VOID *)(&ptAllSibList->tSib9Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB9_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 9;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: NCODE_SIB9 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType9);
            dwReturn = 9;
        }
    }
    /*当s1告警包含sib10需要发送和sib10正在发送时的普通广播更新时才处理*/
    if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)||
        (1 == ptCimSIBroadcastVar->tSIBroadcastDate.ucNormalSibWithSib10Info))
    {
        if (TRUE ==  ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /* SIB10编码 */
            dwResult = CimEncodeFunc(CIM_SI_SIB10, &tCtxt, (VOID *)(&ptAllSibList->tSib10Info));
            if (TRUE == dwResult)
            {
                wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
                ptNewBroadcastStream->ucSibNum++;
                memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
                wAllSibLen += sizeof(wTempLen);
                memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
                wAllSibLen += wTempLen;

                if (CIM_SIB10_BUF_LEN < wTempLen)
                {
                    /* 长度越界 */
                    CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB10_LEN=%d over BCM_SIB10_BUF_LEN.\n",wTempLen);
                    dwReturn = 10;
                }
                else
                {
                    /* 将Sib10码流保存在实例数据区 */
                    /*lint -save -e669 */
                    memcpy(&ptCimData->tBcmiEtwsPrimaryInfo.abySib10Stream, &(tCtxt.buffer.data[0]), wTempLen);
                    ptCimData->tBcmiEtwsPrimaryInfo.dwSib10Size = wTempLen;
                    /*lint -save -e669 */
                }
            }
            else
            {
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI:Sib10 encode fail!\n");
                CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType10);
                dwReturn = 10;
            }
        }
    }
    else
    {
        /*如果sib10没有关系，为了保证不出问题，这里做一次在sib1中去除其调度信息*/
        CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType10);
    }


    /******************************SIB11 start***********************************************/

    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)
    {
        /*去掉原来对整个sib11 和sib12 编码的过程，并且两个sib的码流不再放在ptNewBroadcastStream中
        只保存到cim的实例区中*/
        /* 编码SIB11 分片 */
        dwResult = CimEnCodeSib11SegList(ptWarningMsgSegmentList, ptCimData,ptCimSIBroadcastVar);
        if (FALSE == dwResult)
        {
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType11);
        }

    }

    /******************************SIB11 end*************************************************/
    if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)
    {
        /******************************SIB12 start***********************************************/
        /* 编码SIB12 分片 */
        dwResult = CimEnCodeSib12SegList(ptCmasWarningMsgSegmentList, ptCimData,ptCimSIBroadcastVar);
        if (FALSE == dwResult)
        {
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
        }
        /******************************SIB12 end*************************************************/
    }
    /* 更新调度信息 */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);

    return dwReturn;
}

/*<FUNC>***********************************************************************
* 函数名称: CimEncodeAllSibs
* 功能描述: a.若编码MIB/SIB1/SIB2失败，则停止广播下发
*           b.若编码SIB3~SIB9时，若出现某个SIB编码失败，则调整调度列表，不下发该SIB
*           c.调整SIB1下的调度列表:若数据库中调度信息下的所有SIB由于编码失败，
*             需要将调度列表下的此SI删除
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: WORD32
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeAllSibs(WORD16 wCellId, T_AllSibList *ptAllSibList,
                        T_BroadcastStream *ptNewBroadcastStream)
{
    WORD32    dwResult   = FALSE;
    WORD16    wAllSibLen = 0;
    WORD16    wTempLen   = 0;
    WORD32    dwReturn = RNLC_INVALID_DWORD;

    ASN1CTXT  tCtxt;
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SchedulingInfoList tSchedulingInfoList;

    memset(&tCtxt, 0, sizeof(tCtxt));
    memset(&tBcchDlSchMsg, 0, sizeof(BCCH_DL_SCH_MessageType));
    memset(&tBcchDlSchMsgC1, 0, sizeof(BCCH_DL_SCH_MessageType_c1));
    memset((VOID*)&tSchedulingInfoList, 0, sizeof(tSchedulingInfoList));

    /* MIB编码 */
    dwResult = CimEncodeFunc(CIM_SI_MIB, &tCtxt, (VOID *)(&ptAllSibList->tMibInfo));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucMibLen = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucMibBuf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucMibLen);

        if (CIM_MIB_BUF_LEN < ptNewBroadcastStream->ucMibLen)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: MIB_LEN=%d beyond RNLC_BUF_LEN_4_MIB.\n",ptNewBroadcastStream->ucMibLen);
            dwReturn = 0; 
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: ENCODE_MIB fail and BcmiEncodeAllSibs return false.\n");
        dwReturn = 0; 
        return dwReturn;
    }

    /* SIB1编码 */
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);

        /* ucSib1ValueTagOffset表示ValueTag从SIB1码流最后一个字节到ValueTag起始位置的偏移值，
           8表示每个字节的bit数，5表示ValueTag所占的bit位数 */
        ptNewBroadcastStream->ucSib1ValueTagOffset = (BYTE)((8 - tCtxt.buffer.bitOffset) + 5);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);

        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB1_LEN=%d beyond RNLC_BUF_LEN_30_SIB1.\n",ptNewBroadcastStream->ucSib1Len);
            dwReturn = 1; 
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: ENCODE_SIB1 fail and BcmiEncodeAllSibs return false.\n");
        dwReturn = 1; 
        return dwReturn;
    }

    /* SIB2编码 */
    dwResult = CimEncodeFunc(CIM_SI_SIB2, &tCtxt, (VOID *)(&ptAllSibList->tSib2Info));
    if (TRUE == dwResult)
    {
        wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
        ptNewBroadcastStream->ucSibNum++;
        memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
        wAllSibLen += sizeof(wTempLen);
        memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
        wAllSibLen += wTempLen;

        if (CIM_SIB2_BUF_LEN< wTempLen)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB2_LEN=%d beyond RNLC_BUF_LEN_64_SIB2.\n",wTempLen);
            dwReturn = 2; 
            return dwReturn;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: ENCODE_SIB2 fail and BcmiEncodeAllSibs return false.\n");
        dwReturn = 2; 
        return dwReturn;
    }

    tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;

    /* 其它SIB编码 */
    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB3])
    {
        /* SIB3编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB3, &tCtxt, (VOID *)(&ptAllSibList->tSib3Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB3_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 3; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB3 fail.\n");

            /* 调整调度列表 */
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType3);
            dwReturn = 3; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB4])
    {
        /* SIB4编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB4, &tCtxt, (VOID *)(&ptAllSibList->tSib4Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB4_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 4; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB4 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType4);
            dwReturn = 4; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB5])
    {
        /* SIB5编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB5, &tCtxt, (VOID *)(&ptAllSibList->tSib5Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB5_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 5; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB5 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType5);
            dwReturn = 5; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB6])
    {
        /* SIB6编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB6, &tCtxt, (VOID *)(&ptAllSibList->tSib6Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB6_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 6; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB6 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType6);
            dwReturn = 6; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB7])
    {
        /* SIB7编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB7, &tCtxt, (VOID *)(&ptAllSibList->tSib7Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB7_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 7; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB7 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType7);
            dwReturn = 7; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB8])
    {
        /* SIB8编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB8, &tCtxt, (VOID *)(&ptAllSibList->tSib8Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB8_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 8; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB8 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType8);
            dwReturn = 8; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB9])
    {
        /* SIB9编码 */
        dwResult = CimEncodeFunc(CIM_SI_SIB9, &tCtxt, (VOID *)(&ptAllSibList->tSib9Info));
        if (TRUE == dwResult)
        {
            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            ptNewBroadcastStream->ucSibNum++;
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &wTempLen, sizeof(wTempLen));
            wAllSibLen += sizeof(wTempLen);
            memcpy(ptNewBroadcastStream->aucAllSibBuf + wAllSibLen, &(tCtxt.buffer.data[0]), wTempLen);
            wAllSibLen += wTempLen;

            if (CIM_MAX_STREAM_LEN < wTempLen)
            {
                /* 长度越界 */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB9_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 9; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB9 fail.\n");
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType9);
            dwReturn = 9; 
        }
    }

    /* 更新调度信息 */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);

    return dwReturn;
}
/*<FUNC>***********************************************************************
* 函数名称: CimGetSIScheInfo
* 功能描述: 读取数据库表R_SISCHE获取配置的调度信息，将各SIB映射到对应的SI中
*           此为最初始的调度信息，后续可能调整
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSIScheInfo(T_DBS_GetSib1Info_ACK *ptGetSib1InfoAck, BYTE *pucSibsList,
                        SchedulingInfoList *ptSchedulingInfoList)
{

    WORD32           dwRcdLoop        = 0;
    WORD32           dwSiLoop        = 0;
    WORD32           dwSiNo          = 0;
    SIB_MappingInfo  *ptSibMapInfo   = NULL;

    /* 遍历数据库中的SI配置记录，填写调度信息列表 */
    ptSchedulingInfoList->n = ptGetSib1InfoAck->tSib1.ucSIScheNum;

    /* 不含SIB2的SI，从调度列表中的第2个SI开始填写 */
    dwSiLoop = 1;
    for (dwRcdLoop = 0; dwRcdLoop < ptGetSib1InfoAck->tSib1.ucSIScheNum; dwRcdLoop++)
    {
        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib2)
        {
            /* SIB2配置,即判断是否是第一个SI */
            dwSiNo = 0;
            pucSibsList[DBS_SI_SIB2] = TRUE;
        }
        else
        {
            dwSiNo = dwSiLoop;
            dwSiLoop++;
        }

        ptSchedulingInfoList->elem[dwSiNo].si_Periodicity
        = (SchedulingInfo_si_Periodicity)ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucPeriodicity;

        /* 初始化SIB映射信息列表含有的元素个数为0 */
        ptSibMapInfo = &(ptSchedulingInfoList->elem[dwSiNo].sib_MappingInfo);
        ptSibMapInfo->n = 0;

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib3)
        {
            /* SIB3配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType3;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB3] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib4)
        {
            /* SIB4配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType4;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB4] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib5)
        {
            /* SIB5配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType5;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB5] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib6)
        {
            /* SIB6配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType6;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB6] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib7)
        {
            /* SIB7配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType7;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB7] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib8)
        {
            /* SIB8配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType8;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB8] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib9)
        {
            /* SIB9配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType9;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB9] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib10)
        {
            /* SIB10配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType10;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB10] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib11)
        {
            /* SIB11配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType11;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB11] = TRUE;
        }
    if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib12)
        {
            /* SIB12配置 */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType12_v920;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB12] = TRUE;
        }    
    }

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimAdjustScheInfoList
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimAdjustScheInfoList(SchedulingInfoList *ptSchedulingInfoList, BYTE *pucSibsList, SIB_Type eSibType)
{
    WORD32 dwSiLoop   = 0;  /* SI遍历 */
    WORD32 dwSibLoop  = 0;  /* 每个SI中的SIB遍历  */
    WORD32 dwSibCount = 0;  /* 每个SI中的SIB数目  */
    WORD32 dwTempLoop = 0;

    for (dwSiLoop = 0; (dwSiLoop < ptSchedulingInfoList->n) && (dwSiLoop < 32); dwSiLoop++)
    {
        /* 每个SI中的SIB总数 */
        dwSibCount = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n;

        for (dwSibLoop = 0; (dwSibLoop < dwSibCount) && ( dwSibLoop < 31 ); dwSibLoop++)
        {
            if (eSibType == ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop])
            {
                /* 将后面的SIB进行移位 */
                for (dwTempLoop = dwSibLoop + 1; dwTempLoop < dwSibCount; dwTempLoop++)
                {
                    ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop - 1]
                    = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop];
                }

                /* 置无效位 */
                ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop - 1] = 0XFF;
                (ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n)--;
            }
        }
    }

    /* 在邻时SIB列表中删除此SIB */
    switch (eSibType)
    {
    case sibType3:
    {
        pucSibsList[DBS_SI_SIB3] = FALSE;
        break;
    }
    case sibType4:
    {
        pucSibsList[DBS_SI_SIB4] = FALSE;
        break;
    }
    case sibType5:
    {
        pucSibsList[DBS_SI_SIB5] = FALSE;
        break;
    }
    case sibType6:
    {
        pucSibsList[DBS_SI_SIB6] = FALSE;
        break;
    }
    case sibType7:
    {
        pucSibsList[DBS_SI_SIB7] = FALSE;
        break;
    }
    case sibType8:
    {
        pucSibsList[DBS_SI_SIB8] = FALSE;
        break;
    }
    case sibType9:
    {
        pucSibsList[DBS_SI_SIB9] = FALSE;
        break;
    }
    case sibType10:
    {
        pucSibsList[DBS_SI_SIB10] = FALSE;
        break;
    }
    case sibType11:
    {
        pucSibsList[DBS_SI_SIB11] = FALSE;
        break;
    }
    case sibType12_v920:
    {
        pucSibsList[DBS_SI_SIB12] = FALSE;        
        break;
    }
    default :
    {
        break;
    }
    }
}

/*<FUNC>***********************************************************************
* 函数名称: CimGetSchePeriodInfo
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSchePeriodInfo(SchedulingInfoList *ptSchedulingInfoList,  BYTE *pucSibsList,SIB_Type eSibType)
{    

    CCM_NULL_POINTER_CHECK(pucSibsList);
    WORD32 dwSiLoop   = 0;  /* SI遍历 */
    WORD32 dwSibLoop  = 0;  /* 每个SI中的SIB遍历  */
    WORD32 dwSibCount = 0;  /* 每个SI中的SIB数目  */
    for (dwSiLoop = 0; (dwSiLoop < ptSchedulingInfoList->n) && (dwSiLoop < 32); dwSiLoop++)
    {
        /* 每个SI中的SIB总数 */
        dwSibCount = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n;

        for (dwSibLoop = 0; (dwSibLoop < dwSibCount) && ( dwSibLoop < 31 ); dwSibLoop++)
        {
            if (eSibType == ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop])
            {
                return dwSiLoop;
            }
        }
    }
    return RNLC_FAIL;
    /* 初始化广播实例数据区 */
}

/*<FUNC>***********************************************************************
* 函数名称: CimUpdateSib1Sche
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimUpdateSib1Sche(SchedulingInfoList *ptSchedulingInfoList, T_AllSibList *ptAllSibList)
{
    SchedulingInfo  *ptSchedulingInfo = NULL;
    WORD32                dwSiNo      = 0;
    WORD32                dwSiLoop    = 0;

    ptAllSibList->tSib1Info.schedulingInfoList.n = 0;

    /* 32数值来源:协议定义结构 */
    for (dwSiLoop = 0; dwSiLoop < 32 ; dwSiLoop++)
    {
        if(dwSiLoop < ptSchedulingInfoList->n)
        {
            /*按照ptSchedulingInfoList的要求，去掉去掉的，并把后续的迁移操作*/
            /* 获取SI */
            ptSchedulingInfo = &(ptSchedulingInfoList->elem[dwSiLoop]);
            /* 若SI非空(相应的SIBs已经删除)，但是不排除第一个SI为空的情况(SIB2) */
            if ((0 < ptSchedulingInfo->sib_MappingInfo.n) || (0 == dwSiLoop))
            {
                memcpy(&(ptAllSibList->tSib1Info.schedulingInfoList.elem[dwSiNo]), ptSchedulingInfo, sizeof(SchedulingInfo));
                ptAllSibList->tSib1Info.schedulingInfoList.n++;
                dwSiNo = ptAllSibList->tSib1Info.schedulingInfoList.n;
            }
        }
        else
        {
            /*对已经按照ptSchedulingInfoList 处理过的后面的调度信息清零，
            可以防止出现后面的值再被应用的异常情况*/
            if(dwSiNo > (dwSiLoop +1))
            {
                /*按照正常情况，dwsino是不会大于dwsiloop 加1 的，最多是相等或者小于*/
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        dwSiNo, 
                        dwSiLoop,
                        RNLC_ERROR_LEVEL, 
                        " \n Update Sib1 Schedule Info Fail Happen!SINo = %d , Loop = %d \n "
                        ,dwSiNo,dwSiLoop);
                return;
            }
            memset(&(ptAllSibList->tSib1Info.schedulingInfoList.elem[dwSiNo]), 0, sizeof(SchedulingInfo));
            dwSiNo++;
        }
    }
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib2ResCfgForSuperCell
* 功能描述: 对bpl1板超级小区功能rs参考信号需要读取超级小区的配置中的
                          cp最大值，不能读取小区表中的配置值
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib2ResCfgForSuperCell(T_DBS_GetSib2Info_ACK *ptGetSib2InfoAck, T_AllSibList *ptAllSibList,WORD16 wCellId)
{
    CCM_NULL_POINTER_CHECK(ptGetSib2InfoAck);
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    PDSCH_ConfigCommon   *ptPdschCfg      = NULL;
    WORD16 wBoardType = 0;
    WORD32 dwResult =0;
    dwResult = USF_GetBplBoardTypeByCellId(wCellId, &wBoardType);
    /*函数处理错误则直接返回错误单板类型*/
    if( dwResult != RNLC_SUCC)
    {
         CCM_SIOUT_LOG(RNLC_FATAL_LEVEL,wCellId,"\n SI: [SI]Call USF_GetBplBoardTypeByCellId fail!\n"); 
         return RNLC_INVALID_DWORD;  
    }
    else
    {
       if (USF_BPLBOARDTYPE_BPL1 == wBoardType)
        {
           /*  填写PDSCH配置参数pdsch_Configurationpdsch_Configuration  中rs的参考信号字段*/
            ptPdschCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pdsch_ConfigCommon);
            /*    */
            T_DBS_GetSuperCpInfo_REQ tDbsGetSuperCpInfoReq;
            T_DBS_GetSuperCpInfo_ACK tDbsGetSuperCpInfoAck;
            memset(&tDbsGetSuperCpInfoReq, 0x00, sizeof(tDbsGetSuperCpInfoReq));
            memset(&tDbsGetSuperCpInfoAck, 0x00, sizeof(tDbsGetSuperCpInfoAck));
            tDbsGetSuperCpInfoReq.wCallType = USF_MSG_CALL;
            tDbsGetSuperCpInfoReq.wCId   = wCellId;
            BYTE ucResult = UsfDbsAccess(EV_DBS_GetSuperCpInfo_REQ, (VOID *)&tDbsGetSuperCpInfoReq, (VOID *)&tDbsGetSuperCpInfoAck);
            if ((FALSE == ucResult) || (0 != tDbsGetSuperCpInfoAck.dwResult))
            {
                 /*打印错误*/
                 CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: BPL1 Get Cp Info Fail !\n");
                return RNLC_INVALID_DWORD;
            }
            TSuperCPInfo      *ptSuperCpInfo = NULL;
            /* 协议: [-60,50]，Dbs=IE+60 */
            WORD16 wMaxCellSpeRefSigPwr = 0;   
            /*选取所有可用cp 信息中最大的rs参考信号功率*/
            for(BYTE ucCpLoop = 0;ucCpLoop < tDbsGetSuperCpInfoAck.ucCPNum && ucCpLoop < DBS_MAX_CP_NUM_PER_CID;ucCpLoop++ )
            {
                ptSuperCpInfo = &tDbsGetSuperCpInfoAck.atCpInfo[ucCpLoop];
                if(wMaxCellSpeRefSigPwr < ptSuperCpInfo->wCPSpeRefSigPwr && 
                   ptSuperCpInfo->dwManualOp == 0 &&
                   ptSuperCpInfo->dwStatus == 0)
                {
                    wMaxCellSpeRefSigPwr = ptSuperCpInfo->wCPSpeRefSigPwr;
                }
            }

            if(0 == wMaxCellSpeRefSigPwr)
            {      
                CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId,"\n SI: Sib2 referenceSignalPower is %d \n",wMaxCellSpeRefSigPwr);
                /*初始填为第一个cp的rs信号功率，防止所有cp都不可用的情况*/
                ptPdschCfg->referenceSignalPower = (tDbsGetSuperCpInfoAck.atCpInfo[0].wCPSpeRefSigPwr)/10 - 60;
            }
            else
            {
                /* 协议: [-60,50]，Dbs=IE+60 */
                ptPdschCfg->referenceSignalPower =(wMaxCellSpeRefSigPwr/10)-60;
            }
        }
    }
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimFillSib2RadioResCfgCommon
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib2RadioResCfgCommon(T_DBS_GetSib2Info_ACK *ptGetSib2InfoAck, T_AllSibList *ptAllSibList)
{
    RACH_ConfigCommon                *ptRachCfg       = NULL;
    BCCH_Config                      *ptBcchCfg       = NULL;
    PCCH_Config                      *ptPcchCfg       = NULL;
    PRACH_ConfigSIB                  *ptPrachCfgSib   = NULL;
    PDSCH_ConfigCommon               *ptPdschCfg      = NULL;
    UL_ReferenceSignalsPUSCH         *ptUlRefSignal   = NULL;
    PUCCH_ConfigCommon               *ptPucchCfg      = NULL;
    SoundingRS_UL_ConfigCommon       *ptSrsUlCfg      = NULL;
    UplinkPowerControlCommon         *ptUlPowCtrl     = NULL;
    SoundingRS_UL_ConfigCommon_setup *ptSrsCfgSetup   = NULL;
    PUSCH_ConfigCommon_pusch_ConfigBasic   *ptPuschCfgBasic = NULL;

    /* 2.1. 填写RACH配置rach_Configuration */
    ptRachCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.rach_ConfigCommon);

    /* 2.1.1 填写前导信息preambleInfo */
    /* 协议: 类型ENUM={n4,n8,n12,n16,n20,n24,n28,n32,n36,n40,n44,n48,n52,n56,n60,n64} */
    ptRachCfg->preambleInfo.numberOfRA_Preambles
    = (RACH_ConfigCommon_numberOfRA_Preambles)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucNumRAPreambles;

    /* 数据库不支持可选,必填 */
    ptRachCfg->preambleInfo.m.preamblesGroupAConfigPresent = 1;

    /* 协议: 类型ENUM={n4,n8,n12,n16,n20,n24,n28,n32,n36,n40,n44,n48,n52,n56,n60} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA
    = (RACH_ConfigCommon_sizeOfRA_PreamblesGroupA)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucSizeRAGroupA;

    /* 协议: 类型ENUM={b56,b144,b208,b256} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.messageSizeGroupA
    = (RACH_ConfigCommon_messageSizeGroupA)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucSelPreGrpThresh;

    /* 协议: 类型ENUM={minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB
    = (RACH_ConfigCommon_messagePowerOffsetGroupB)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMsgPwrOfstGrpB;

    ptRachCfg->preambleInfo.preamblesGroupAConfig.extElem1.numocts = 0;

    /* 2.1.2 填写功率攀升参数powerRampingParameters */
    /* 协议: 类型ENUM={dB0,dB2,dB4,dB6} */
    ptRachCfg->powerRampingParameters.powerRampingStep
    = (RACH_ConfigCommon_powerRampingStep)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucPrachPwrStep;

    /* 协议: 类型ENUM={dBm-120,dBm-118,dBm-116,dBm-114,dBm-112,dBm-110,dBm-108,dBm-106, */
    /*               dBm-104,dBm-102,dBm-100,dBm-98,dBm-96,dBm-94,dBm-92,dBm-90} */
    ptRachCfg->powerRampingParameters.preambleInitialReceivedTargetPower
    = (RACH_ConfigCommon_preambleInitialReceivedTargetPower)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucPreInitPwr;

    /* 2.1.3 填写随机接入检测信息ra_SupervisionInformation */
    /* 协议: 类型ENUM={n3,n4,n5,n6,n7,n8,n10,n20,n50,n100,n200} */
    ptRachCfg->ra_SupervisionInfo.preambleTransMax
    = (RACH_ConfigCommon_preambleTransMax)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMaxRetransNum;

    /* 协议: 类型ENUM={sf2,sf3,sf4,sf5,sf6,sf7,sf8,sf10} */
    ptRachCfg->ra_SupervisionInfo.ra_ResponseWindowSize
    = (RACH_ConfigCommon_ra_ResponseWindowSize)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucRARspWinSize;

    /* 协议: 类型ENUM={sf8,sf16,sf24,sf32,sf40,sf48,sf56,sf64} */
    ptRachCfg->ra_SupervisionInfo.mac_ContentionResolutionTimer
    = (RACH_ConfigCommon_mac_ContentionResolutionTimer)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMACContResTimer;

    /* 2.1.4 填写Msg3最大重传次数maxHARQ_Msg3Tx */
    ptRachCfg->maxHARQ_Msg3Tx
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMaxHARQMsg3Tx;
    ptRachCfg->extElem1.numocts = 0;

    /* 2.2 填写BCCH配置参数bcch_Configuration */
    ptBcchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.bcch_Config);
    ptBcchCfg->modificationPeriodCoeff
    = (BCCH_Config_modificationPeriodCoeff_Root)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tBcch_Config.ucBcchModPrdPara;

    /* 2.3 填写PCCH配置参数pcch_Configuration */
    ptPcchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pcch_Config);
    ptPcchCfg->defaultPagingCycle
    = (PCCH_Config_defaultPagingCycle)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPcch_Config.ucPagDrxCyc;
    ptPcchCfg->nB
    = (PCCH_Config_nB)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPcch_Config.ucnB;

    /* 2.4 填写PRACH配置参数prach_Configuration */
    ptPrachCfgSib = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.prach_Config);
    ptPrachCfgSib->rootSequenceIndex
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.wLogRtSeqStNum;

    ptPrachCfgSib->prach_ConfigInfo.prach_ConfigIndex
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucPrachConfig;

    /* 协议: 类型BOOL  */
    if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucCellHighSpdAtt)
    {
        ptPrachCfgSib->prach_ConfigInfo.highSpeedFlag = FALSE;
    }
    else
    {
        ptPrachCfgSib->prach_ConfigInfo.highSpeedFlag = TRUE;
    }

    ptPrachCfgSib->prach_ConfigInfo.zeroCorrelationZoneConfig
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucNcs;

    /* 0903协议变化:值域(0..104)改为(0..94) */
    ptPrachCfgSib->prach_ConfigInfo.prach_FreqOffset
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucPrachFreqOffset;

    /* 2.5 填写PDSCH配置参数pdsch_Configurationpdsch_Configuration */
    ptPdschCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pdsch_ConfigCommon);

    /* 协议: [-60,50]，Dbs=IE+60 */
    ptPdschCfg->referenceSignalPower
    = (WORD16)(ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPdsch_ConfigCommon.wCellSpeRefSigPwr/10) - 60;
    ptPdschCfg->p_b
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPdsch_ConfigCommon.ucPB;

    /* 2.6 填写PUSCH配置参数pusch_Configuration */
    ptPuschCfgBasic = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic);
    ptPuschCfgBasic->n_SB
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschNsb;
    ptPuschCfgBasic->hoppingMode
    = (PUSCH_ConfigCommon_hoppingMode)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschFHpMode;

    /* 0903协议变化: (0..63)改为(0..98) */
    ptPuschCfgBasic->pusch_HoppingOffset
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschhopOfst;

    /* 协议: 类型BOOL {0:Not support; 1:Support} */
    if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucUl64QamDemSpInd)
    {
        ptPuschCfgBasic->enable64QAM = FALSE;
    }
    else
    {
        ptPuschCfgBasic->enable64QAM = TRUE;
    }


    ptUlRefSignal = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH);

    /* 协议: 类型BOOL {0:Disable; 1:Enable} */
    if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucGrpHopEnableInd)
    {
        ptUlRefSignal->groupHoppingEnabled = FALSE;
    }
    else
    {
        ptUlRefSignal->groupHoppingEnabled = TRUE;
    }

    ptUlRefSignal->groupAssignmentPUSCH
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucDeltaSs;
    ptUlRefSignal->sequenceHoppingEnabled
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucSeqHopEnableInd;
    ptUlRefSignal->cyclicShift
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucnDMRS1;

    /* 2.7 填写PUCCH配置参数pucch_Configuration */
    ptPucchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pucch_ConfigCommon);
    ptPucchCfg->deltaPUCCH_Shift
    = (PUCCH_ConfigCommon_deltaPUCCH_Shift)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucPucchDeltaShf;
    ptPucchCfg->nRB_CQI
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucNumPucchCqiRB;
    ptPucchCfg->nCS_AN
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucPucchNcsAn;

    /* mXA0003551:SR信道数+半静态A/N信道数+A/N Repetition信道数 */
    ptPucchCfg->n1PUCCH_AN
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchSemiAn +
      ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchSr +
      ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchAckRep;

    /* 2.8 填写上行Sounding公共配置参数soundingRsUl_ConfigCommon */
    ptSrsUlCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.soundingRS_UL_ConfigCommon);

    /* 协议: TS36.331v850 类型CHOICE(release,setup) */
    if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tSoundingRSULConfigCommon.ucSrsEnable)
    {
        ptSrsUlCfg->t = T_SoundingRS_UL_ConfigCommon_release;
    }
    else
    {
        ptSrsUlCfg->t = T_SoundingRS_UL_ConfigCommon_setup;
    }

    if (T_SoundingRS_UL_ConfigCommon_setup == ptSrsUlCfg->t)
    {
        ptSrsCfgSetup = ptSrsUlCfg->u.setup;

        /* 可选(Condition of TDD) */
        if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tSoundingRSULConfigCommon.ucSrsMaxUpPts)
        {
            ptSrsCfgSetup->m.srs_MaxUpPtsPresent = 0;
        }
        else
        {
            ptSrsCfgSetup->m.srs_MaxUpPtsPresent = 1;
            ptSrsCfgSetup->srs_MaxUpPts = (SoundingRS_UL_ConfigCommon_srs_MaxUpPts)true_2;
        }

        ptSrsCfgSetup->srs_BandwidthConfig
        = (SoundingRS_UL_ConfigCommon_srs_BandwidthConfig)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tSoundingRSULConfigCommon.ucSrsBWCfg;
        ptSrsCfgSetup->srs_SubframeConfig
        = (SoundingRS_UL_ConfigCommon_srs_SubframeConfig)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tSoundingRSULConfigCommon.ucSrsSubFrameCfg;
        ptSrsCfgSetup->ackNackSRS_SimultaneousTransmission
        = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tSoundingRSULConfigCommon.ucAckSrsTraSptInd;
        ptSrsUlCfg->u.setup = ptSrsCfgSetup;
    }

    /* 2.9 填写上行链路功率配置参数uplinkPowerControlCommon */
    ptUlPowCtrl = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.uplinkPowerControlCommon);

    /* 协议: [-126,24]，Dbs = IE+126 */
    ptUlPowCtrl->p0_NominalPUSCH
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucPoNominalPusch1 - 126;

    ptUlPowCtrl->alpha
    = (UplinkPowerControlCommon_alpha)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucAlfa;

    /* 协议: [-127,-96]，Dbs = IE+127*/
    ptUlPowCtrl->p0_NominalPUCCH
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucPoNominalPucch - 127;

    ptUlPowCtrl->deltaFList_PUCCH.deltaF_PUCCH_Format1
    = (DeltaFList_PUCCH_deltaF_PUCCH_Format1)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPoPucchF1;
    ptUlPowCtrl->deltaFList_PUCCH.deltaF_PUCCH_Format1b
    = (DeltaFList_PUCCH_deltaF_PUCCH_Format1b)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPoPucchF1b;
    ptUlPowCtrl->deltaFList_PUCCH.deltaF_PUCCH_Format2
    = (DeltaFList_PUCCH_deltaF_PUCCH_Format2)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPoPucchF2;
    ptUlPowCtrl->deltaFList_PUCCH.deltaF_PUCCH_Format2a
    = (DeltaFList_PUCCH_deltaF_PUCCH_Format2a)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPoPucchF2a;
    ptUlPowCtrl->deltaFList_PUCCH.deltaF_PUCCH_Format2b
    = (DeltaFList_PUCCH_deltaF_PUCCH_Format2b)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPoPucchF2b;

    /* 协议: [-1,6]，Dbs = IE+1 */
    ptUlPowCtrl->deltaPreambleMsg3
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPrmbMsg3 - 1;

    /* 2.10 ul_CyclicPrefixLength */
    ptAllSibList->tSib2Info.radioResourceConfigCommon.ul_CyclicPrefixLength
    = (UL_CyclicPrefixLength)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.ucPhyChCPSel;

    ptAllSibList->tSib2Info.radioResourceConfigCommon.extElem1.numocts = 0;

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimGetAllowMeasBwOfFreq
* 功能描述: 获取同频邻区允许测量的最小带宽
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetAllowMeasBwOfFreq(WORD16 wCellId, BYTE ucRadioMode, T_DBS_GetSib3Info_ACK *ptGetSib3InfoAck)
{
    BYTE   ucMinDlBw       = 0;
    BYTE   ucNbrDlBw       = 0;
    BYTE   ucResult        = FALSE;
    WORD32 dwNbrCellNum    = 0;
    WORD32 dwNbrCellLoop   = 0;
    WORD32 dwNbrDlCntrFreq = 0;
#if 0

    T_DBS_GetCellCfgInfo_REQ tGetCellCfgInfoReq;
    T_DBS_GetCellCfgInfo_ACK tGetCellCfgInfoAck;
#endif
    T_DBS_GetAdjCellInfoByCellId_REQ tGetAdjCellInfoByCellIdReq;
    T_DBS_GetAdjCellInfoByCellId_ACK tGetAdjCellInfoByCellIdAck;
    T_DBS_GetAllIntraFreqLTENbrCellInfo_REQ  tGetAllIntraFreqLTENbrCellInfoReq;
    T_DBS_GetAllIntraFreqLTENbrCellInfo_ACK  tGetAllIntraFreqLTENbrCellInfoAck;

    CCM_NULL_POINTER_CHECK(ptGetSib3InfoAck);
#if 0

    memset((VOID*)&tGetCellCfgInfoReq,  0, sizeof(tGetCellCfgInfoReq));
    memset((VOID*)&tGetCellCfgInfoAck,  0, sizeof(tGetCellCfgInfoAck));
#endif
    memset((VOID*)&tGetAdjCellInfoByCellIdReq,  0, sizeof(tGetAdjCellInfoByCellIdReq));
    memset((VOID*)&tGetAdjCellInfoByCellIdAck,  0, sizeof(tGetAdjCellInfoByCellIdAck));
    memset((VOID*)&tGetAllIntraFreqLTENbrCellInfoReq,  0, sizeof(tGetAllIntraFreqLTENbrCellInfoReq));
    memset((VOID*)&tGetAllIntraFreqLTENbrCellInfoAck,  0, sizeof(tGetAllIntraFreqLTENbrCellInfoAck));


    /* 没有邻区信息，则取默认的最小值 */
    tGetAllIntraFreqLTENbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllIntraFreqLTENbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllIntraFreqLTENbrCellInfo_REQ, (VOID *)&tGetAllIntraFreqLTENbrCellInfoReq, (VOID *)&tGetAllIntraFreqLTENbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllIntraFreqLTENbrCellInfoAck.dwResult))
    {
        return mbw6;
    }

    dwNbrCellNum = tGetAllIntraFreqLTENbrCellInfoAck.wIntraFreqLTENbrCellNum;
    if (0 == dwNbrCellNum)
    {
        return mbw6;
    }

    /* 初始化 */
    ucMinDlBw = mbw100 + 1;

    /* 邻小区属性判断并记录 */
    for (dwNbrCellLoop = 0;
            (dwNbrCellLoop < dwNbrCellNum) && ( dwNbrCellLoop < DBS_MAX_INTRA_FREQ_NBRNUM_PER_CELL);
            dwNbrCellLoop++)
    {
        if (1 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucBlackNbrCellInd)
        {
            /* 不考虑黑名单小区 */
            continue;
        }

        /* 如果是eNB内的小区，则读取SRVCEL获取其相关参数 */
        if ((tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMcc == ptGetSib3InfoAck->tSib3.wMCC) &&
                (tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMnc == ptGetSib3InfoAck->tSib3.wMNC) &&
                (tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.dwENodeBId == ptGetSib3InfoAck->tSib3.dwEnodebID))
        {
            T_DBS_GetCellInfoByCellId_REQ tDBSGetCellInfoByCellId_REQ;
            memset(&tDBSGetCellInfoByCellId_REQ, 0, sizeof(tDBSGetCellInfoByCellId_REQ));
            tDBSGetCellInfoByCellId_REQ.wCellId = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId;
            tDBSGetCellInfoByCellId_REQ.wCallType = USF_MSG_CALL;

            T_DBS_GetCellInfoByCellId_ACK tDBSGetCellInfoByCellId_ACK;
            memset(&tDBSGetCellInfoByCellId_ACK, 0, sizeof(tDBSGetCellInfoByCellId_ACK));

            BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetCellInfoByCellId_REQ,
                                             static_cast<VOID*>(&tDBSGetCellInfoByCellId_REQ),
                                             static_cast<VOID*>(&tDBSGetCellInfoByCellId_ACK));
            if ((!bDbResult) || (0 != tDBSGetCellInfoByCellId_ACK.dwResult))
            {
                CCM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetCellInfoByCellId_REQ,
                                    tDBSGetCellInfoByCellId_ACK.dwResult,
                                    bDbResult,
                                    RNLC_FATAL_LEVEL,
                                    " CCM_CIM_MainComponentFsm DBAccessFail_GetCellInfoByCellId_REQ! ");

                continue;
            }

            /* 获取下行中心频率和系统带宽 */
            if (CIM_LTE_MODE_FDD == ucRadioMode)
            {
                ucNbrDlBw = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.ucDlBandwidth;
                dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
            }

            if (CIM_LTE_MODE_TDD == ucRadioMode)
            {
                ucNbrDlBw = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.ucDlBandwidth;
                dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
            }

        }
        else
        {
            /* 否则，其它eNB小区读取ADJCEL获取其相关参数 */
            tGetAdjCellInfoByCellIdReq.wCallType = USF_MSG_CALL;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMcc
            = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMcc;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMnc
            = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMnc;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.dwENodeBId
            = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.dwENodeBId;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wCellId
            = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetAdjCellInfoByCellId_REQ, (VOID *)&tGetAdjCellInfoByCellIdReq, (VOID *)&tGetAdjCellInfoByCellIdAck);
            if ((FALSE == ucResult) || (0 != tGetAdjCellInfoByCellIdAck.dwResult))
            {
                continue;
            }

            /* 获取下行中心频率和系统带宽 */
            ucNbrDlBw = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.ucDlSysBandWidth;
            dwNbrDlCntrFreq = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.dwDlCenterFreq;
        }

        if (ucNbrDlBw < ucMinDlBw)
        {
            ucMinDlBw = ucNbrDlBw;
        }
    }

    return ((mbw100 + 1) == ucMinDlBw) ? mbw6 : ucMinDlBw;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib7NccPermitted
* 功能描述: 填写SIB7中Permitted NCC信息
* 算法描述: 1.获取该邻区信息，包括NCC、ARFCN和频段标识;
*           2.特别的，如果该邻小区的ARFCN位于[512,810]，则需要判断该邻小区的频段标识是否
*           和当前ARFCN集合的频段标识相同, 如果ARFCN值位于[512,810]，那么UE区分不出属于
*           DCS1800频段还是PCS1900频段
*           3.如果该邻小区的ARFCN属于当前的ARFCN集合(即信元carrierFreqs)，则以
*           该邻区的NCC为索引，将ncc_permitted中相应的bit位置1；
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimFillSib7NccPermitted(T_DBS_GetAllGSMNbrCellInfo_ACK  *ptGetAllGSMNbrCellInfoAck,
                             CarrierFreqsGERAN  *ptCarrierFreqsGERAN,
                             BYTE *pucNccPermitted)
{
    WORD32  dwGsmNbrCellLoop = 0;
    WORD32  dwGsmNbrCellNum  = 0;
    BYTE    ucResult         = FALSE;

    T_DBS_GetGSMAdjCellInfoByCellId_REQ tGetGSMAdjCellInfoByCellIdReq;
    T_DBS_GetGSMAdjCellInfoByCellId_ACK tGetGSMAdjCellInfoByCellIdAck;

    memset((VOID*)&tGetGSMAdjCellInfoByCellIdReq,  0, sizeof(tGetGSMAdjCellInfoByCellIdReq));
    memset((VOID*)&tGetGSMAdjCellInfoByCellIdAck,  0, sizeof(tGetGSMAdjCellInfoByCellIdAck));

    dwGsmNbrCellNum = ptGetAllGSMNbrCellInfoAck->wGSMNbrCellNum;
    for (dwGsmNbrCellLoop = 0; dwGsmNbrCellLoop < dwGsmNbrCellNum; dwGsmNbrCellLoop++)
    {
        /* 根据邻区的标识查询其NCC */
        tGetGSMAdjCellInfoByCellIdReq.wCallType = USF_MSG_CALL;
        tGetGSMAdjCellInfoByCellIdReq.tGSMAdjCellId.wMCC = ptGetAllGSMNbrCellInfoAck->atTGSMNbrCellInfo[dwGsmNbrCellLoop].tGSMCellId.wMCC;
        tGetGSMAdjCellInfoByCellIdReq.tGSMAdjCellId.wMNC = ptGetAllGSMNbrCellInfoAck->atTGSMNbrCellInfo[dwGsmNbrCellLoop].tGSMCellId.wMNC;
        tGetGSMAdjCellInfoByCellIdReq.tGSMAdjCellId.wLAC = ptGetAllGSMNbrCellInfoAck->atTGSMNbrCellInfo[dwGsmNbrCellLoop].tGSMCellId.wLAC;
        tGetGSMAdjCellInfoByCellIdReq.tGSMAdjCellId.wCellId = ptGetAllGSMNbrCellInfoAck->atTGSMNbrCellInfo[dwGsmNbrCellLoop].tGSMCellId.wCellId;
        ucResult = UsfDbsAccess(EV_DBS_GetGSMAdjCellInfoByCellId_REQ, (VOID *)&tGetGSMAdjCellInfoByCellIdReq, (VOID *)&tGetGSMAdjCellInfoByCellIdAck);
        if ((FALSE == ucResult) || (0 != tGetGSMAdjCellInfoByCellIdAck.dwResult))
        {
           CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB7 Get GSM Adjacent Cell(ID= %d) error!\n",tGetGSMAdjCellInfoByCellIdReq.tGSMAdjCellId.wCellId);
        }
        else
        {
            /* 如果该小区的ARFCN位于[512,810]，则首先判断频段指示 */
            if (((512 <= tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN) && (tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN <= 810)) &&
                    (ptCarrierFreqsGERAN->bandIndicator != tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.ucBandIndicator))
            {
                /* 如果频段不一样，则不再考虑此小区的NCC */
                continue;
            }

            /* 判断该小区的频点是否在当前频点集合中 */
            if (ptCarrierFreqsGERAN->startingARFCN == tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN)
            {
                /* 如果和起始ARFCN相同，则将对应的bit位置1；从左到右对应NCC[0~7] */
                *pucNccPermitted |= 0X80 >> tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.ucNCC;
                continue;
            }

            /* to do:不在剩余的ARFCN列表中查询,因为目前数据库暂不支持剩余ARFCN的填写 */
        }
    }

    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimGetBandLocation
* 功能描述: 判断频带byBandClass是否在频带列表ptBandList中存在，如果存在，
*           pdwBandLocation记录下标位置，返回TRUE，不存在返回FALSE
* 算法描述:
* 全局变量:
* Input参数:
* 返 回 值:
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetBandLocation(BYTE ucBandClass, NeighCellListCDMA2000 *ptBandList, WORD32 *pdwBandLocation)
{
    WORD32 dwLoop      = 0;
    WORD32   dwBandExist = RNLC_FAIL;

    /* 判断频带是否被填过 */
    for (dwLoop = 0; (dwLoop < ptBandList->n) && (dwLoop < 16); dwLoop++)
    {
        if (ucBandClass == ptBandList->elem[dwLoop].bandClass)
        {
            /* 记录频带位置 */
            *pdwBandLocation = dwLoop;
            dwBandExist = RNLC_SUCC;
        }
    }

    return dwBandExist;
}

/*<FUNC>***********************************************************************
* 函数名称: BcmdaFillSib8NeighCellList
* 功能描述: 判断频点wFreq是否在频点列表ptFreqList中存在，如果存在，
            pwFreqLocation记录下标位置，返回TRUE，不存在返回FALSE
* 算法描述:
* 全局变量:
* Input参数:
* 返 回 值:
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BYTE CimGetFreqLocation(WORD16 wFreq, NeighCellsPerBandclassListCDMA2000 *ptFreqList,
                        WORD32 *pwFreqLocation)
{
    WORD32 dwLoop = 0;
    BYTE   ucFreqExist = FALSE;

    /* 频点是否已被填过 */
    for (dwLoop = 0; (dwLoop < ptFreqList->n) && (dwLoop < 16); dwLoop++)
    {
        if (wFreq == ptFreqList->elem[dwLoop].arfcn)
        {
            /* 记录频点位置 */
            *pwFreqLocation = dwLoop;
            return ucFreqExist;
        }
    }

    return ucFreqExist;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib8CdmaNbrList
* 功能描述: 以CDMANBR表中的邻区标识为索引在CDMAADJCEL表中找到该邻区的属性信息:bandclass,
*           arfcn和phycellID,然后以bandclass为类别填写arfcn列表,再以arfcn为类别填写
*           小区列表
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BYTE CimFillSib8CdmaNbrList(T_DBS_GetAllCDMANbrCellInfo_ACK *ptGetAllCDMANbrCellInfoAck,
                                 BYTE ucCellType, NeighCellListCDMA2000 *ptNbrElement,
                                   NeighCellListCDMA2000_v920 *ptNeighCellList_v920 )
{
    WORD32   dwCdmaNbrCellNum  = 0;
    WORD32   dwCdmaNbrCellLoop = 0;
    BYTE     ucResult          = 0;
    BYTE     ucNeighCellV920Flag          = 0;
    BYTE     ucFreqExist                  = FALSE;
    WORD32   dwBandExist       = RNLC_FAIL;
    WORD32   dwBandLocation    = 0;
    WORD32   dwFreqLocation    = 0;
    WORD32   dwNeighCelllistLoop          = 0;

    PhysCellIdListCDMA2000_v920        *ptCellListv920           = NULL;
    PhysCellIdListCDMA2000             *ptCellList               = NULL;
    NeighCellsPerBandclassListCDMA2000 *ptFreqList               = NULL;

    T_DBS_GetCDMAAdjCellInfoByCellId_REQ tGetCDMAAdjCellInfoByCellIdReq;
    T_DBS_GetCDMAAdjCellInfoByCellId_ACK tGetCDMAAdjCellInfoByCellIdAck;

    memset((VOID*)&tGetCDMAAdjCellInfoByCellIdReq,  0, sizeof(tGetCDMAAdjCellInfoByCellIdReq));
    memset((VOID*)&tGetCDMAAdjCellInfoByCellIdAck,  0, sizeof(tGetCDMAAdjCellInfoByCellIdAck));

    memset(ptNbrElement, 0, sizeof(NeighCellListCDMA2000));
    memset(ptNeighCellList_v920, 0, sizeof(NeighCellListCDMA2000_v920));
    dwCdmaNbrCellNum  = ptGetAllCDMANbrCellInfoAck->wCDMANbrCellNum;
    for (dwCdmaNbrCellLoop = 0; dwCdmaNbrCellLoop < dwCdmaNbrCellNum; dwCdmaNbrCellLoop++)
    {
        /* 读取CDMANBR */
        tGetCDMAAdjCellInfoByCellIdReq.wCallType = USF_MSG_CALL;
        memmove(&tGetCDMAAdjCellInfoByCellIdReq.tCDMAAdjCellId,
        &ptGetAllCDMANbrCellInfoAck->atCDMANbrCellInfo[dwCdmaNbrCellLoop].tCDMACellId,
        sizeof(tGetCDMAAdjCellInfoByCellIdReq.tCDMAAdjCellId));        
        ucResult = UsfDbsAccess(EV_DBS_GetCDMAAdjCellInfoByCellId_REQ, (VOID *)&tGetCDMAAdjCellInfoByCellIdReq, (VOID *)&tGetCDMAAdjCellInfoByCellIdAck);
        if ((FALSE == ucResult) || (0 != tGetCDMAAdjCellInfoByCellIdAck.dwResult))
        {
            CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB8 Get CDMA Adjcent Cell error!\n");
        }

        if (ucCellType != tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.tCDMACellId.ucCellType)
        {
            CCM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB8 This Neighber Cell (Cell Id = %d) Type ( %d )don't Match Now Filling Type( %d)! \n",
                       tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.tCDMACellId.wCId,
                       tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.tCDMACellId.ucCellType,
                       ucCellType);
            continue;
        }

        /* 判断该频带是否已经填过 */
        dwBandExist = CimGetBandLocation(tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.ucBandClass,
                                        ptNbrElement, &dwBandLocation);
        if (RNLC_SUCC == dwBandExist)
        {
            /* 判断该邻区频点是否已经填过 */
            ucFreqExist = CimGetFreqLocation(tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq,
                         &ptNbrElement->elem[dwBandLocation].neighCellsPerFreqList,&dwFreqLocation);

            if (TRUE == ucFreqExist)
            {
                /* 增加该频点下的邻区 */
                ptCellList = &ptNbrElement->elem[dwBandLocation].neighCellsPerFreqList.elem[dwFreqLocation].physCellIdList;
                ptCellListv920 = &ptNeighCellList_v920->elem[dwBandLocation].neighCellsPerFreqList_v920.elem[dwFreqLocation].physCellIdList_v920;
                if (ptCellList->n < 16)
                {
                    ptCellList->elem[ptCellList->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;
                    ptCellList->n++;
                }
                else
                {   
                    ptCellListv920->elem[ptCellListv920->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;
                    ptCellListv920->n++;
                    if(0 == ucNeighCellV920Flag)
                    {
                        ucNeighCellV920Flag = 1;
                    }
                }
            }
            else
            {
                /* 频点没填过，则小区一定没填过，先增加该频点，再增加该频点下的小区 */
                ptFreqList = &ptNbrElement->elem[dwBandLocation].neighCellsPerFreqList;

                /* 增加频点 */
                //if ((ptFreqList->n < 16) && (ptCellList->n < 16))
                {
                    ptFreqList->elem[ptFreqList->n].arfcn = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq;

                    /* 增加小区 */
                    ptCellList = &ptFreqList->elem[ptFreqList->n].physCellIdList;
                    ptCellList->elem[ptCellList->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;

                    ptFreqList->n++;  /*频点数加1*/
                    ptCellList->n++;  /*小区数加1*/
                }
            }
        }
        else
        {
            //if ((ptNbrElement->n < 16) && (ptFreqList->n < 16) && (ptCellList->n < 16))
            {
                /* 频带没有填入过，则频点、小区一定没有填入过，
                先增加该邻区频带,再增加该频带下的频点，最后增加该频点下的小区 */
                ptNbrElement->elem[ptNbrElement->n].bandClass = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.ucBandClass;

                /* 增加频点 */
                ptFreqList = &ptNbrElement->elem[ptNbrElement->n].neighCellsPerFreqList;
                ptFreqList->elem[ptFreqList->n].arfcn = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq;

                /* 增加小区 */
                ptCellList = &ptFreqList->elem[ptFreqList->n].physCellIdList;
                ptCellList->elem[ptCellList->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;

                ptNbrElement->n++;  /* 频带数加1 */
                ptFreqList->n++;    /* 频点数加1 */
                ptCellList->n++;    /* 小区数加1 */
            }
        }
    }

    if (0 == ptNbrElement->n)
    {
        return FALSE;
    }

    if(1 == ucNeighCellV920Flag)
    {
        ptNeighCellList_v920->n = ptNbrElement->n;
        for( dwNeighCelllistLoop = 0 ; dwNeighCelllistLoop < ptNbrElement->n ; dwNeighCelllistLoop ++ )
        {
            ptNeighCellList_v920->elem[dwNeighCelllistLoop].neighCellsPerFreqList_v920.n 
            = ptNbrElement->elem[dwNeighCelllistLoop].neighCellsPerFreqList.n;
        }
    }

    CCM_LOG(RNLC_INFO_LEVEL, "\n SI: This CDMA ucNeighCellV920Flag = %d !\n",ucNeighCellV920Flag);
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillMibInfo
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillMibInfo(T_DBS_GetMibInfo_ACK *ptGetMibInfoAck, T_AllSibList *ptAllSibList)
{
    CCM_NULL_POINTER_CHECK(ptGetMibInfoAck);
    CCM_NULL_POINTER_CHECK(ptAllSibList);

    CCM_LOG(RNLC_INFO_LEVEL, "\n SI: Fill MIB Info! \n");
    
    /* 填写系统带宽dl_Bandwidth */
    ptAllSibList->tMibInfo.dl_Bandwidth = (MasterInformationBlock_dl_Bandwidth)ptGetMibInfoAck->tMib.ucSysBandWidth;
    
    /* 填写PHICH配置phich_Configuration */
    ptAllSibList->tMibInfo.phich_Config.phich_Duration = (PHICH_Config_phich_Duration)ptGetMibInfoAck->tMib.ucPhichDuration;
    ptAllSibList->tMibInfo.phich_Config.phich_Resource = (PHICH_Config_phich_Resource)ptGetMibInfoAck->tMib.ucNg;
    
    /* 填写系统帧号systemFrameNumber */
    ptAllSibList->tMibInfo.systemFrameNumber.numbits = 8; /* 协议规定8bit */
    ptAllSibList->tMibInfo.systemFrameNumber.data[0] = 0; /* SFN偏移位置：3+3=6bit */
    /* 标准协议版本*/
    if(0 == g_ucSceneCfg||2 == g_ucSceneCfg)
    {
         ptAllSibList->tMibInfo.spare.numbits = 10;            /* 协议规定10bit */
         ptAllSibList->tMibInfo.spare.data[0] = 0;
         ptAllSibList->tMibInfo.spare.data[1] = 0;
    }
    /* 航线高铁版本*/
    else 
    {
         ptAllSibList->tMibInfo.spare.numbits = 10;            /* 协议规定10bit */
         ptAllSibList->tMibInfo.spare.data[0] = 170;           /* 高铁覆盖要求10bit为1010101010 */
         ptAllSibList->tMibInfo.spare.data[1] = 170;    
    }

    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib1Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib1Info(BYTE ucRadioMode,BYTE ucLastValueTag, T_DBS_GetSib1Info_ACK *ptGetSib1InfoAck, T_AllSibList *ptAllSibList)
{
    SystemInformationBlockType1_cellAccessRelatedInfo  *ptCellAccessInfo    = NULL;
    SystemInformationBlockType1_cellSelectionInfo      *ptCellSelectionInfo = NULL;
    SystemInformationBlockType1_v920_IEs                      *ptRNineExtension  = NULL;
    TDD_Config       *ptTddCfg = NULL;
    PLMN_Identity    *ptPlmnId = NULL;
    WORD32   dwIdLoop     = 0;
    WORD16   wMcc         = 0;
    WORD16   wMnc         = 0;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID, "\n SI: Fill SIB1 Info! \n");


    /* 1. 填充小区接入相关信息cellAccessRelatedInformation */
    ptCellAccessInfo = &(ptAllSibList->tSib1Info.cellAccessRelatedInfo);

    /* 1.1 填充PLMN列表plmn_IdentityList */

    /*
    MCC和MNC的转化说明:
    MCC固定长度为3，且三位的每位的取值范围为0~9
    MNC长度可以为2或3，且有效位的取值范围是0~9，其中020和20表示的含义是不同的

    数据库给CCM带来的值是一个WORD16的值，其中F表示无效
    举例说明:MCC = 460，那么数据带来的值为0xF460
    MNC = 020，则数据库带来的值为0xF020; MNC = 20，则数据库带来的值为0xFF20;
    */
    if (0 == g_tSystemDbgSwitch.ucS1DbgSwitch)
    {
        T_DBS_GetPlmnByCellId_REQ tDbsGetPlmnReq;
        T_DBS_GetPlmnByCellId_ACK tDbsGetPlmnAck;
        BOOL bResult = FALSE;

        memset(&tDbsGetPlmnReq, 0x00 ,sizeof(tDbsGetPlmnReq));
        memset(&tDbsGetPlmnAck, 0x00, sizeof(tDbsGetPlmnAck));

        tDbsGetPlmnReq.wCallType = USF_MSG_CALL;
        tDbsGetPlmnReq.wCellId = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID;
        bResult = UsfDbsAccess(EV_DBS_GetPlmnByCellId_REQ, (VOID *)&tDbsGetPlmnReq, (VOID *)&tDbsGetPlmnAck);
        if ((FALSE == bResult) || (0 != tDbsGetPlmnAck.dwResult))
        {
            /* print */
            return RNLC_FAIL;
        }
        /*设计变更*/
#if 0
        memset(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC, 0, sizeof((WORD16 *)(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC)));
        memset(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC, 0, sizeof((WORD16 *)(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC)));

        ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucPlmnNum = tDbsGetPlmnAck.ucPlmnNum;
        memmove(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC, tDbsGetPlmnAck.awMcc, sizeof((WORD16 *)(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC)));
        memmove(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC, tDbsGetPlmnAck.awMnc, sizeof((WORD16 *)(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC)));
#endif 
        
        memset(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC, 0, sizeof((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC)));
        memset(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC, 0, sizeof((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC)));

        ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucPlmnNum = tDbsGetPlmnAck.ucPlmnNum;
        memmove(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC, tDbsGetPlmnAck.awMcc, sizeof((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC)));
        memmove(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC, tDbsGetPlmnAck.awMnc, sizeof((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC)));

}

    
    ptCellAccessInfo->plmn_IdentityList.n = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucPlmnNum;
    for (dwIdLoop = 0; dwIdLoop < min(ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucPlmnNum, MAX_NUM_PLMN_PRE_CELL); dwIdLoop++)
    {
        ptPlmnId = &(ptCellAccessInfo->plmn_IdentityList.elem[dwIdLoop].plmn_Identity);
        ptPlmnId->m.mccPresent = 1;

        /* 判断MCC是否合法 */
        if (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] & 0x000F) > 9)
            || (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] >> 4) & 0x000F) > 9)
            || (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] >> 8) & 0x000F) > 9))
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID, "\n SI: MCC is error. MCC=%d\n",ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop]);

            return FALSE;
        }
        wMcc = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] & 0x0fff;
        ptPlmnId->mcc.n = 3;
        ptPlmnId->mcc.elem[0] = (wMcc >> 8)  & 0x000F;        /* 百 */
        ptPlmnId->mcc.elem[1] = (wMcc >> 4)  & 0x000F;        /* 十 */
        ptPlmnId->mcc.elem[2] = wMcc & 0x000F;                   /* 个 */

        /* 判断MNC是否合法 */
        wMnc = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop];
        if (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop]& 0x000F) > 9)
            || (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop] >> 4) & 0x000F) > 9)
            || ((9<((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop] >> 8) & 0x000F))&&(((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop] >> 8) & 0x000F)<15)))
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID, "\n SI: MNC is error. MNC=%d\n",ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMNC[dwIdLoop]);

            return FALSE;
        }
        if (wMnc >= 0xff00)
        {
            wMnc = wMnc & 0x00ff;
            ptPlmnId->mnc.n = 2;
            ptPlmnId->mnc.elem[0] = (wMnc >> 4)  & 0x000F;       /* 十 */
            ptPlmnId->mnc.elem[1] =  wMnc & 0x000F;                 /* 个 */
        }
        else
        {
            wMnc = wMnc & 0x0fff;
            ptPlmnId->mnc.n = 3;
            ptPlmnId->mnc.elem[0] =  (wMnc >> 8)  & 0x000F;       /* 百 */
            ptPlmnId->mnc.elem[1] =  (wMnc >> 4)  & 0x000F;  /* 十 */
            ptPlmnId->mnc.elem[2] =  wMnc & 0x000F;        /* 个 */
        }

        ptCellAccessInfo->plmn_IdentityList.elem[dwIdLoop].cellReservedForOperatorUse
        = (PLMN_IdentityInfo_cellReservedForOperatorUse)ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.aucCellResvForOprUse[dwIdLoop];
    }

    /* 1.2 填充追踪域码trackingAreaCode */
    ptCellAccessInfo->trackingAreaCode.numbits = 16;  /* [16,16] */
    ptCellAccessInfo->trackingAreaCode.data[0]
    = (ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wTAC) >> 8;     /* TA码高8bit */
    ptCellAccessInfo->trackingAreaCode.data[1]
    = (ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wTAC) & 0x00FF; /* TA码低8bit */

    /* 1.3 填充小区标识cellIdentity */
    ptCellAccessInfo->cellIdentity.numbits = 28; /* [28,28] */
    ptCellAccessInfo->cellIdentity.data[0]
    = (BYTE)((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.dwEnodebID >> 12) & 0x00FF); /* dwENodeBId 20bit */
    ptCellAccessInfo->cellIdentity.data[1]
    = (BYTE)((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.dwEnodebID >> 4) & 0x00FF);
    ptCellAccessInfo->cellIdentity.data[2]
    = (BYTE)(((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.dwEnodebID & 0x0000000F) << 4) +
         ((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID & 0xF0) >> 4));
    ptCellAccessInfo->cellIdentity.data[3]
    = ((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID) & 0x0F) << 4; /* [0,255] */

    /* 1.4 填充其他cellBarred，intraFreqReselection，csg_Indication */
    if(SI_UPD_CELL_RECFG_DEL == ptAllSibList->ucResv)
    {
        ptCellAccessInfo->cellBarred = (SystemInformationBlockType1_cellBarred)0;
    }
    else
    {
        ptCellAccessInfo->cellBarred
        = (SystemInformationBlockType1_cellBarred)ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucCellBarred;
    }
    ptCellAccessInfo->intraFreqReselection
    = (SystemInformationBlockType1_intraFreqReselection)ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucIntraFReselInd;
    ptCellAccessInfo->csg_Indication
    = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.ucCsgInd;

    /* 1.5 暂不支持CSGID */
    ptCellAccessInfo->m.csg_IdentityPresent = 0;

    /* 2. 填充小区选择相关信息cellSelectionInfo */
    ptCellSelectionInfo = &(ptAllSibList->tSib1Info.cellSelectionInfo);
    ptCellSelectionInfo->q_RxLevMin
    = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucSelQrxLevMin - 70; /* [-70,-22] */

    /* 数据库不支持可选,必填 */
    ptCellSelectionInfo->m.q_RxLevMinOffsetPresent = 1;
    ptCellSelectionInfo->q_RxLevMinOffset
    = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucQrxLevMinOfst; /* [1,8]*/

    /* 3. 填充最大传输功率p_Max，数据库不支持可选,必填 */
    ptAllSibList->tSib1Info.m.p_MaxPresent = 1;
    ptAllSibList->tSib1Info.p_Max = ptGetSib1InfoAck->tSib1.ucP_Max - 30; /* [-30,33] */

    /* 4. 填充频带指示frequencyBandIndicator */
    ptAllSibList->tSib1Info.freqBandIndicator = (ASN1INT)(ptGetSib1InfoAck->tSib1.ucFreqBandIndicator) ; /* [1,64] */

    /* 5. 调度信息在确定实际可以下发的SIB后填写 */

    /* 6. 填写TDD配置参数 */
    if (CIM_LTE_MODE_FDD == ucRadioMode)
    {
        ptAllSibList->tSib1Info.m.tdd_ConfigPresent = 0;
    }

    if (CIM_LTE_MODE_TDD == ucRadioMode)
    {
        ptAllSibList->tSib1Info.m.tdd_ConfigPresent = 1;
        ptTddCfg = &(ptAllSibList->tSib1Info.tdd_Config);
        ptTddCfg->subframeAssignment = (TDD_Config_subframeAssignment)ptGetSib1InfoAck->tSib1.tTDD_Config.ucUlDlSlotAlloc;
        ptTddCfg->specialSubframePatterns = (TDD_Config_specialSubframePatterns)ptGetSib1InfoAck->tSib1.tTDD_Config.ucSpecSubFramePat;
    }

    /* 7. 填写SI窗口长度si-WindowLength */
    ptAllSibList->tSib1Info.si_WindowLength = (SystemInformationBlockType1_si_WindowLength)ptGetSib1InfoAck->tSib1.ucSiWindowLength;

    /* 8. 值标签systemInformationValueTag */
    ptAllSibList->tSib1Info.systemInfoValueTag = ucLastValueTag;

    /*9. R9协议升级部分开始*/
    ptAllSibList->tSib1Info.m.nonCriticalExtensionPresent = 0;
    /*设置r890协议升级部分存在*/
    ptAllSibList->tSib1Info.m.nonCriticalExtensionPresent = 1;
    /*填写紧急呼叫指示*/
    /*获取指向r9信息扩展的结构体指针*/
    ptRNineExtension = &(ptAllSibList->tSib1Info.nonCriticalExtension.nonCriticalExtension);
    /*2.初始化读取数据库的请求*/
    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    /*3初始化获得获取的结果结构体*/
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
        /*4.调用usf框架获取小区记录参数*/
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                    static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                    static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID, "\n SI: DBAccessFail_GetSrvcelRecordReq\n");
    }
    else
    {
        if (1 == tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucIMSEMCSptSwch)
        {
            /*设置r9协议升级部分存在*/
            ptAllSibList->tSib1Info.nonCriticalExtension.m.nonCriticalExtensionPresent = 1;

            /*设置携带ims支持*/
            ptRNineExtension->m.ims_EmergencySupport_r9Present = 1;
            ptRNineExtension->ims_EmergencySupport_r9 = true_5;
        }
    }
    /*填写Qqualmin 、 Qqualminoffset字段*/
    ptAllSibList->tSib1Info.nonCriticalExtension.m.nonCriticalExtensionPresent = 1;
    ptRNineExtension->m.cellSelectionInfo_v920Present = 1;
    ptRNineExtension->cellSelectionInfo_v920.m.q_QualMinOffset_r9Present = 1;
    ptRNineExtension->cellSelectionInfo_v920.q_QualMinOffset_r9 = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucQqualminoffset;
    ptRNineExtension->cellSelectionInfo_v920.q_QualMin_r9 = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucCellSelQqualMin-34;
    /*9. R9协议升级部分 结束*/
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib2Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib2Info(BYTE ucRadioMode, T_DBS_GetSib2Info_ACK *ptGetSib2InfoAck, T_AllSibList *ptAllSibList,WORD16 wCellId)
{
    SystemInformationBlockType2_ac_BarringInfo  *ptAccessBar = NULL;
    WORD32   dwSpecAcNum  = 0;
    BYTE     ucSpecAcCfg  = 0;      /* AC11~AC15配置的位图信息 */
    WORD32   dwSpecAcLoop = 0;
    WORD32   dwResult     = FALSE;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId,"\n SI: Fill SIB2 Info! \n");

    /* 1. 填写接入禁止信息accessBarringInformation，数据库不支持可选，必填  */
    ptAllSibList->tSib2Info.m.ac_BarringInfoPresent = 1;
    ptAccessBar = &(ptAllSibList->tSib2Info.ac_BarringInfo);

    /* 1.1 填写关于紧急呼叫的接入限制accessBarringForEmergencyCalls */
    if (0 == ptGetSib2InfoAck->tSib2.AcBarringInfo.ucAcBarringForEmergency)
    {
        ptAccessBar->ac_BarringForEmergency = FALSE;
    }
    else
    {
        ptAccessBar->ac_BarringForEmergency = TRUE;
    }

    if ( 1 == g_dwAcSigDebug )
    {
        ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.ucAcBarringFactor = 16;
    }
    /* 1.2 填写关于信令的接入限制accessBarringForSignalling，数据库不支持可选,必填 */
    if (16 != ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.ucAcBarringFactor )
    {
        ptAccessBar->m.ac_BarringForMO_SignallingPresent = 1;
        ptAccessBar->ac_BarringForMO_Signalling.ac_BarringFactor =
            (AC_BarringConfig_ac_BarringFactor)ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.ucAcBarringFactor;
        ptAccessBar->ac_BarringForMO_Signalling.ac_BarringTime =
            (AC_BarringConfig_ac_BarringTime)ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.ucAcBarringTime;


        dwSpecAcNum = sizeof(ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.tAcBarringForSpecialAC.aucAcBarList) / sizeof(BYTE);
        ptAccessBar->ac_BarringForMO_Signalling.ac_BarringForSpecialAC.numbits = CIM_SPEC_AC_NUM; /* AC11~AC15 */

        for (dwSpecAcLoop = CIM_SPEC_AC_NUM; dwSpecAcLoop > 0 ; dwSpecAcLoop--)
        {
            ucSpecAcCfg = ucSpecAcCfg >> 1;
            if (0 != ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOSignalling.tAcBarringForSpecialAC.aucAcBarList[dwSpecAcLoop - 1])
            {
                ucSpecAcCfg |= 0X80;
            }
        }
        ptAccessBar->ac_BarringForMO_Signalling.ac_BarringForSpecialAC.data[0] = ucSpecAcCfg;
    }
    else
    {
        ptAccessBar->m.ac_BarringForMO_SignallingPresent = 0;
    }

    if ( 1 == g_dwAcDataDebug )
    {
        ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.ucAcBarringFactor = 16;
    }
    /* 1.3 填写关于发起呼叫的接入限制accessBarringForOriginatingCalls，数据库不支持可选,必填 */
    if (16 != ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.ucAcBarringFactor )
    {
        ucSpecAcCfg = 0;
        ptAccessBar->m.ac_BarringForMO_DataPresent = 1;
        ptAccessBar->ac_BarringForMO_Data.ac_BarringFactor
        = (AC_BarringConfig_ac_BarringFactor)ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.ucAcBarringFactor;
        ptAccessBar->ac_BarringForMO_Data.ac_BarringTime
        = (AC_BarringConfig_ac_BarringTime)ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.ucAcBarringTime;

        dwSpecAcNum = sizeof(ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.tAcBarringForSpecialAC.aucAcBarList) / sizeof(BYTE);
        ptAccessBar->ac_BarringForMO_Data.ac_BarringForSpecialAC.numbits = CIM_SPEC_AC_NUM;

        for (dwSpecAcLoop = CIM_SPEC_AC_NUM; dwSpecAcLoop > 0 ; dwSpecAcLoop--)
        {
            ucSpecAcCfg = ucSpecAcCfg >> 1;
            if (0 != ptGetSib2InfoAck->tSib2.AcBarringInfo.tAcBarringForMOData.tAcBarringForSpecialAC.aucAcBarList[dwSpecAcLoop - 1])
            {
                ucSpecAcCfg |= 0X80;
            }
        }
        ptAccessBar->ac_BarringForMO_Data.ac_BarringForSpecialAC.data[0] = ucSpecAcCfg;
    }
    else
    {
        ptAccessBar->m.ac_BarringForMO_DataPresent = 0;
    }
    /* 2. 填写无线资源配置公共参数radioResourceConfigCommon */
    dwResult = CimFillSib2RadioResCfgCommon(ptGetSib2InfoAck, ptAllSibList);
    if (FALSE == dwResult)
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: Failed to fill radio resource cfg!\n");

        return FALSE;
    }
    /* 2 + 对bpl1更新pdsch信道中的rs参考信号字段为超级小区配置中cp的最大值 */
    dwResult = (WORD32)CimFillSib2ResCfgForSuperCell(ptGetSib2InfoAck, ptAllSibList,wCellId);
    if (RNLC_SUCC != dwResult)
    {
       /*如果失败，还填小区表中的值，暂定为函数不返回*/
       CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: BPL1 Failed to fill radio resource SIB2 RS cfg!\n");
    }
    /* 3. 填写ue_TimersAndConstant */
    ptAllSibList->tSib2Info.ue_TimersAndConstants.t300
    = (UE_TimersAndConstants_t300)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.uct300;
    ptAllSibList->tSib2Info.ue_TimersAndConstants.t301
    = (UE_TimersAndConstants_t301)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.uct301;
    ptAllSibList->tSib2Info.ue_TimersAndConstants.t310
    = (UE_TimersAndConstants_t310)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.uct310;
    /* {n1, n2, n3, n4, n6, n8, n10, n20} */
    ptAllSibList->tSib2Info.ue_TimersAndConstants.n310
    = (UE_TimersAndConstants_n310)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.ucn310;
    ptAllSibList->tSib2Info.ue_TimersAndConstants.t311
    = (UE_TimersAndConstants_t311)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.uct311;
    /* {n1, n2, n3, n4, n5, n6, n8, n10} */
    ptAllSibList->tSib2Info.ue_TimersAndConstants.n311
    = (UE_TimersAndConstants_n311)ptGetSib2InfoAck->tSib2.tUeTimersAndConstants.ucn311;
    ptAllSibList->tSib2Info.ue_TimersAndConstants.extElem1.numocts = 0;

    /* 4. 填写frequencyInformation */
    if (CIM_LTE_MODE_FDD == ucRadioMode)
    {
        ptAllSibList->tSib2Info.freqInfo.m.ul_CarrierFreqPresent = 1;
        ptAllSibList->tSib2Info.freqInfo.m.ul_BandwidthPresent   = 1;

        /* to do */
        ptAllSibList->tSib2Info.freqInfo.ul_CarrierFreq = (ARFCN_ValueEUTRA)ptGetSib2InfoAck->tSib2.tDbsFreqInfo.dwUlCenterCarrierFreq;
        ptAllSibList->tSib2Info.freqInfo.ul_Bandwidth   = (SystemInformationBlockType2_ul_Bandwidth)ptGetSib2InfoAck->tSib2.tDbsFreqInfo.ucUlBandwidth;
    }

    if (CIM_LTE_MODE_TDD == ucRadioMode)
    {
        ptAllSibList->tSib2Info.freqInfo.m.ul_CarrierFreqPresent = 0;
        ptAllSibList->tSib2Info.freqInfo.m.ul_BandwidthPresent   = 0;
    }

    ptAllSibList->tSib2Info.freqInfo.additionalSpectrumEmission = (AdditionalSpectrumEmission)ptGetSib2InfoAck->tSib2.tDbsFreqInfo.ucAddiSpecEmiss; /* [1,32] */

    /* 5. 填写mbsfn_SubframeConfiguration 目前不支持 */
    ptAllSibList->tSib2Info.m.mbsfn_SubframeConfigListPresent = 0;

    /* 6. 填写timeAlignmentTimerCommon */
    ptAllSibList->tSib2Info.timeAlignmentTimerCommon = (TimeAlignmentTimer)ptGetSib2InfoAck->tSib2.ucTimeAlignTimer;

    /* 7. 填写lateR8NonCriticalExtension */
    ptAllSibList->tSib2Info.m.lateNonCriticalExtensionPresent = 0;
    ptAllSibList->tSib2Info.lateNonCriticalExtension.numocts = 0;
    ptAllSibList->tSib2Info.m._v3ExtPresent = 1;

    /*R9协议升级需要添加 */
    /* 8. 填写ssac_BarringForMMTEL_Voice_r9 */
    ptAllSibList->tSib2Info.m.ssac_BarringForMMTEL_Voice_r9Present =  1;
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Voice_r9.ac_BarringFactor
    = (AC_BarringConfig_ac_BarringFactor)ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Voice.ucAcBarringFactor;

    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Voice_r9.ac_BarringTime
    = (AC_BarringConfig_ac_BarringTime)ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Voice.ucAcBarringTime;

    ucSpecAcCfg = 0;
    dwSpecAcNum = sizeof(ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Voice.tAcBarringForSpecialAC.aucAcBarList) / sizeof(BYTE);
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Voice_r9.ac_BarringForSpecialAC.numbits = CIM_SPEC_AC_NUM;
    for (dwSpecAcLoop = CIM_SPEC_AC_NUM; dwSpecAcLoop > 0 ; dwSpecAcLoop--)
    {
        ucSpecAcCfg = ucSpecAcCfg >> 1;
        if (0 != ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Voice.tAcBarringForSpecialAC.aucAcBarList[dwSpecAcLoop - 1])
        {
            ucSpecAcCfg |= 0X80;
        }
    }
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Voice_r9.ac_BarringForSpecialAC.data[0] = ucSpecAcCfg;


    /* 9. 填写ssac_BarringForMMTEL_Video_r9 */
    ptAllSibList->tSib2Info.m.ssac_BarringForMMTEL_Video_r9Present =  1;
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Video_r9.ac_BarringFactor =
        (AC_BarringConfig_ac_BarringFactor)ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Video.ucAcBarringFactor;
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Video_r9.ac_BarringTime =
        (AC_BarringConfig_ac_BarringTime)ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Video.ucAcBarringTime;

    ucSpecAcCfg = 0;
    dwSpecAcNum = sizeof(ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Video.tAcBarringForSpecialAC.aucAcBarList) / sizeof(BYTE);

    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Video_r9.ac_BarringForSpecialAC.numbits = CIM_SPEC_AC_NUM;
    for (dwSpecAcLoop = CIM_SPEC_AC_NUM; dwSpecAcLoop > 0 ; dwSpecAcLoop--)
    {
        ucSpecAcCfg = ucSpecAcCfg >> 1;
        if (0 != ptGetSib2InfoAck->tSib2.tBarringForMMTEL_Video.tAcBarringForSpecialAC.aucAcBarList[dwSpecAcLoop - 1])
        {
            ucSpecAcCfg |= 0X80;
        }
    }
    ptAllSibList->tSib2Info.ssac_BarringForMMTEL_Video_r9.ac_BarringForSpecialAC.data[0] = ucSpecAcCfg;
    /*R9协议升级需要添加 */

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib3Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib3Info(WORD16 wCellId,  BYTE ucRadioMode, T_DBS_GetSib3Info_ACK *ptGetSib3InfoAck, T_AllSibList *ptAllSibList)
{
    BYTE  ucAllowMeasBw = 0;
    SystemInformationBlockType3_cellReselectionInfoCommon        *ptCellRslComm    = NULL;
    SystemInformationBlockType3_cellReselectionServingFreqInfo   *ptCellRslSrvFreq = NULL;
    SystemInformationBlockType3_intraFreqCellReselectionInfo     *ptIntraCellRsl   = NULL;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId,"\n SI: Fill SIB3 Info!\n");

    /* 1. 填充小区重选公共参数cellReselectionInfoCommon */
    ptCellRslComm = &(ptAllSibList->tSib3Info.cellReselectionInfoCommon);
    /* 3. 填写intraFreqCellReselectionInfo */
    ptIntraCellRsl = &(ptAllSibList->tSib3Info.intraFreqCellReselectionInfo);

    /* 1.1 q_Hyst */
    ptCellRslComm->q_Hyst
    = (SystemInformationBlockType3_q_Hyst)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucQhyst;

    /* 1.2 speedDependentReselection，根据用户开关配置决定是否下发,必填 */
    ptCellRslComm->m.speedStateReselectionParsPresent = 0;
    ptIntraCellRsl->m.t_ReselectionEUTRA_SFPresent = 0;
    if (1 == ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucReselParaBaseSpeedFlag)
    {
        /*填写CellReselectionInfoCommon   基于速度的相关参数*/
        ptCellRslComm->m.speedStateReselectionParsPresent = 1;
        ptCellRslComm->speedStateReselectionPars.mobilityStateParameters.t_Evaluation
        = (MobilityStateParameters_t_Evaluation)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucTCrMax;
        ptCellRslComm->speedStateReselectionPars.mobilityStateParameters.t_HystNormal
        = (MobilityStateParameters_t_HystNormal)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucTCrMaxHyst;
        ptCellRslComm->speedStateReselectionPars.mobilityStateParameters.n_CellChangeMedium
        = ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucNCrM;
        ptCellRslComm->speedStateReselectionPars.mobilityStateParameters.n_CellChangeHigh
        = ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucNCrH;
        ptCellRslComm->speedStateReselectionPars.q_HystSF.sf_Medium
        = (SystemInformationBlockType3_sf_Medium)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucqHystSFMedium;
        ptCellRslComm->speedStateReselectionPars.q_HystSF.sf_High
        = (SystemInformationBlockType3_sf_High)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucqHystSFHigh;
        /*填写eutran下的基于速度的重选相关参数*/
        ptIntraCellRsl->m.t_ReselectionEUTRA_SFPresent = 1;
        ptIntraCellRsl->t_ReselectionEUTRA_SF.sf_Medium
        = (SpeedStateScaleFactors_sf_Medium)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctReselIntraSFM;
        ptIntraCellRsl->t_ReselectionEUTRA_SF.sf_High
        = (SpeedStateScaleFactors_sf_High)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctReselIntraSFH;         
    }

    /* 2. 填写cellReselectionServingFreqInfo */
    ptCellRslSrvFreq = &(ptAllSibList->tSib3Info.cellReselectionServingFreqInfo);

    /* [0,1] {0:false; 1:true} */
    if (0 == ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucSNintraSrchPre)
    {
        ptCellRslSrvFreq->m.s_NonIntraSearchPresent = 0;
    }
    else
    {
        ptCellRslSrvFreq->m.s_NonIntraSearchPresent = 1;
        ptCellRslSrvFreq->s_NonIntraSearch
        = ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucSNintraSrch;
    }

    ptCellRslSrvFreq->threshServingLow
    = ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucThreshSvrLow;
    ptCellRslSrvFreq->cellReselectionPriority
    = ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucIntraReselPrio;

    /* 表设计描述:D=(P+140)/2 ,默认值是-130，协议描述为[-70 -22],暂时回退代码，确认后合入*/
    ptIntraCellRsl->q_RxLevMin
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraQrxLevMin - 70;

    /* 协议: [-30,33]，Dbs=IE+30，数据库不支持可选,必填  */
    ptIntraCellRsl->m.p_MaxPresent = 1;
    ptIntraCellRsl->p_Max
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraPmax - 30;

    /* [0,1] {0:false; 1:true} */
    if (0 == ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucSIntraSrchPre)
    {
        ptIntraCellRsl->m.s_IntraSearchPresent = 0;
    }
    else
    {
        ptIntraCellRsl->m.s_IntraSearchPresent = 1;
        ptIntraCellRsl->s_IntraSearch
        = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucSIntraSrch;
    }

    /* 填充同频小区重选允许UE测量带宽 */
    ucAllowMeasBw = (BYTE)CimGetAllowMeasBwOfFreq(wCellId, ucRadioMode, ptGetSib3InfoAck);

    if (ucAllowMeasBw == ptGetSib3InfoAck->tSib3.ucDlSysBandWidth)
    {
        /* 协议:如果该信元不填写，则UE取服务小区下行系统带宽作为允许测量带宽 */
        ptIntraCellRsl->m.allowedMeasBandwidthPresent = 0;
    }
    else
    {
        ptIntraCellRsl->m.allowedMeasBandwidthPresent = 1;
        ptIntraCellRsl->allowedMeasBandwidth = (AllowedMeasBandwidth)ucAllowMeasBw;
    }

    ptIntraCellRsl->presenceAntennaPort1
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraAntPort1;

    ptIntraCellRsl->neighCellConfig.numbits = 2; /* (2, 2) */
    ptIntraCellRsl->neighCellConfig.data[0] = 0; /* 数据库缺少字段，写死为0 */

    ptIntraCellRsl->t_ReselectionEUTRA
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctRslIntraEutra;

    /*R9协议升级 添加 */

    /* 4. 填写lateR8NonCriticalExtension */
    ptAllSibList->tSib3Info.m.lateNonCriticalExtensionPresent = 0;
    ptAllSibList->tSib3Info.m._v3ExtPresent = 1;

    /* 5. 填写s_IntraSearch_v920 */
    if (0 == ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucSIntraSrchPre)
    {
        ptAllSibList->tSib3Info.m.s_IntraSearch_v920Present = 0;

    }
    else
    {
        ptAllSibList->tSib3Info.m.s_IntraSearch_v920Present = 1;
        ptAllSibList->tSib3Info.s_IntraSearch_v920.s_IntraSearchP_r9
        = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucR9SIntraSrchP;
        ptAllSibList->tSib3Info.s_IntraSearch_v920.s_IntraSearchQ_r9
        = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucSIntraSrchQ;
    }
    /* 6. 填写s_NonIntraSearch_v920 */
    if (0 == ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucSNintraSrchPre)
    {
        ptAllSibList->tSib3Info.m.s_NonIntraSearch_v920Present = 0;

    }
    else
    {

        ptAllSibList->tSib3Info.m.s_NonIntraSearch_v920Present = 1;
        ptAllSibList->tSib3Info.s_NonIntraSearch_v920.s_NonIntraSearchP_r9
        = ptGetSib3InfoAck->tSib3.tCellReselectionServingFreqInfo.ucR9SNintraSrchP;
        ptAllSibList->tSib3Info.s_NonIntraSearch_v920.s_NonIntraSearchQ_r9
        = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucSNintraSrchQ;
    }
    /* 7. 填写q_QualMin_r9 */
    ptAllSibList->tSib3Info.m.q_QualMin_r9Present = 1;
    ptAllSibList->tSib3Info.q_QualMin_r9
    = (Q_QualMin_r9)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraFreqQqualMin - 34;

    /* 8. 填写threshServingLowQ_r9  需要根据用户配置的开关决定是否下发 --算法变更*/
    ptAllSibList->tSib3Info.m.threshServingLowQ_r9Present = 0;
 
    if(1 == ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucThreshSrvLowQSwitch)
    {
        ptAllSibList->tSib3Info.m.threshServingLowQ_r9Present = 1;
        ptAllSibList->tSib3Info.threshServingLowQ_r9 = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucThreshSrvLowQ;
    }
    /*R9协议升级 添加 */
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib4Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib4Info(WORD16 wCellId, BYTE ucRadioMode, T_DBS_GetSib4Info_ACK *ptGetSib4InfoAck, T_AllSibList *ptAllSibList)
{
    BYTE    ucNtetradNum    = 0;
    BYTE    ucTetradNum     = 0;
    BYTE    ucPciRangeIndx  = 0;
    WORD16  wPciCnt         = 0;
    BYTE    ucResult        = 0; /* 获取数据库表记录返回结果 */
    WORD16  wNbrPhyCellId   = 0; /* 邻小区物理ID */
    WORD32  dwNbrDlCntrFreq = 0; /* 邻小区下行中心频率 */
    WORD16  wNbrCellNum     = 0; /* 邻区数目 */
    WORD32  dwNbrCellLoop   = 0;
    WORD32  dwEmptyItem     = 0;
    WORD32  dwRangeLoop     = 0;
    WORD32  wPciStart       = 0;
    BYTE    aucPciRange[]   = {1, 2, 3, 4, 6, 8, 12, 16, 21, 24, 32, 42, 63, 126}; /* PCI range after div by 4 */
    BYTE    aucBlkPciList[CIM_MAX_EUTRAN_PCI + 2] = {0}; /* 黑名单物理小区ID列表 */

    IntraFreqNeighCellList    *ptNeighCellList  = NULL;
    IntraFreqBlackCellList    *ptBlackCellList  = NULL;
    PhysCellIdRange           *ptPciRange       = NULL;
#if 0
    T_DBS_GetCellCfgInfo_REQ tGetCellCfgInfoReq;
    T_DBS_GetCellCfgInfo_ACK tGetCellCfgInfoAck;
#endif
    T_DBS_GetAdjCellInfoByCellId_REQ tGetAdjCellInfoByCellIdReq;
    T_DBS_GetAdjCellInfoByCellId_ACK tGetAdjCellInfoByCellIdAck;
    T_DBS_GetAllIntraFreqLTENbrCellInfo_REQ  tGetAllIntraFreqLTENbrCellInfoReq;
    T_DBS_GetAllIntraFreqLTENbrCellInfo_ACK  tGetAllIntraFreqLTENbrCellInfoAck;

    memset(aucBlkPciList, 0, sizeof(aucBlkPciList)/sizeof(BYTE));
#if 0
    memset((VOID*)&tGetCellCfgInfoReq,  0, sizeof(tGetCellCfgInfoReq));
    memset((VOID*)&tGetCellCfgInfoAck,  0, sizeof(tGetCellCfgInfoAck));
#endif
    memset((VOID*)&tGetAdjCellInfoByCellIdReq,  0, sizeof(tGetAdjCellInfoByCellIdReq));
    memset((VOID*)&tGetAdjCellInfoByCellIdAck,  0, sizeof(tGetAdjCellInfoByCellIdAck));
    memset((VOID*)&tGetAllIntraFreqLTENbrCellInfoReq,  0, sizeof(tGetAllIntraFreqLTENbrCellInfoReq));
    memset((VOID*)&tGetAllIntraFreqLTENbrCellInfoAck,  0, sizeof(tGetAllIntraFreqLTENbrCellInfoAck));

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI: Fill SIB4 Info!\n");

    tGetAllIntraFreqLTENbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllIntraFreqLTENbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllIntraFreqLTENbrCellInfo_REQ, (VOID *)&tGetAllIntraFreqLTENbrCellInfoReq, (VOID *)&tGetAllIntraFreqLTENbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllIntraFreqLTENbrCellInfoAck.dwResult))
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: SIB4 No neighbour cell! Don't send!\n");
        return FALSE;
    }

    wNbrCellNum = tGetAllIntraFreqLTENbrCellInfoAck.wIntraFreqLTENbrCellNum;
    if (0 == wNbrCellNum)
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: SIB4 Intra Freq Num :zero! Don't send!\n");

        /* 没有邻区的时候下发SIB4，协议应该是允许的 */
        return FALSE;
    }

    /* 0. 初始化SIB4内相关信元下发标志位 */
    ptAllSibList->tSib4Info.m.intraFreqNeighCellListPresent = 0;
    ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent = 0;

    /* 暂不支持 */
    ptAllSibList->tSib4Info.m.csg_PhysCellIdRangePresent = 0;
    ptAllSibList->tSib4Info.m.lateNonCriticalExtensionPresent = 0;

    /* 1. 填充邻区列表和黑名单列表 */
    ptNeighCellList = &(ptAllSibList->tSib4Info.intraFreqNeighCellList);
    ptBlackCellList = &(ptAllSibList->tSib4Info.intraFreqBlackCellList);
    ptNeighCellList->n = 0;
    ptBlackCellList->n = 0;

    /* 邻小区属性判断并记录 */
    for (dwNbrCellLoop = 0; dwNbrCellLoop < wNbrCellNum; dwNbrCellLoop++)
    {
        /*EC:罗秋鹏 如果临区设置成connect态，则广播中排除此临区信息*/
            if(1 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucStateInd)
                {
                CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI:SIB4 Cell(ID= %d) set connect state.ingnored! \n",tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId);
              continue;
                }
        /* 如果是eNB内的小区，则读取SRVCEL获取其相关参数 */
        if ((tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMcc == ptGetSib4InfoAck->tSib4.wMCC) &&
                (tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMnc == ptGetSib4InfoAck->tSib4.wMNC) &&
                (tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.dwENodeBId == ptGetSib4InfoAck->tSib4.dwEnodebID))
        {
            T_DBS_GetCellInfoByCellId_REQ tDBSGetCellInfoByCellId_REQ;
            memset(&tDBSGetCellInfoByCellId_REQ, 0, sizeof(tDBSGetCellInfoByCellId_REQ));
            tDBSGetCellInfoByCellId_REQ.wCellId = tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId;
            tDBSGetCellInfoByCellId_REQ.wCallType = USF_MSG_CALL;

            T_DBS_GetCellInfoByCellId_ACK tDBSGetCellInfoByCellId_ACK;
            memset(&tDBSGetCellInfoByCellId_ACK, 0, sizeof(tDBSGetCellInfoByCellId_ACK));

            BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetCellInfoByCellId_REQ,
                                             static_cast<VOID*>(&tDBSGetCellInfoByCellId_REQ),
                                             static_cast<VOID*>(&tDBSGetCellInfoByCellId_ACK));
            if ((!bDbResult) || (0 != tDBSGetCellInfoByCellId_ACK.dwResult))
            {
                CCM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetCellInfoByCellId_REQ,
                                    tDBSGetCellInfoByCellId_ACK.dwResult,
                                    bDbResult,
                                    RNLC_FATAL_LEVEL,
                                    " CCM_CIM_MainComponentFsm DBAccessFail_GetCellInfoByCellId_REQ! ");

                continue;
            }

            /* 获取下行中心频率和物理小区Id */
#if 0
            wNbrPhyCellId   = tGetCellCfgInfoAck.tCellCfgInfo.tCellCfgBaseInfo.wPhyCellId;
#endif
            wNbrPhyCellId = tDBSGetCellInfoByCellId_ACK.tCellAttr.tCellInfo.wPhyCellId;

            if (CIM_LTE_MODE_FDD == ucRadioMode)
            {
                dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
            }

            if (CIM_LTE_MODE_TDD == ucRadioMode)
            {
                dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
            }

        }
        else
        {
            /* 否则，其它eNB小区读取ADJCEL获取其相关参数 */
            tGetAdjCellInfoByCellIdReq.wCallType = USF_MSG_CALL;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMcc = 
                tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMcc;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMnc = 
                tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wMnc;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.dwENodeBId = 
                tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.dwENodeBId;
            tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wCellId = 
                tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId;
            ucResult = UsfDbsAccess(EV_DBS_GetAdjCellInfoByCellId_REQ, (VOID *)&tGetAdjCellInfoByCellIdReq, (VOID *)&tGetAdjCellInfoByCellIdAck);
            if ((FALSE == ucResult) || (0 != tGetAdjCellInfoByCellIdAck.dwResult))
            {
                continue;
            }

            /* 获取下行中心频率和系统带宽 */
            wNbrPhyCellId   = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.wPhyCellId;
            dwNbrDlCntrFreq = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.dwDlCenterFreq;
        }

        /* ucBlackNbrCellInd: ENUMERATE{0:false, 1:true} */
        if (0 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucBlackNbrCellInd)
        {
            /* 填写白名单列表,最多16个 */
            if (ptNeighCellList->n < 16)
            {
                dwEmptyItem = ptNeighCellList->n;
                ptNeighCellList->elem[dwEmptyItem].physCellId = wNbrPhyCellId;
                ptNeighCellList->elem[dwEmptyItem].q_OffsetCell
                = (Q_OffsetRange)tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucQOfStCell;
                ptNeighCellList->elem[dwEmptyItem].extElem1.numocts = 0;

                /* 更新邻区列表计数器 */
                ptNeighCellList->n++;
            }
        }
        else
        {
            /* 填写黑名单列表 */
            if (wNbrPhyCellId <= CIM_MAX_EUTRAN_PCI)
            {
                /* 以物理小区ID为索引，如果列入黑名单则置1 */
                aucBlkPciList[wNbrPhyCellId] = 1;
                ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent = 1;
            }
            else
            {
                CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB4 Invalid Black Neighbour PhyCellId(%lu)!\n",wNbrPhyCellId);
            }
        }

    }

    if (0 < ptNeighCellList->n)
    {
        ptAllSibList->tSib4Info.m.intraFreqNeighCellListPresent = 1;
    }
    else
    {
         CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId,"\n SI: SIB4 No White intraFreq neighbour cell!\n");

        /* 协议说明白列表是可选的，但是没有说没有白列表的时候SIB4就不下了，所以不要return */
        /* return FALSE; */
    }

    if (1 == ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent)
    {
        /* 填充黑名单列表，将相邻的PCI组合,填写range */
        wPciCnt = 0;

        aucBlkPciList[CIM_MAX_EUTRAN_PCI + 1] = 0;
        for (dwNbrCellLoop = 0; dwNbrCellLoop < (CIM_MAX_EUTRAN_PCI + 2); dwNbrCellLoop++)
        {
            if (1 == aucBlkPciList[dwNbrCellLoop])
            {
                wPciCnt++; /* 计算连续的PCI个数 */
            }
            else
            {
                if (0 == wPciCnt)
                {
                    continue;
                }

                ucNtetradNum = wPciCnt % 4;

                /* 如果连续PCI个数大于4，则肯定填写range */
                ucTetradNum = (BYTE)(wPciCnt / 4);
                wPciStart = dwNbrCellLoop - wPciCnt;
                while (ucTetradNum > 0)
                {
                    /* 寻找最接近的range */
                    ucPciRangeIndx = 0xFF;
                    for (dwRangeLoop = 0; dwRangeLoop < sizeof(aucPciRange); dwRangeLoop++)
                    {
                        if (ucTetradNum == aucPciRange[dwRangeLoop])
                        {
                            ucPciRangeIndx =  (BYTE)dwRangeLoop;
                            break;
                        }

                        if (ucTetradNum < aucPciRange[dwRangeLoop])
                        {
                            ucPciRangeIndx =  (BYTE)(dwRangeLoop - 1);
                            break;
                        }
                    }

                    if (0xFF == ucPciRangeIndx)
                    {
                        /* 没有找到合适的range，如果系统运行正常，不会发生 */
                        break;
                    }

                    /* 填写黑名单列表，不超过16项 */
                    if (ptBlackCellList->n < 16)
                    {
                        dwEmptyItem = ptBlackCellList->n;
                        ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                        ptPciRange->start = (ASN1INT)wPciStart;
                        ptPciRange->m.rangePresent = 1;
                        ptPciRange->range = (PhysCellIdRange_range)ucPciRangeIndx;

                        /* 更新黑名单列表计数器 */
                        ptBlackCellList->n++;
                    }

                    wPciStart += aucPciRange[ucPciRangeIndx] * 4;
                    ucTetradNum -= aucPciRange[ucPciRangeIndx];
                }

                if (ucTetradNum > 0)
                {
                    CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB4 Failed fill black neighbour Cell list!\n");
                    ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent = 0;
                    memset(ptBlackCellList, 0, sizeof(IntraFreqBlackCellList));
                    break;
                }

                /* 填写不能匹配range的PCI */
                for (dwRangeLoop = 0; dwRangeLoop < (WORD32)ucNtetradNum; dwRangeLoop++)
                {
                    if (ptBlackCellList->n < 16)
                    {
                        dwEmptyItem = ptBlackCellList->n;
                        ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                        ptPciRange->start = (ASN1INT)wPciStart;
                        ptPciRange->m.rangePresent = 0;

                        /* 更新黑名单列表计数器 */
                        ptBlackCellList->n++;
                    }
                    wPciStart++;
                }

                /* 重置计数器 */
                wPciCnt = 0;
            }

        }

    }

    /* csg_PhysCellIdRange 暂不支持 */

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib5Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib5Info(WORD16 wCellId, BYTE ucRadioMode, T_DBS_GetSib5Info_ACK *ptGetSib5InfoAck, T_AllSibList *ptAllSibList)
{
    BYTE    ucNtetradNum     = 0;
    BYTE    ucTetradNum      = 0;
    BYTE    ucPciRangeIndx   = 0;
    BYTE    ucMinDlBw        = 0;
    BYTE    ucNbrDlBw        = 0;
    WORD16  wPciCnt          = 0;
    BYTE    ucResult         = FALSE;
    WORD16  wNbrPhyCellId    = 0;
    WORD32  dwNbrCellNum     = 0;  /* 邻区数目 */
    WORD32  dwNbrCellNumLoop = 0;
    WORD32  dwCfLoop         = 0;
    WORD32  dwCfNum          = 0;  /* Carrier Frequency个数 */
    WORD32  dwNbrDlCntrFreq  = 0;
    WORD32  dwEmptyItem      = 0;
    WORD32  dwNbrCellLoop    = 0;
    WORD32  dwRangeLoop      = 0;
    WORD32  wPciStart        = 0;
    BYTE    aucBlkPciList[CIM_MAX_EUTRAN_PCI + 2] = {0}; /* 黑名单物理小区ID列表 */
    BYTE    aucPciRange[] = { 1, 2, 3, 4, 6, 8, 12, 16, 21, 24, 32, 42, 63, 126}; /* PCI range after div by 4 */

    SpeedStateScaleFactors            *ptSpdScal                = NULL;
    InterFreqCarrierFreqInfo          *ptFreqInfo               = NULL;
    InterFreqNeighCellList            *ptNeighCellList          = NULL;
    InterFreqBlackCellList            *ptBlackCellList          = NULL;
    PhysCellIdRange                   *ptPciRange               = NULL;

    static T_DBS_GetAdjCellInfoByCellId_REQ tGetAdjCellInfoByCellIdReq;
    static T_DBS_GetAdjCellInfoByCellId_ACK tGetAdjCellInfoByCellIdAck;
    static T_DBS_GetAllInterFreqLTENbrCellInfo_REQ  tGetAllInterFreqLTENbrCellInfoReq;
    static T_DBS_GetAllInterFreqLTENbrCellInfo_ACK  tGetAllInterFreqLTENbrCellInfoAck;

    memset(aucBlkPciList, 0, sizeof(aucBlkPciList));
    memset((VOID*)&tGetAdjCellInfoByCellIdReq,  0, sizeof(tGetAdjCellInfoByCellIdReq));
    memset((VOID*)&tGetAdjCellInfoByCellIdAck,  0, sizeof(tGetAdjCellInfoByCellIdAck));
    memset((VOID*)&tGetAllInterFreqLTENbrCellInfoReq,  0, sizeof(tGetAllInterFreqLTENbrCellInfoReq));
    memset((VOID*)&tGetAllInterFreqLTENbrCellInfoAck,  0, sizeof(tGetAllInterFreqLTENbrCellInfoAck));

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId,"\n SI: Fill SIB5 Info!\n");

    /* 由于SIB5的载频列表是必需填写的，且不能为0，所以频点信息如果没有配置的话，
    该SIB不进行下发 */
    if (0 == ptGetSib5InfoAck->tSib5.tCelSelInfo.ucInterFreqNum)
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: SIB5 Inter Freq Num Set zero! Don't send!\n");
        return FALSE;
    }
    else if (ptGetSib5InfoAck->tSib5.tCelSelInfo.ucInterFreqNum > CIM_INTER_FREQ_LIST)
    {
        ptAllSibList->tSib5Info.interFreqCarrierFreqList.n = CIM_INTER_FREQ_LIST;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB5 Number of inter frequency(%u)beyond boundary(%u)!\n",
               ptGetSib5InfoAck->tSib5.tCelSelInfo.ucInterFreqNum,
               CIM_INTER_FREQ_LIST);
    }
    else
    {
        ptAllSibList->tSib5Info.interFreqCarrierFreqList.n = ptGetSib5InfoAck->tSib5.tCelSelInfo.ucInterFreqNum;
    }
    /*获取异频临区列表信息*/ 
    tGetAllInterFreqLTENbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllInterFreqLTENbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllInterFreqLTENbrCellInfo_REQ, (VOID *)&tGetAllInterFreqLTENbrCellInfoReq, (VOID *)&tGetAllInterFreqLTENbrCellInfoAck);
    if (FALSE == ucResult)
    {
       CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB5 USF EV_DBS_GetAllInterFreqLTENbrCellInfo_REQ FAIL! \n");
    }
    /* 填充SIB5所有信息 */
    dwCfNum = ptAllSibList->tSib5Info.interFreqCarrierFreqList.n;
    for (dwCfLoop = 0; dwCfLoop < dwCfNum; dwCfLoop++)
    {
        ptFreqInfo = &(ptAllSibList->tSib5Info.interFreqCarrierFreqList.elem[dwCfLoop]);

        ptFreqInfo->m.p_MaxPresent = 1;
        ptFreqInfo->m.cellReselectionPriorityPresent = 1;
        ptFreqInfo->m.interFreqNeighCellListPresent = 0;
        ptFreqInfo->m.interFreqBlackCellListPresent = 0;

        ptFreqInfo->dl_CarrierFreq = ptGetSib5InfoAck->tSib5.tCelSelInfo.awInterCarriFreq[dwCfLoop];

        /* 协议: [-70,-22] Dbs=IE+70 */
        ptFreqInfo->q_RxLevMin = ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterQrxLevMin[dwCfLoop] - 70;

        /* 协议: [-30,33] Dbs=IE+30 */
        ptFreqInfo->p_Max = ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterPmax[dwCfLoop] - 30;
        ptFreqInfo->t_ReselectionEUTRA = ptGetSib5InfoAck->tSib5.tCelSelInfo.auctRslInterEutra[dwCfLoop];

        /*填写与速度有关的重选参数，需要根据用户选择的开关决定是否下发--算法变更*/
        ptFreqInfo->m.t_ReselectionEUTRA_SFPresent = 0;
        if (1 == ptGetSib5InfoAck->tSib5.tCelSelInfo.ucReselParaBaseSpeedFlag)
        {
            /*如果开关是打开的状态则下发相关的参数*/
            ptFreqInfo->m.t_ReselectionEUTRA_SFPresent = 1;
            ptSpdScal = &(ptFreqInfo->t_ReselectionEUTRA_SF);
            ptSpdScal->sf_Medium = (SpeedStateScaleFactors_sf_Medium)ptGetSib5InfoAck->tSib5.tCelSelInfo.auctReselInterSFM[dwCfLoop];
            ptSpdScal->sf_High = (SpeedStateScaleFactors_sf_High)ptGetSib5InfoAck->tSib5.tCelSelInfo.auctReselInterSFH[dwCfLoop];
        }
        ptFreqInfo->threshX_High = (ReselectionThreshold)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThrdXHigh[dwCfLoop];
        ptFreqInfo->threshX_Low = (ReselectionThreshold)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThrdXLow[dwCfLoop];
        ptFreqInfo->presenceAntennaPort1 = (PresenceAntennaPort1)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterAntPort1[dwCfLoop];
        ptFreqInfo->cellReselectionPriority = (CellReselectionPriority)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterReselPrio[dwCfLoop];

        ptFreqInfo->neighCellConfig.numbits = 2;    /* (2, 2) */
        ptFreqInfo->neighCellConfig.data[0] = 0;    /* 暂时写死0 */

        ptFreqInfo->q_OffsetFreq = (Q_OffsetRange)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucQOffsetFreq[dwCfLoop];

        /*添加R9协议升级需要填入的部分 */
        /*填写与rsrq相关的几个字段，根据协议这几个字段的下发与否需要与sib3中的rsrq字段一致*/
        ptFreqInfo->m._v2ExtPresent = 0;
        ptFreqInfo->m.threshX_Q_r9Present = 0;
        ptFreqInfo->m.q_QualMin_r9Present = 0;
        if(1 == ptGetSib5InfoAck->tSib5.tCelSelInfo.ucThreshSrvLowQSwitch)
        {       
            /*如果sib3中与rsrq相关的开关是打开的，则下发*/
            ptFreqInfo->m._v2ExtPresent = 1;
            ptFreqInfo->m.threshX_Q_r9Present = 1;
            ptFreqInfo->m.q_QualMin_r9Present = 1;
            /*待确定表设计中对应的变量后再填q_QualMin_r9 */
            ptFreqInfo->q_QualMin_r9 = (Q_QualMin_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterFreqQqualMin[dwCfLoop]-34;
            ptFreqInfo->threshX_Q_r9.threshX_HighQ_r9 = (ReselectionThresholdQ_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThreshXHighQ[dwCfLoop];
            ptFreqInfo->threshX_Q_r9.threshX_LowQ_r9 =  (ReselectionThresholdQ_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThreshXLowQ[dwCfLoop];
        }
        /*R协议升级增加结束 */

        /*如果没有临区列表则不再填写临区列表相关的信息，直接返回，解决没有临区列表不下发广播的问题*/
        if (0 != tGetAllInterFreqLTENbrCellInfoAck.dwResult)
        {
            CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: SIB5 Not get neighbor cell list! \n");
            continue;
        }
        /*以下全部是填写临区相关信息的内容，如果没有临区列表前面已经返回，不会执行这里*/
        /* 黑白名单长度初始为0 */
        ptFreqInfo->interFreqNeighCellList.n = 0;
        ptFreqInfo->interFreqBlackCellList.n = 0;

        /* 读取邻区记录 */
        ucMinDlBw = mbw100 + 1;

        ptNeighCellList = &(ptFreqInfo->interFreqNeighCellList);
        ptBlackCellList = &(ptFreqInfo->interFreqBlackCellList);
        dwNbrCellNum = tGetAllInterFreqLTENbrCellInfoAck.wInterFreqLTENbrCellNum;

        /* 该行代码不能删除，解决不同载频的黑表问题*/
        memset(aucBlkPciList, 0, sizeof(aucBlkPciList));

        for (dwNbrCellNumLoop = 0; dwNbrCellNumLoop < dwNbrCellNum; dwNbrCellNumLoop++)
        {
            /*EC:罗秋鹏 如果临区设置成connect态，则广播中排除此临区信息 */
            if(1 == tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].ucStateInd)
            {
                CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI:SIB5 Cell has been set only connect state,it will be ingnored! \n");
                continue;
            }
            /* 本eNB邻接小区，读取 DBTABLE_R_SRVCEL */
            if ((tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMcc == ptGetSib5InfoAck->tSib5.wMCC) &&
            (tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMnc == ptGetSib5InfoAck->tSib5.wMNC) &&
            (tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.dwENodeBId == ptGetSib5InfoAck->tSib5.dwEnodebID))
            {
                /* 根据邻区标识获取该小区的下行中心频率 */
                T_DBS_GetCellInfoByCellId_REQ tDBSGetCellInfoByCellId_REQ;
                memset(&tDBSGetCellInfoByCellId_REQ, 0, sizeof(tDBSGetCellInfoByCellId_REQ));
                tDBSGetCellInfoByCellId_REQ.wCellId = tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wCellId;
                tDBSGetCellInfoByCellId_REQ.wCallType = USF_MSG_CALL;

                T_DBS_GetCellInfoByCellId_ACK tDBSGetCellInfoByCellId_ACK;
                memset(&tDBSGetCellInfoByCellId_ACK, 0, sizeof(tDBSGetCellInfoByCellId_ACK));

                BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetCellInfoByCellId_REQ,
                         static_cast<VOID*>(&tDBSGetCellInfoByCellId_REQ),
                         static_cast<VOID*>(&tDBSGetCellInfoByCellId_ACK));
                if ((!bDbResult) || (0 != tDBSGetCellInfoByCellId_ACK.dwResult))
                {
                    CCM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetCellInfoByCellId_REQ,
                                        tDBSGetCellInfoByCellId_ACK.dwResult,
                                        bDbResult,
                                        RNLC_FATAL_LEVEL,
                                        " CCM_CIM_MainComponentFsm DBAccessFail_GetCellInfoByCellId_REQ! ");
                    continue;
                }

                /* 获取下行中心频率和物理小区Id */
#if 0
                wNbrPhyCellId   = tGetCellCfgInfoAck.tCellCfgInfo.tCellCfgBaseInfo.wPhyCellId;
#endif
                wNbrPhyCellId = tDBSGetCellInfoByCellId_ACK.tCellAttr.tCellInfo.wPhyCellId;
                if (CIM_LTE_MODE_FDD == ucRadioMode)
                {
                    dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
                    ucNbrDlBw       = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.ucDlBandwidth;;
                }
                if (CIM_LTE_MODE_TDD == ucRadioMode)
                {
                    dwNbrDlCntrFreq = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.dwDlCenterCarrierFreq;
                    ucNbrDlBw       = tDBSGetCellInfoByCellId_ACK.tCellAttr.tFreqInfo.ucDlBandwidth;;
                }

            }
            else
            {
                /* 否则，其它eNB小区读取ADJCEL获取其相关参数 */
                tGetAdjCellInfoByCellIdReq.wCallType = USF_MSG_CALL;
                tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMcc
                = tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMcc;
                tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wMnc
                = tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMnc;
                tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.dwENodeBId
                = tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.dwENodeBId;
                tGetAdjCellInfoByCellIdReq.tLTEAdjCellId.wCellId
                = tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wCellId;
                ucResult = UsfDbsAccess(EV_DBS_GetAdjCellInfoByCellId_REQ, (VOID *)&tGetAdjCellInfoByCellIdReq, (VOID *)&tGetAdjCellInfoByCellIdAck);
                if ((FALSE == ucResult) || (0 != tGetAdjCellInfoByCellIdAck.dwResult))
                {
                    continue;
                }

                /* 获取下行中心频率和物理小区Id */
                wNbrPhyCellId   = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.wPhyCellId;
                dwNbrDlCntrFreq = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.dwDlCenterFreq;
                ucNbrDlBw       = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.ucDlSysBandWidth;
            }

            /* 判断是否异频邻区，以及是否是期望的异频邻区 */
            if ((dwNbrDlCntrFreq == ptGetSib5InfoAck->tSib5.dwDlCenterFreq) ||
                (dwNbrDlCntrFreq != (WORD32)ptFreqInfo->dl_CarrierFreq))
            {
                continue;
            }

            /* 填充黑白名单 */
            /* ucBlackNbrCellInd: ENUMERATE{false:0, true:1} */
            if (0 == tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].ucBlackNbrCellInd)
            {
                if (ptNeighCellList->n < 16)
                {
                    /* 填充异频白名单邻区列表，在每个频点上最多16个邻区 */
                    ptFreqInfo->m.interFreqNeighCellListPresent = 1;
                    dwEmptyItem = ptNeighCellList->n;

                    ptNeighCellList->elem[dwEmptyItem].physCellId = wNbrPhyCellId;
                    ptNeighCellList->elem[dwEmptyItem].q_OffsetCell
                    = (Q_OffsetRange)tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].ucQOfStCell;
                    ptNeighCellList->n++;

                    if (ucNbrDlBw < ucMinDlBw)
                    {
                        ucMinDlBw = ucNbrDlBw;
                    }
                }
            }
            else
            {
                /* 填写黑名单列表 */
                if (wNbrPhyCellId <= CIM_MAX_EUTRAN_PCI)
                {
                    /* 以物理小区ID为索引，对应位置1 */
                    aucBlkPciList[wNbrPhyCellId] = 1;
                    ptFreqInfo->m.interFreqBlackCellListPresent = 1;
                }
                else
                {
                     CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB5 Invalid Black Neighbour PhyCellId(%lu)!\n",wNbrPhyCellId);
                }
            }

        } /* end读取邻区记录  */

        /* 填充测量带宽ptFreqInfo->allowedMeasBandwidth */
        if ((mbw100 + 1) == ucMinDlBw)
        {
            ptFreqInfo->allowedMeasBandwidth = mbw6;
        }
        else
        {
            ptFreqInfo->allowedMeasBandwidth = (AllowedMeasBandwidth)ucMinDlBw;
        }

        if (1 == ptFreqInfo->m.interFreqBlackCellListPresent)
        {
            /* 填充黑名单列表，将相邻的PCI组合,填写range */
            wPciCnt = 0; /* 计算连续的PCI个数 */
            aucBlkPciList[CIM_MAX_EUTRAN_PCI + 1] = 0; 
            for (dwNbrCellLoop = 0; dwNbrCellLoop < (CIM_MAX_EUTRAN_PCI + 2); dwNbrCellLoop++)
            {
                if (1 == aucBlkPciList[dwNbrCellLoop])
                {
                    wPciCnt++;
                }
                else
                {
                    if (0 == wPciCnt)
                    {
                        continue;
                    }

                    ucNtetradNum = wPciCnt % 4;
                    ucTetradNum = (BYTE)(wPciCnt / 4);
                    wPciStart = dwNbrCellLoop - wPciCnt;
                    while (ucTetradNum > 0)
                    {
                        /* 寻找最接近的range */
                        ucPciRangeIndx = 0xFF;
                        for (dwRangeLoop = 0; dwRangeLoop < sizeof(aucPciRange); dwRangeLoop++)
                        {
                            if (ucTetradNum == aucPciRange[dwRangeLoop])
                            {
                                ucPciRangeIndx =  (BYTE)dwRangeLoop;
                                break;
                            }

                            if (ucTetradNum < aucPciRange[dwRangeLoop])
                            {
                                ucPciRangeIndx =  (BYTE)(dwRangeLoop - 1);
                                break;
                            }
                        }

                        if (0XFF == ucPciRangeIndx)
                        {
                            break; /* 没有找到合适的range */
                        }

                        if (ptBlackCellList->n < 16)
                        {
                            dwEmptyItem = ptBlackCellList->n;
                            ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                            ptPciRange->start = (ASN1INT)wPciStart;
                            ptPciRange->m.rangePresent = 1;
                            ptPciRange->range = (PhysCellIdRange_range)ucPciRangeIndx;

                            ptBlackCellList->n++; /* 更新黑名单列表计数器 */
                        }

                        wPciStart += aucPciRange[ucPciRangeIndx] * 4;
                        ucTetradNum -= aucPciRange[ucPciRangeIndx];
                    }

                    if (ucTetradNum > 0)
                    {
                        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB5 Failed to fill black neighbour Cell list!\n");
                        ptFreqInfo->m.interFreqBlackCellListPresent = 0;
                        memset(ptBlackCellList, 0, sizeof(InterFreqBlackCellList));
                        break;
                    }

                    for (dwRangeLoop = 0; dwRangeLoop < (WORD32)ucNtetradNum; dwRangeLoop++)
                    {
                        if (ptBlackCellList->n < 16)
                        {
                            dwEmptyItem = ptBlackCellList->n;
                            ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                            ptPciRange->start = (ASN1INT)wPciStart;
                            ptPciRange->m.rangePresent = 0;

                            ptBlackCellList->n++; /* 更新黑名单列表计数器 */
                        }
                        wPciStart++;
                    }

                    wPciCnt = 0;
                }
            }
        }

    }
    ptAllSibList->tSib5Info.m.lateNonCriticalExtensionPresent = 0;
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib6Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib6Info(WORD16 wCellId,BYTE ucRadioMode, T_DBS_GetSib6Info_ACK *ptGetSib6InfoAck, T_AllSibList *ptAllSibList)
{
    WORD32              dwCfListLoop    = 0;      /* 用于载频列表遍历 */
    WORD32              dwCfListSize    = 0;

    CarrierFreqUTRA_FDD *ptFddCfList    = NULL;
    CarrierFreqUTRA_TDD *ptTddCfList    = NULL;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI: Fill SIB6 Info!\n");
    ucRadioMode = ucRadioMode;
    /* FDD */
    ptAllSibList->tSib6Info.m.carrierFreqListUTRA_FDDPresent = 1;

    if (0 == ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum)
    {
        /* 载频数目为0，由于载频信息为可选的，所以即使载频个数为0，也下发该SIB */
        ptAllSibList->tSib6Info.m.carrierFreqListUTRA_FDDPresent = 0;
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = 0;    /* (1,16) */
    }
    else if (ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum > CIM_FDD_CARRIER_FREQ_List)
    {
        /* 超出列表长度 */
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = CIM_FDD_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL, wCellId,"\n SI: SIB6 carrierFreqListUTRA_FDD.n=%u beyond maxmum(%u)!\n",
                ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum,
                CIM_FDD_CARRIER_FREQ_List);
    }
    else
    {
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum;
    }

    /* 填写FDD载频列表信息 */
    dwCfListSize = ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n;
    for (dwCfListLoop = 0; dwCfListLoop < dwCfListSize; dwCfListLoop++)
    {
        ptFddCfList = &(ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.elem[dwCfListLoop]);

        ptFddCfList->m.cellReselectionPriorityPresent = 1;
        ptFddCfList->carrierFreq = ptGetSib6InfoAck->tSib6.awUtranFDDCarriFreq[dwCfListLoop];
        ptFddCfList->cellReselectionPriority = ptGetSib6InfoAck->tSib6.aucUtranFDDReselPrio[dwCfListLoop];
        ptFddCfList->threshX_High = ptGetSib6InfoAck->tSib6.aucUtranFDDThrdXHigh[dwCfListLoop];
        ptFddCfList->threshX_Low = ptGetSib6InfoAck->tSib6.aucUtranFDDThrdXLow[dwCfListLoop];
        ptFddCfList->q_RxLevMin = ptGetSib6InfoAck->tSib6.aucUtranFDDQrxLevMin[dwCfListLoop] - 60;
        ptFddCfList->p_MaxUTRA = ptGetSib6InfoAck->tSib6.aucUtranFDDPmax[dwCfListLoop] - 50;
        ptFddCfList->q_QualMin = ptGetSib6InfoAck->tSib6.aucqUtranFDDQualMin[dwCfListLoop] - 24;

        /*R9协议升级填写部分*/
        /*根据sib3中rsrq字段是否下发来决定是否下发sib6中相关的rsrq字段--协议规定*/
        ptFddCfList->m.threshX_Q_r9Present = 0;
        if(1 == ptGetSib6InfoAck->tSib6.ucThreshSrvLowQSwitch)
        {
            /*如果sib3中下发的开关打开，则也下发*/
            ptFddCfList->m.threshX_Q_r9Present = 1;
            /*何建伟add20120920*/
            ptFddCfList->m._v2ExtPresent = 1;
            ptFddCfList->threshX_Q_r9.threshX_HighQ_r9 = ptGetSib6InfoAck->tSib6.aucUtranThreshXHighQFdd[dwCfListLoop];
            ptFddCfList->threshX_Q_r9.threshX_LowQ_r9 =  ptGetSib6InfoAck->tSib6.aucUtranThreshXLowQFdd[dwCfListLoop];
        }
        /*R9协议升级添加结束*/
    }

    /* TDD */
    ptAllSibList->tSib6Info.m.carrierFreqListUTRA_TDDPresent = 1;

    if (0 == ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum)
    {
        /* 载频数目为0，由于载频信息为可选的，所以即使载频个数为0，也下发该SIB */
        ptAllSibList->tSib6Info.m.carrierFreqListUTRA_TDDPresent = 0;
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = 0;    /* (1,16) */
    }
    else if (ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum > CIM_TDD_CARRIER_FREQ_List)
    {
        /* 超出列表长度 */
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = CIM_TDD_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId,"\n SI: SIB6 carrierFreqListUTRA_TDD.n=%u beyond maxmum(%u)!\n",
                ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum,
                CIM_TDD_CARRIER_FREQ_List);
    }
    else
    {
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum;
    }

    /* 填写TDD载频列表信息 */
    dwCfListSize = ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n;
    for (dwCfListLoop = 0; dwCfListLoop < dwCfListSize; dwCfListLoop++)
    {
        ptTddCfList = &(ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.elem[dwCfListLoop]);

        ptTddCfList->m.cellReselectionPriorityPresent = 1;
        ptTddCfList->carrierFreq = ptGetSib6InfoAck->tSib6.awUtranTDDCarriFreq[dwCfListLoop];
        ptTddCfList->cellReselectionPriority = ptGetSib6InfoAck->tSib6.aucUtranTDDReselPrio[dwCfListLoop];
        ptTddCfList->threshX_High = ptGetSib6InfoAck->tSib6.aucUtranTDDThrdXHigh[dwCfListLoop];
        ptTddCfList->threshX_Low = ptGetSib6InfoAck->tSib6.aucUtranTDDThrdXLow[dwCfListLoop];
        ptTddCfList->q_RxLevMin = ptGetSib6InfoAck->tSib6.aucUtranTDDQrxLevMin[dwCfListLoop] - 60;
        ptTddCfList->p_MaxUTRA = ptGetSib6InfoAck->tSib6.aucUtranTDDPmax[dwCfListLoop] - 50;
        ptTddCfList->extElem1.numocts = 0;
    }

    ptAllSibList->tSib6Info.t_ReselectionUTRA = ptGetSib6InfoAck->tSib6.ucReselUtran;

    /*填写utran基于速度的重选参数，如果开关打开则下发*/
    ptAllSibList->tSib6Info.m.t_ReselectionUTRA_SFPresent = 0;
    if(1 == ptGetSib6InfoAck->tSib6.ucReselParaBaseSpeedFlag)
    {
        ptAllSibList->tSib6Info.m.t_ReselectionUTRA_SFPresent = 1;
        ptAllSibList->tSib6Info.t_ReselectionUTRA_SF.sf_Medium
        = (SpeedStateScaleFactors_sf_Medium)ptGetSib6InfoAck->tSib6.ucReselUtranSFM;
        ptAllSibList->tSib6Info.t_ReselectionUTRA_SF.sf_High
        = (SpeedStateScaleFactors_sf_High)ptGetSib6InfoAck->tSib6.ucReselUtranSFH;
    }
    ptAllSibList->tSib6Info.m.lateNonCriticalExtensionPresent = 0;

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib7Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib7Info(WORD16 wCellId, T_DBS_GetSib7Info_ACK *ptGetSib7InfoAck, T_AllSibList *ptAllSibList)
{
    CarrierFreqsInfoGERAN                    *ptCarrierFreqsInfo       = NULL;
    ExplicitListOfARFCNs                     *ptExpListOfARFCNs        = NULL;
    CarrierFreqsGERAN_equallySpacedARFCNs    *ptEqualSpaceARFCN        = NULL;
    CarrierFreqsGERAN_variableBitMapOfARFCNs *ptVarBitMapARFCN         = NULL;
    CarrierFreqsGERAN                        *ptCarrierFreqsGERAN      = NULL;
    CarrierFreqsInfoGERAN_commonInfo         *ptCarrierFreqsInfoCommon = NULL;

    WORD32     dwNfListLoop   = 0;
    BYTE       ucResult       = 0;
    WORD32     dwExpListOfArfcnsLoop = 0;
    WORD32     dwTempLoop     = 0;

    T_DBS_GetAllGSMNbrCellInfo_REQ  tGetAllGSMNbrCellInfoReq;
    T_DBS_GetAllGSMNbrCellInfo_ACK  tGetAllGSMNbrCellInfoAck;

    memset((VOID*)&tGetAllGSMNbrCellInfoReq,  0, sizeof(tGetAllGSMNbrCellInfoReq));
    memset((VOID*)&tGetAllGSMNbrCellInfoAck,  0, sizeof(tGetAllGSMNbrCellInfoAck));

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: Fill SIB7 Info!\n");
    ptAllSibList->tSib7Info.m.carrierFreqsInfoListPresent  = 1;
    ptAllSibList->tSib7Info.m.lateNonCriticalExtensionPresent  = 0;

    ptAllSibList->tSib7Info.t_ReselectionGERAN
    = (T_Reselection)ptGetSib7InfoAck->tSib7.uctReselGeran;
    
    /*填写gsm基于速度的重选参数*/
    ptAllSibList->tSib7Info.m.t_ReselectionGERAN_SFPresent = 0;
    if (1 == ptGetSib7InfoAck->tSib7.ucReselParaBaseSpeedFlag)
    {
        /*如果开关打开则下发基于速度的重选参数*/
        ptAllSibList->tSib7Info.m.t_ReselectionGERAN_SFPresent = 1;
        ptAllSibList->tSib7Info.t_ReselectionGERAN_SF.sf_Medium
        = (SpeedStateScaleFactors_sf_Medium)ptGetSib7InfoAck->tSib7.uctReselGeranSFM;
        ptAllSibList->tSib7Info.t_ReselectionGERAN_SF.sf_High
        = (SpeedStateScaleFactors_sf_High)ptGetSib7InfoAck->tSib7.uctReselGeranSFH;
    }
    /* 读取GSMNBR */
    tGetAllGSMNbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllGSMNbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllGSMNbrCellInfo_REQ, (VOID *)&tGetAllGSMNbrCellInfoReq, (VOID *)&tGetAllGSMNbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllGSMNbrCellInfoAck.dwResult))
    {
        /*SIB6-SIB7优化，sib7的下发与临区解耦合，没有临区也会下发*/
        CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: SIB7 No neighbour cell list!\n");
    }

    if (0 == ptGetSib7InfoAck->tSib7.ucGeranFreqNum)
    {
        /* 载频数目为0，由于载频信息为可选的，所以即使载频个数为0，也下发该SIB */
        ptAllSibList->tSib7Info.m.carrierFreqsInfoListPresent = 0;
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = 0; /* (1,16) */
    }
    else if (ptGetSib7InfoAck->tSib7.ucGeranFreqNum > CIM_GERAN_CARRIER_FREQ_List)
    {
        /* 超出列表范围 */
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = CIM_GERAN_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId,"\n SI: SIB7 carrierFreqsInfoList.n=%d beyond CIM_GERAN_CARRIER_FREQ_List!\n",ptGetSib7InfoAck->tSib7.ucGeranFreqNum);
    }
    else
    {
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = ptGetSib7InfoAck->tSib7.ucGeranFreqNum;
    }

    /* 填写列表信息 */
    for (dwNfListLoop = 0; dwNfListLoop < ptGetSib7InfoAck->tSib7.ucGeranFreqNum; dwNfListLoop++)
    {
        ptCarrierFreqsInfo = &(ptAllSibList->tSib7Info.carrierFreqsInfoList.elem[dwNfListLoop]);

        /* 1. 填写CarrierFreqsGERAN */
        ptCarrierFreqsGERAN = &(ptCarrierFreqsInfo->carrierFreqs);
        ptCarrierFreqsGERAN->startingARFCN = ptGetSib7InfoAck->tSib7.awStartARFCN[dwNfListLoop];
        ptCarrierFreqsGERAN->bandIndicator = (BandIndicatorGERAN)ptGetSib7InfoAck->tSib7.aucBandIndicator[dwNfListLoop];

        /* to do:应该为数据库可配置字段，目前默认采用显示方式 */
        ptCarrierFreqsGERAN->followingARFCNs.t = CIM_EXPLICITLIST_ARFCNS;
        if (CIM_EXPLICITLIST_ARFCNS == ptCarrierFreqsGERAN->followingARFCNs.t)
        {
            ptExpListOfARFCNs = &(ptAllSibList->tSibPointStruct.tExpListOfArfcns[dwNfListLoop]);
            ptCarrierFreqsGERAN->followingARFCNs.u.explicitListOfARFCNs = ptExpListOfARFCNs;

            /* 填写剩余ARFCN，The remaining ARFCN values in the set are explicitly listed one by one.*/
            if (31< ptGetSib7InfoAck->tSib7.aucExpliARFCNNum[dwNfListLoop])
            {
                ptGetSib7InfoAck->tSib7.aucExpliARFCNNum[dwNfListLoop] = 31;
            }

            ptExpListOfARFCNs->n = ptGetSib7InfoAck->tSib7.aucExpliARFCNNum[dwNfListLoop];
            for (dwExpListOfArfcnsLoop = 0; dwExpListOfArfcnsLoop < ptExpListOfARFCNs->n; dwExpListOfArfcnsLoop++)
            {
                dwTempLoop = dwNfListLoop * 31 + dwExpListOfArfcnsLoop;
                if (dwTempLoop < MAX_NUM_GRAN_FREQ_PRE_CELL * 31)
                {
                    ptExpListOfARFCNs->elem[dwExpListOfArfcnsLoop]= ptGetSib7InfoAck->tSib7.awExplicitARFCN[dwTempLoop];
                }
            }

        }
        else if (CIM_EQUALLY_ARFCNS == ptCarrierFreqsGERAN->followingARFCNs.t)
        {
            /* The number, n, of the remaining equally spaced ARFCN values in the set.
            The complete set of (n+1) ARFCN values is defined as: {s, ((s + d) mod 1024),
            ((s + 2*d) mod 1024) ... ((s + n*d) mod 1024)}.*/
            ptEqualSpaceARFCN = &(ptAllSibList->tSibPointStruct.tEquSpacedArfcns[dwNfListLoop]);
            ptCarrierFreqsGERAN->followingARFCNs.u.equallySpacedARFCNs = ptEqualSpaceARFCN;
            ptEqualSpaceARFCN->arfcn_Spacing = ptGetSib7InfoAck->tSib7.aucArfcnSpacing[dwNfListLoop];
            ptEqualSpaceARFCN->numberOfFollowingARFCNs = ptGetSib7InfoAck->tSib7.aucFollowARFCNNum[dwNfListLoop];
        }
        else if (CIM_VARIABLEBITMAP_ARFCNS == ptCarrierFreqsGERAN->followingARFCNs.t)
        {
            /* Bitmap field representing the remaining ARFCN values in the set.
            The leading bit of the first octet in the bitmap corresponds to the
            ARFCN = ((s + 1) mod 1024), the next bit to the ARFCN = ((s + 2) mod 1024), and so on.
            If the bitmap consist of N octets, the trailing bit of octet N corresponds to ARFCN = ((s + 8*N) mod 1024). */
            ptVarBitMapARFCN = &(ptAllSibList->tSibPointStruct.tVarBitMapOfArfcns[dwNfListLoop]);
            ptCarrierFreqsGERAN->followingARFCNs.u.variableBitMapOfARFCNs = ptVarBitMapARFCN;

            /* 数据库目前没有配置字段 */
            ptVarBitMapARFCN->numocts = 0;
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: SIB7 ptCarrierFreqsGERAN->followingARFCNs.t=%d false.\n",ptCarrierFreqsGERAN->followingARFCNs.t);
            return FALSE;
        }

        /* 2. 填写CarrierFreqsInfoGERAN_commonInfo */
        ptCarrierFreqsInfoCommon = &(ptCarrierFreqsInfo->commonInfo);
        ptCarrierFreqsInfoCommon->m.cellReselectionPriorityPresent = 1;
        /*    */
        ptCarrierFreqsInfoCommon->m.p_MaxGERANPresent = 0;
        ptCarrierFreqsInfoCommon->cellReselectionPriority = ptGetSib7InfoAck->tSib7.aucGeranReselPrio[dwNfListLoop];

        /* 初始化NCC_Permitted */
        ptCarrierFreqsInfoCommon->ncc_Permitted.numbits = 8;
        ptCarrierFreqsInfoCommon->ncc_Permitted.data[0] = 0;

        /* 填写SIB7中Permitted NCC信息 */
        BYTE       ucNccPermitted = 0;
        ucNccPermitted  = ptGetSib7InfoAck->tSib7.aucNCCPermitInd[dwNfListLoop];

        ptCarrierFreqsInfoCommon->ncc_Permitted.data[0] = (ASN1OCTET)ucNccPermitted;
        ptCarrierFreqsInfoCommon->q_RxLevMin = ptGetSib7InfoAck->tSib7.aucGeranQrxLevMin[dwNfListLoop];
        //ptCarrierFreqsInfoCommon->p_MaxGERAN = ptGetSib7InfoAck->tSib7.aucGeranPmax[dwNfListLoop];
        ptCarrierFreqsInfoCommon->threshX_High = ptGetSib7InfoAck->tSib7.aucGeranThrdXHigh[dwNfListLoop];
        ptCarrierFreqsInfoCommon->threshX_Low = ptGetSib7InfoAck->tSib7.aucGeranThrdXLow[dwNfListLoop];

        ptCarrierFreqsInfo->extElem1.numocts = 0;
    }

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib8Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib8Info(WORD16 wCellId, T_DBS_GetSib8Info_ACK *ptGetSib8InfoAck, T_AllSibList *ptAllSibList)
{
    BandClassInfoCDMA2000                       *ptBandClassInfo    = NULL;
    SystemInformationBlockType8_parametersHRPD  *ptParaHrpd         = NULL;
    PreRegistrationInfoHRPD                     *ptPreRegInfo       = NULL;
    CellReselectionParametersCDMA2000           *ptCellRslPara      = NULL;
    SecondaryPreRegistrationZoneIdListHRPD      *ptZoneIdList       = NULL;
    SpeedStateScaleFactors                      *ptSpeedScal        = NULL;
    SystemInformationBlockType8_parameters1XRTT *pt1xrttPara        = NULL;
    CSFB_RegistrationParam1XRTT                 *ptCsfbRegPara      = NULL;
    SystemTimeInfoCDMA2000                      *ptSystemTimeInfo   = NULL;

    WORD32                                      dwBcListLoop        = 0;
    WORD32                                      dwHrpdBcListSize    = 0;
    WORD32                                      dwOnexrttBcListSize = 0;
    BYTE                                        ucResult       = 0;

    T_DBS_GetAllCDMANbrCellInfo_REQ tGetAllCDMANbrCellInfoReq;
    T_DBS_GetAllCDMANbrCellInfo_ACK tGetAllCDMANbrCellInfoAck;

    memset((VOID*)&tGetAllCDMANbrCellInfoReq,  0, sizeof(tGetAllCDMANbrCellInfoReq));
    memset((VOID*)&tGetAllCDMANbrCellInfoAck,  0, sizeof(tGetAllCDMANbrCellInfoAck));

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: Fill SIB8 Info!\n");

    /* 读取CDMANBR */
    tGetAllCDMANbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllCDMANbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllCDMANbrCellInfo_REQ, (VOID *)&tGetAllCDMANbrCellInfoReq, (VOID *)&tGetAllCDMANbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllCDMANbrCellInfoAck.dwResult))
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB8 No CDMA neighbour Cell!\n");
    }

    /* 1. 填写IE: systemTimeInfo */
    if(0 == g_dwDebugSysTimeAndLongCode)
    {
        ptAllSibList->tSib8Info.m.systemTimeInfoPresent   = 0;
        /*这个地方需要赋值否则，桩函数参数为0的情况就可能会出现问题*/
        g_dwSysTimeAndLongCodeExist = 0;
    }
    else
    {
        g_dwSysTimeAndLongCodeExist = 1;
        ptAllSibList->tSib8Info.m.systemTimeInfoPresent   = 1;
        ptSystemTimeInfo = &(ptAllSibList->tSib8Info.systemTimeInfo);
        if ( !g_bCcGPSLockState )
        {
            ptSystemTimeInfo->cdma_EUTRA_Synchronisation = FALSE;/*初始值设置为不同步*/
            ptSystemTimeInfo->cdma_SystemTime.t = 2;/*为了编码通过*/
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime = &g_tSystemTimeInfo_asyn;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->numbits = 49;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[0]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[1]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[2]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[3]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[4]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[5]=0xA;
            ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[6]=0xA;
        }
        else
        {
                    ptSystemTimeInfo->cdma_EUTRA_Synchronisation = TRUE;/*初始值设置为不同步*/
                    ptSystemTimeInfo->cdma_SystemTime.t = 1;
                    /*lint -save -e740 */
                    ptSystemTimeInfo->cdma_SystemTime.u.synchronousSystemTime = (SystemTimeInfoCDMA2000_synchronousSystemTime *)&g_tSystemTimeInfo_asyn;
                    /*lint -restore*/
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->numbits =39;
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[0]=0xA;
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[1]=0xA;
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[2]=0xA;
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[3]=0xA;
                    ptSystemTimeInfo->cdma_SystemTime.u.asynchronousSystemTime ->data[4]=0xA;
        }
    }

    /* 2. 填写searchWindowSize */
    ptAllSibList->tSib8Info.m.searchWindowSizePresent = 1;
    ptAllSibList->tSib8Info.searchWindowSize = ptGetSib8InfoAck->tSib8.ucSearchWinSize;

    /* 3. 填写HRPD参数 */
    ptAllSibList->tSib8Info.m.parametersHRPDPresent   = 1;
    ptParaHrpd = &(ptAllSibList->tSib8Info.parametersHRPD);

    /* 3.1 填写HRPD参数的preRegistrationInfoHRPD */
    ptPreRegInfo = &(ptParaHrpd->preRegistrationInfoHRPD);
    if (0 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucHrpdRegAllow)
    {
        ptPreRegInfo->preRegistrationAllowed = 0;
        ptPreRegInfo->m.preRegistrationZoneIdPresent = 0;
    }
    else
    {
        ptPreRegInfo->preRegistrationAllowed = 1;
        ptPreRegInfo->m.preRegistrationZoneIdPresent = 1;

        /* 协议: INT[0,255] 可选cond PreRegAllowed */
        ptPreRegInfo->preRegistrationZoneId = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.wPreRegZoneId;
    }

    /* 协议: 信元secondaryPreRegistrationZoneIdList  SIZE[1,2] 类型INT[0,255] 可选 */
    ptPreRegInfo->m.secondaryPreRegistrationZoneIdListPresent = 1;
    ptZoneIdList = &(ptPreRegInfo->secondaryPreRegistrationZoneIdList);
    ptZoneIdList->n = 2;
    ptZoneIdList->elem[0] = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.awSecPreRegZoneId[0];
    ptZoneIdList->elem[1] = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.awSecPreRegZoneId[1];

    /* 3.2 填写HRPD参数的cellReselectionParametersHRPD */
    ptParaHrpd->m.cellReselectionParametersHRPDPresent = 1;
    ptCellRslPara = &(ptParaHrpd->cellReselectionParametersHRPD);

    /* 填写bandClassList */
    if (0 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucHrpdBdClassNum)
    {
        ptParaHrpd->m.cellReselectionParametersHRPDPresent = 0;
        dwHrpdBcListSize = 0;
    }
    else if (CIM_BAND_CLASS_LIST < ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucHrpdBdClassNum)
    {
        dwHrpdBcListSize = CIM_BAND_CLASS_LIST;
        ptCellRslPara->bandClassList.n = dwHrpdBcListSize;

        CCM_SIOUT_LOG(RNLC_WARN_LEVEL, wCellId,"\n SI: bandClassList.n=%d beyond CIM_BAND_CLASS_LIST!\n",
        ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucHrpdBdClassNum);
    }
    else
    {
        dwHrpdBcListSize = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucHrpdBdClassNum;
        ptCellRslPara->bandClassList.n = dwHrpdBcListSize;
    }

    /* 填充bandClassList列表 */
    for (dwBcListLoop = 0; dwBcListLoop < dwHrpdBcListSize; dwBcListLoop++)
    {
        ptBandClassInfo = &(ptCellRslPara->bandClassList.elem[dwBcListLoop]);

        ptBandClassInfo->m.cellReselectionPriorityPresent = 1;
        ptBandClassInfo->bandClass = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.aucHrpdBandClass[dwBcListLoop];
        ptBandClassInfo->cellReselectionPriority = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.aucHrpdReselPrio[dwBcListLoop];
        ptBandClassInfo->threshX_High = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.aucHrpdThrdXHigh[dwBcListLoop];
        ptBandClassInfo->threshX_Low = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.aucHrpdThrdXLow[dwBcListLoop];
        ptBandClassInfo->extElem1.numocts = 0;
    }

    /* to do 填充邻区信息 */
    if (1 == ptParaHrpd->m.cellReselectionParametersHRPDPresent)
    {
        /* 填充CDMA HRPD 邻区信息 */
        //bResult = BcmdaFillSib8CdmaNbrList(ptRecords, 0,&(ptCellRslPara->neighCellList));
        ucResult = CimFillSib8CdmaNbrList(&tGetAllCDMANbrCellInfoAck, CDMA2000_HRPD, &(ptCellRslPara->neighCellList),
                  &(ptAllSibList->tSib8Info.cellReselectionParametersHRPD_v920.neighCellList_v920));
        if (FALSE == ucResult)
        {
            CCM_SIOUT_LOG(RNLC_WARN_LEVEL, wCellId,"\n SI: Fill HRPD neighbour cell list not complete!\n");
            ptParaHrpd->m.cellReselectionParametersHRPDPresent = 0;
            memset(ptCellRslPara, 0, sizeof(CellReselectionParametersCDMA2000));
        }
    }

    if (1 == ptParaHrpd->m.cellReselectionParametersHRPDPresent)
    {
        ptCellRslPara->t_ReselectionCDMA2000 = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.uctReselHrpd;

        /*填写cdma基于速度的重选相关参数*/   
        ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 0;
        if (1 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucReselParaBaseSpeedFlag)
        {
            /*如果开发打开，则下发基于速度的相关参数*/
            ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 1;
            ptSpeedScal = &(ptCellRslPara->t_ReselectionCDMA2000_SF);
            ptSpeedScal->sf_Medium = (SpeedStateScaleFactors_sf_Medium)ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.uctReselHrpdSFM;
            ptSpeedScal->sf_High   = (SpeedStateScaleFactors_sf_High)ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.uctReselHrpdSFH;
        }
    }

    /* 4. 填写1XRTT相关参数 */
    ptAllSibList->tSib8Info.m.parameters1XRTTPresent  = 1;
    pt1xrttPara = &(ptAllSibList->tSib8Info.parameters1XRTT);

    /* 4.1 填写CS Fall Back注册参数csfb_RegistrationParam1XRTT */
    pt1xrttPara->m.csfb_RegistrationParam1XRTTPresent = 1;
    ptCsfbRegPara = &(pt1xrttPara->csfb_RegistrationParam1XRTT);

    ptCsfbRegPara->sid.numbits                = 15; /* (15,15) */
    ptCsfbRegPara->sid.data[0]                = (BYTE)((ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttSID >> 8) & 0x00FF);
    ptCsfbRegPara->sid.data[1]                = (BYTE)(ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttSID & 0x00FF);
    ptCsfbRegPara->nid.numbits                = 16; /* (16,16) */
    ptCsfbRegPara->nid.data[0]                = (BYTE)((ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttNID >> 8) & 0x00FF);
    ptCsfbRegPara->nid.data[1]                = (BYTE)(ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttNID & 0x00FF);
    ptCsfbRegPara->multipleSID                = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttMultSID;
    ptCsfbRegPara->multipleNID                = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttMultNID;
    ptCsfbRegPara->homeReg                    = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttHomeReg;
    ptCsfbRegPara->foreignSIDReg              = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttForeSIDReg;
    ptCsfbRegPara->foreignNIDReg              = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttForeNIDReg;
    ptCsfbRegPara->parameterReg               = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttParaReg;
    ptCsfbRegPara->powerUpReg                 = ptGetSib8InfoAck->tSib8.ucPowerUpReg; 
    ptCsfbRegPara->registrationPeriod.numbits = 7;  /* (7, 7) */
    ptCsfbRegPara->registrationPeriod.data[0] = (BYTE)(ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttRegPeriod << 1);
    ptCsfbRegPara->registrationZone.numbits   = 12; /* (12, 12) */
    ptCsfbRegPara->registrationZone.data[0]   = (BYTE)((ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttRegZone >> 4) & 0x00FF);
    ptCsfbRegPara->registrationZone.data[1]   = (BYTE)((ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.w1XrttRegZone & 0x000F) << 4);
    ptCsfbRegPara->totalZone.numbits          = 3;  /* (3, 3) */
    ptCsfbRegPara->totalZone.data[0]          = (BYTE)(ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttTotalZone << 5);
    ptCsfbRegPara->zoneTimer.numbits          = 3;  /* (3, 3) */
    ptCsfbRegPara->zoneTimer.data[0]          = (BYTE)(ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttZoneTimer << 5);

    /* 4.2 填写longCodeState1XRTT */
    if((1 == ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttRegAllow)
        &&(1 == ptAllSibList->tSib8Info.m.systemTimeInfoPresent)
        &&(2 == g_dwDebugSysTimeAndLongCode))
    {
        g_dwSysTimeAndLongCodeExist = 2;
        pt1xrttPara->m.longCodeState1XRTTPresent = 1;
        pt1xrttPara->longCodeState1XRTT.numbits   = 42;
        pt1xrttPara->longCodeState1XRTT.data[0]   = 0;
    }
    else
    {
            pt1xrttPara->m.longCodeState1XRTTPresent = 0;
    }


    /* 4.3 填写1XRTT小区重选参数cellReselectionParameters1XRTT */
    pt1xrttPara->m.cellReselectionParameters1XRTTPresent = 1;
    ptCellRslPara = &(pt1xrttPara->cellReselectionParameters1XRTT);

    if (0 == ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttBdClassNum)
    {
        pt1xrttPara->m.cellReselectionParameters1XRTTPresent = 0;
        dwOnexrttBcListSize = 0;
    }
    else if (CIM_ONEXRTT_BAND_CLASS_LIST < ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttBdClassNum)
    {
        dwOnexrttBcListSize = CIM_ONEXRTT_BAND_CLASS_LIST;
        ptCellRslPara->bandClassList.n = dwOnexrttBcListSize;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB8 bandClassList.n=%d beyond CIM_ONEXRTT_BAND_CLASS_LIST!\n",
        ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttBdClassNum);
    }
    else
    {
        dwOnexrttBcListSize =  ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uc1XrttBdClassNum; /* (1, 32) (0, 31) */
        ptCellRslPara->bandClassList.n = dwOnexrttBcListSize;
    }

    for (dwBcListLoop = 0; dwBcListLoop < dwOnexrttBcListSize; dwBcListLoop++)
    {
        ptBandClassInfo = &(ptCellRslPara->bandClassList.elem[dwBcListLoop]);

        ptBandClassInfo->m.cellReselectionPriorityPresent = 1;
        ptBandClassInfo->bandClass = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.auc1XrttBandClass[dwBcListLoop];
        ptBandClassInfo->cellReselectionPriority = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.auc1XrttReselPrio[dwBcListLoop];
        ptBandClassInfo->threshX_High = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.auc1XrttThrdXHigh[dwBcListLoop];
        ptBandClassInfo->threshX_Low = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.auc1XrttThrdXLow[dwBcListLoop];
        ptBandClassInfo->extElem1.numocts = 0;
    }

    /* to do CDMA2000_NeighbourCellList */
    if (1 == pt1xrttPara->m.cellReselectionParameters1XRTTPresent)
    {
        /* 填充CDMA 1XRTT 邻区信息 */
        //bResult = BcmdaFillSib8CdmaNbrList(ptRecords, 1,&(ptCellRslPara->neighCellList));
        ucResult = CimFillSib8CdmaNbrList(&tGetAllCDMANbrCellInfoAck, CDMA2000_1XRTT,&(ptCellRslPara->neighCellList),
        &(ptAllSibList->tSib8Info.cellReselectionParameters1XRTT_v920.neighCellList_v920));
        if (FALSE == ucResult)
        {
            CCM_SIOUT_LOG(RNLC_WARN_LEVEL, wCellId,"\n SI: SIB8 Fill 1XRTT neighbour cell list not complete!\n");
            pt1xrttPara->m.cellReselectionParameters1XRTTPresent = 0;
            memset(ptCellRslPara, 0, sizeof(CellReselectionParametersCDMA2000));
        }
    }

    if (1 == pt1xrttPara->m.cellReselectionParameters1XRTTPresent)
    {
        /* 填写重选到CDMA2000 1XRTT小区判决定时器长度 */
        ptCellRslPara->t_ReselectionCDMA2000 = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1Xrtt;

        /*填写基于速度的重选参数，如果开关打开则下发，这里开关复用hrpd的*/
        ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 0; 
        if (1 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucReselParaBaseSpeedFlag)
        {
            /* 填写中\高速状态重选到CDMA2000 1XRTT小区时间比例因子 */
            ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 1; 
            ptSpeedScal = &(ptCellRslPara->t_ReselectionCDMA2000_SF);
            ptSpeedScal->sf_Medium = (SpeedStateScaleFactors_sf_Medium)ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1XrttSFM;
            ptSpeedScal->sf_High = (SpeedStateScaleFactors_sf_High)ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1XrttSFH;
        }
    }

    /* 5. 填写IE: lateR8NonCriticalExtension，暂不填写 */
    ptAllSibList->tSib8Info.m.lateNonCriticalExtensionPresent   = 0;

    /* 6. 填写IE: csfb_SupportForDualRxUEs_r9，*/
    ptAllSibList->tSib8Info.m.csfb_SupportForDualRxUEs_r9Present   = 1;
    ptAllSibList->tSib8Info.csfb_SupportForDualRxUEs_r9 =
    (ASN1BOOL)ptGetSib8InfoAck->tSib8.ucCsfbSuptDualRx;

    /* 7. 填写IE: cellReselectionParametersHRPD_v920 */
    if(0 == ptAllSibList->tSib8Info.cellReselectionParametersHRPD_v920.neighCellList_v920.n)
    {
        ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present   = 0;
    }
    else
    {
        ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present   = 1;
    }

    /* 8. 填写IE: cellReselectionParameters1XRTT_v920 */

    if(0 == ptAllSibList->tSib8Info.cellReselectionParameters1XRTT_v920.neighCellList_v920.n)
    {
        ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present   = 0;
    }
    else
    {
        ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present   = 1;
    }    

    /* 9. 填写IE: csfb_RegistrationParam1XRTT_v920 */
    if(1 == ptGetSib8InfoAck->tSib8.ucPowerDownReg_r9)
    {
        ptAllSibList->tSib8Info.csfb_RegistrationParam1XRTT_v920.powerDownReg_r9 = true_8;
        ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present   = 1;
    }
    else
    {
        ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present   = 0;
    }

    /* 10. 填写IE: ac_BarringConfig1XRTT_r9，*/
    ptAllSibList->tSib8Info.m.ac_BarringConfig1XRTT_r9Present   = 1;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring0to9_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar0to9;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring10_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar10;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring11_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar11;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring12_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar12;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring13_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar13;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring14_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar14;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_Barring15_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBar15;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_BarringMsg_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBarMsg;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_BarringReg_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBarReg;
    ptAllSibList->tSib8Info.ac_BarringConfig1XRTT_r9.ac_BarringEmg_r9 =
    ptGetSib8InfoAck->tSib8.tBarCfg1XRTT_r9.ucAcBarEmg;

    /* 11. 填写IE: __v3ExtPresent */   
    ptAllSibList->tSib8Info.m._v3ExtPresent = ptAllSibList->tSib8Info.m.csfb_SupportForDualRxUEs_r9Present
                                             |ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present
                                             |ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present
                                             |ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present
                                             |ptAllSibList->tSib8Info.m.ac_BarringConfig1XRTT_r9Present;

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFillSib9Info
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib9Info(T_DBS_GetSib9Info_ACK *ptGetSib9InfoAck, T_AllSibList *ptAllSibList)
{

    CCM_LOG(RNLC_INFO_LEVEL, "\n SI: Get SIB9 Info!aucHnbName[0]=%d\n",ptGetSib9InfoAck->tSib9.aucHnbName[0]);
    /* 暂时不实现 */
    ptAllSibList->tSib9Info.m.hnb_NamePresent = 1;
    ptAllSibList->tSib9Info.hnb_Name.numocts = 1;
    ptAllSibList->tSib9Info.hnb_Name.data[0] = 1;

    ptAllSibList->tSib9Info.m.lateNonCriticalExtensionPresent = 0;

    return TRUE;
}
/* MIB&SIBs信元填充end */

/*<FUNC>***********************************************************************
* 函数名称: CimEncodeFunc
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeFunc(BYTE ucEncodeInfoType, ASN1CTXT *ptCtxt, VOID *pMsg)
{
    int             iStat = ASN_OK;           /* 编码结果 */
    static BYTE     aucEncodeBuf[1024 * 15];  /* 广播编码缓冲区,大小为4K */

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCtxt);
    CCM_NULL_POINTER_CHECK(pMsg);

    memset(aucEncodeBuf, 0, sizeof(aucEncodeBuf));
/*清除编解码上下文*/
    ClearEncodeContext();
    iStat = pu_initContext(ptCtxt,
                           aucEncodeBuf,
                           (ASN1UINT)sizeof(aucEncodeBuf),
                           FALSE);
    if (ASN_OK != iStat)
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: Init encode context failed!\n");
        return FALSE;
    }

    iStat = (*(g_atCimSIMsgInfoTable[ucEncodeInfoType].pEncoder))(ptCtxt, pMsg);
    if (ASN_OK != iStat)
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: %s encode fail! nStat = %d\n",g_atCimSIMsgInfoTable[ucEncodeInfoType].pchMsgName, iStat);
        /* codec出错信息打印 */
        while (ptCtxt->errInfo.stkx > 0)
        {
            ptCtxt->errInfo.stkx--;
            
            CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: codec encode error. Module:%s, Line:%d, status:%d\n",
                         ptCtxt->errInfo.stack[ptCtxt->errInfo.stkx].module,
                         ptCtxt->errInfo.stack[ptCtxt->errInfo.stkx].lineno,
                         ptCtxt->errInfo.status);
        }
        pu_freeContext(ptCtxt);
        return FALSE;
    }
    pu_freeContext(ptCtxt);
    /* 编码成功 */
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimDecodeFunc
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimDecodeFunc(BYTE ucDecodeInfoType, ASN1CTXT *ptCtxt, VOID *pMsg)
{
    int    iStat = ASN_OK;   /* 解码结果标志 */

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCtxt);
    CCM_NULL_POINTER_CHECK(pMsg);

    iStat = (*(g_atCimSIMsgInfoTable[ucDecodeInfoType].pDecoder))(ptCtxt, pMsg);
    if (ASN_OK != iStat)
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: %s Decode fail! nStat = %d\n",g_atCimSIMsgInfoTable[ucDecodeInfoType].pchMsgName, iStat);
        /* codec出错信息打印 */
        while (ptCtxt->errInfo.stkx > 0)
        {
            ptCtxt->errInfo.stkx--;

            CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: codec decode error. Module:%s, Line:%d, status:%d\n",
                         ptCtxt->errInfo.stack[ptCtxt->errInfo.stkx].module,
                         ptCtxt->errInfo.stack[ptCtxt->errInfo.stkx].lineno,
                         ptCtxt->errInfo.status);
        }

        return RNLC_FAIL;
    }

    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimToMacAlignment
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimToMacAlignment(WORD16 wMsgLen)
{
    WORD32 dwTableLoop;
    WORD32 dwStartPoint;  /* TB Size表的起点 */
    WORD32 dwMidPoint;    /* TB Size表的中点 */
    WORD32 dwEndPoint;    /* TB Size表的终点 */
    const WORD16 *pwTbSizeTable = NULL;

    static WORD16 awTbSizeFor1A[] =   /* 仅适用于DCI-1A的TB Size表 */
    {
        4, 7, 9, 11, 13, 15, 18, 22, 26, 28, 32,
        37, 41, 47, 49, 55, 57, 61, 63, 69, 73, 75,
        79, 85, 87, 93, 97, 105, 113, 121, 125, 133, 141,
        145, 149, 157, 161, 173, 185, 201, 217, 225, 233, 277
    };

    static WORD16 awTbSizeFor1C[] =   /* 仅适用于DCI-1C的TB Size表 */
    {
        5, 7, 9, 15, 17, 18, 22, 26, 28, 32, 35,
        37, 41, 42, 49, 61, 69, 75, 79, 87, 97, 105,
        113, 125, 133, 141, 153, 161, 173, 185, 201, 217
    };

    static WORD16 awTbSizeFor1A1C[] =   /* 适用于DCI-1A/1C的TB Size表 */
    {
        4, 5, 7, 9, 11, 13, 15, 17, 18, 22, 26,
        28, 32, 35, 37, 41, 42, 47, 49, 55, 57, 61,
        63, 69, 73, 75, 79, 85, 87, 93, 97, 105, 113,
        121, 125, 133, 141, 145, 149, 153, 157, 161, 173, 185,
        201, 217, 225, 233, 277
    };

    /* TB Size表选择{0:仅支持DCI-1A；1:仅支持DCI-1C; 2:支持DCI-1A/1C} */
    if (0 == g_dwTbSizeMode)
    {
        pwTbSizeTable = awTbSizeFor1A;
        dwEndPoint = (sizeof(awTbSizeFor1A) / sizeof(WORD16)) - 1; /* 下标从0开始 */
    }
    else if (1 == g_dwTbSizeMode)
    {
        pwTbSizeTable = awTbSizeFor1C;
        dwEndPoint = (sizeof(awTbSizeFor1C) / sizeof(WORD16)) - 1; /* 下标从0开始 */
    }
    else
    {
        pwTbSizeTable = awTbSizeFor1A1C;
        dwEndPoint = (sizeof(awTbSizeFor1A1C) / sizeof(WORD16)) - 1; /* 下标从0开始 */
    }

    /* 如果超出TB Size表支持范围,则返回0 */
    if (pwTbSizeTable[dwEndPoint] < wMsgLen)
    {
        CCM_LOG(RNLC_WARN_LEVEL, "\n SI: Message code stream len(=%lu) beyond TB size boundary(=%lu)!\n",
                     wMsgLen, pwTbSizeTable[dwEndPoint]);
        return 0;
    }

    /* 消息长度一般处于TB Size表的中间部分，采用从中点向两边搜索的做法 */
    /* 无论何种格式的TB size表都满足[startPoint,midPoint)和(midPoint,endPoint] */
    /* 长度>=2, 因此，以下代码不会考虑三点有重叠的特殊情况 */
    dwMidPoint = dwEndPoint / 2;
    if (wMsgLen == pwTbSizeTable[dwMidPoint])
    {
        return pwTbSizeTable[dwMidPoint];
    }
    else if (wMsgLen < pwTbSizeTable[dwMidPoint])
    {
        /* 如果消息长度处于表的左半部分,则从中点向表头搜索 */
        dwStartPoint = dwMidPoint - 1;
        dwEndPoint = 0; /* 终点为表头 */
        for (dwTableLoop = dwStartPoint; dwTableLoop != dwEndPoint; dwTableLoop--)
        {
            if (wMsgLen < pwTbSizeTable[dwTableLoop])
            {
                continue;
            }
            else if (pwTbSizeTable[dwTableLoop] < wMsgLen)
            {
                return pwTbSizeTable[dwTableLoop + 1];
            }
            else
            {
                return pwTbSizeTable[dwTableLoop];
            }
        }

        /* 如果一直搜索到表头都没有发现匹配的TB SIZE,则判断表头是否合适 */
        if (wMsgLen <= pwTbSizeTable[dwEndPoint])
        {
            return pwTbSizeTable[dwEndPoint];
        }
        else
        {
            return pwTbSizeTable[dwEndPoint + 1];
        }
    }
    else
    {
        /* 如果消息长度处于表的右半部分,则从中点向表尾搜索 */
        dwStartPoint = dwMidPoint + 1; /* 起点为中点右边 */
        for (dwTableLoop = dwStartPoint; dwTableLoop <= dwEndPoint; dwTableLoop++)
        {
            if (wMsgLen <= pwTbSizeTable[dwTableLoop])
            {
                break;
            }
        }

        return pwTbSizeTable[dwTableLoop];
    }
}

/*<FUNC>***********************************************************************
* 函数名称: CimToMacAlignment
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimReEncodeSib1(T_CimSIBroadcastVar *ptCimSIBroadcastVar, T_AllSibList *ptAllSibList,
                       T_BroadcastStream  *ptNewBroadcastStream)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    ASN1CTXT  tCtxt;          /* 编解码上下文结构 */
    WORD32 dwResult = FALSE;

    /* 清空SIB1内容 */
    ptNewBroadcastStream->ucSib1Len = 0;
    memset(ptNewBroadcastStream->aucSib1Buf, 0, sizeof(ptNewBroadcastStream->aucSib1Buf));

    /* 使用更新后的ValueTage重新编码 */
    ptAllSibList->tSib1Info.systemInfoValueTag = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);

    /* 重新编码SIB1 */
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        /* 更新码流信息 */
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);

        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* 长度越界 */
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI: SIB1_LEN=%d beyond RNLC_BUF_LEN_30_SIB1.\n",ptNewBroadcastStream->ucSib1Len);
            return FALSE;
        }
    }
    else
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI: ENCODE_SIB1 fail and BcmiEncodeAllSibs return false.\n");
        return FALSE;
    }

    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimFourEncodeSib1Stream
* 功能描述: 生成sib10 、sib11、sib12包含不包含的组合场景下的四个sib1
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: word32
**    
*    
* 版    本: V3.1
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFourEncodeSib1Stream(T_CimSIBroadcastVar *ptCimSIBroadcastVar, T_AllSibList *ptAllSibList,
                       T_BroadcastStream  *ptNewBroadcastStream)
{
    SchedulingInfoList  tSchedulingInfoList;
    WORD32 wCurrentN;
    WORD32 dwResult;
    BYTE      ucSavedSiPeriod;
    tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI : Sib11 = (%d),Sib12 = (%d). Encode Four SIB1 Now...\n",
                                   ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11,ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12);
    /*既没有sib11 和sib12 的码流生成*/
    if ((0 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)&&
       (0 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting))
    {
        CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType10);
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    }
    CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType11);
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);

    CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
     
    dwResult = CimReEncodeSib1(ptCimSIBroadcastVar, ptAllSibList,ptNewBroadcastStream);
    if (FALSE == dwResult)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n Normal SIb1 Reencode Fail!.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen = ptNewBroadcastStream->ucSib1Len;
    memcpy(ptCimSIBroadcastVar->tSib1Buf[0].aucSib1Buf, ptNewBroadcastStream->aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen);

    /**只有sib11的sib1码流生成*/
    wCurrentN = tSchedulingInfoList.n;
    tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.elem[tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n] = sibType11;
    ucSavedSiPeriod =  ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod;
    /*lint -save -e685 -e568*/
    if(ucSavedSiPeriod > rf512)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n Get SIB11 Schednle Info Invilad !.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        /*如果是非法的，人为给一个默认值*/
        ucSavedSiPeriod = rf16_1;
    }
    /*lint -restore*/
    tSchedulingInfoList.elem[wCurrentN].si_Periodicity = (SchedulingInfo_si_Periodicity) ucSavedSiPeriod;
    (tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n)++;
    (tSchedulingInfoList.n)++;
    /* 更新调度信息 */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    dwResult = CimReEncodeSib1(ptCimSIBroadcastVar, ptAllSibList,ptNewBroadcastStream);
    if (FALSE == dwResult)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIb1with sib11  Reencode Fail!.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    ptCimSIBroadcastVar->tSib1Buf[1].wMsgLen = ptNewBroadcastStream->ucSib1Len;
    memcpy(ptCimSIBroadcastVar->tSib1Buf[1].aucSib1Buf, ptNewBroadcastStream->aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[1].wMsgLen);

    /**两则都有sib11和sib12的sib1码流生成*/
    wCurrentN = tSchedulingInfoList.n;
    tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.elem[tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n] = sibType12_v920;
    ucSavedSiPeriod =  ptCimSIBroadcastVar->tCimWarningInfo.ucSIB12SiPeriod;
    /*lint -save -e685 -e568*/
    if(ucSavedSiPeriod > rf512)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n Get SIB12 Schednle Info Invilad !.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        /*如果是非法的，人为给一个默认值*/
        ucSavedSiPeriod = rf16_1;
    }
    /*lint -restore*/
    tSchedulingInfoList.elem[wCurrentN].si_Periodicity = (SchedulingInfo_si_Periodicity) ucSavedSiPeriod;
    (tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n)++;
    (tSchedulingInfoList.n)++;
    /* 更新调度信息 */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    dwResult = CimReEncodeSib1(ptCimSIBroadcastVar, ptAllSibList,ptNewBroadcastStream);
    if (FALSE == dwResult)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIb1with sib11 and sib12 Reencode Fail!.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    ptCimSIBroadcastVar->tSib1Buf[3].wMsgLen = ptNewBroadcastStream->ucSib1Len;
    memcpy(ptCimSIBroadcastVar->tSib1Buf[3].aucSib1Buf, ptNewBroadcastStream->aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[3].wMsgLen);

    /*只有sib12的码流生成 先更新调度列表*/
    CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType11);
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    dwResult = CimReEncodeSib1(ptCimSIBroadcastVar, ptAllSibList,ptNewBroadcastStream);
    if (FALSE == dwResult)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SIb1with sib12 Reencode Fail!.cellId = %d! \n",
                        ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    ptCimSIBroadcastVar->tSib1Buf[2].wMsgLen = ptNewBroadcastStream->ucSib1Len;
    memcpy(ptCimSIBroadcastVar->tSib1Buf[2].aucSib1Buf, ptNewBroadcastStream->aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[2].wMsgLen);
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId, "\n SI: Four SIB1 Encode Finish!\n");
    return TRUE;
}

/*<FUNC>***********************************************************************
* 函数名称: CimToMacAlignment
* 功能描述: 4字节对齐
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeAlignment(WORD16 wLen)
{
    WORD16 wModResult;    /* 模后的结果 */

    /* 模4 余数 */
    wModResult = wLen % 4;

    if (0 == wModResult)
    {
        return wLen;
    }
    else
    {
        return((wLen - wModResult) + 4);
    }
}


/*<FUNC>***********************************************************************
* 函数名称: CimFillSib8No
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimFillSib8No(SchedulingInfoList *ptSchedulingInfoList, WORD16  *pwSib8SiNo)
{
    WORD32 dwSiLoop;    /* SI遍历 */
    WORD32 dwSibLoop;   /* 每个SI中SIB遍历 */
    WORD32 dwSibCount;  /* 每个SI中SIB总数 */
    WORD32 dwSibName;  /* SIB名 */
    BYTE   ucSiNum;    /* 当前SI */

    CCM_NULL_POINTER_CHECK_VOID(ptSchedulingInfoList);
    CCM_NULL_POINTER_CHECK_VOID(pwSib8SiNo);

    for (dwSiLoop = 0; dwSiLoop < ptSchedulingInfoList->n; dwSiLoop++)
    {
        dwSibCount = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n;
        for (dwSibLoop = 0; dwSibLoop < dwSibCount; dwSibLoop++)
        {
            dwSibName = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop];
            if (sibType8 == dwSibName)
            {
                ucSiNum = (BYTE)dwSiLoop;/*用户面的0号位置对应的是sib1,其他sib是从1号位置开始*/
                *pwSib8SiNo = ucSiNum & 0xFF;    /* 临时解决，先编译通过，后续修改 */
                return ;
            }
        }
    }

    *pwSib8SiNo = 0xffff;

    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimGetSiMaxTransNum
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSiMaxTransNum(BYTE ucSIWindowLength)
{
    /* TS36.331v850:si-WindowLength ENUM{ms1,ms2,ms5,ms10,ms15,ms20,ms40} */
    BYTE aucSiWndLen[] = {1, 2, 5, 10, 15, 20, 40};

    /* 判断入参的合法性 */
    if (sizeof(aucSiWndLen) <= ucSIWindowLength)
    {
        CCM_LOG(RNLC_WARN_LEVEL, "\n SI: Received invalid SI-WndLenIndex[=%u]!\n",ucSIWindowLength);
        /* SI在窗口内的最大传输次数取为最小值1 */
        return 1;
    }

    /* 因为冗余版本的周期为4个子帧，因此SI重传的最大次数=floor(SiWndLen/4),
       然后再加上首次传输，就为SI窗口内最大传输次数 */
    return (BYTE)(1 + (aucSiWndLen[ucSIWindowLength] / 4));
}

/*<FUNC>***********************************************************************
* 函数名称: CimDbgShowCurBroadcastInfo
* 功能描述: 显示下发的调度信息，举例
*           SI-1 = {SIB2}
*           SI-2 = {SIB3 SIB4 SIB5}
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimDbgShowCurBroadcastInfo(SchedulingInfoList *ptSchedulInfoList)
{
    INT32  iRet;
    WORD32 dwSiNum;
    WORD32 dwSibNumInSi;
    WORD32 dwSiLoop;
    WORD32 dwSibLoop;
    WORD32 dwSibType;
    BYTE   aucShowString[500];  /* 显示下发广播内容的字符串，长度限定为500字节 */
    BYTE   aucTmpString[28];    /* 格式化用的临时字符串，长度不会超过28字节 */
    BYTE   aucBroadcastInfo[14]; /* 记录各SIB是否下发，0: 否; 1: 是 */

    CCM_NULL_POINTER_CHECK_VOID(ptSchedulInfoList);
    /* 初始化相关参数 */
    dwSiNum = ptSchedulInfoList->n;
    memset((BYTE *)aucBroadcastInfo, 0, sizeof(aucBroadcastInfo));
    memset((BYTE *)aucShowString, 0, sizeof(aucShowString));

    /* 根据SIB1中的调度信息，获取下发的广播内容 */
    /*lint -save -e1773 */
    iRet = sprintf((char *)aucShowString, (char *)"The broadcasting system infomations are: \nMIB, SIB1\n");
    /*lint -restore*/
    for (dwSiLoop = 0; dwSiLoop < dwSiNum; ++dwSiLoop)
    {
        dwSibNumInSi = ptSchedulInfoList->elem[dwSiLoop].sib_MappingInfo.n;
        memset(aucBroadcastInfo, 0, sizeof(aucBroadcastInfo));

        /* 遍历当前SI的映射信息列表,找出其包含的SIB */
        for (dwSibLoop = 0 ; dwSibLoop < dwSibNumInSi; ++dwSibLoop)
        {
            dwSibType = ptSchedulInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop];
            aucBroadcastInfo[dwSibType + 3] = 1; /* 加3表示考虑到MIB,SIB1和SIB2 */
        }

        /* 考虑到SIB2的特别处理 */
        if (0 == dwSiLoop)
        {
            /* 如果是第一个SI,则将SIB2的标志位置1 */
            aucBroadcastInfo[2] = 1;
        }
        /*lint -save -e1773 */
        sprintf((char *)aucTmpString, (char *)"SI-%lu = {", dwSiLoop + 1);
        strcat((char *)aucShowString, (char *)aucTmpString);
        /*lint -restore*/
        /* 组织SI信息字符串 */
        for (dwSibLoop = 0; dwSibLoop < 14; ++dwSibLoop)
        {
            if (1 == aucBroadcastInfo[dwSibLoop])
            {
                /*lint -save -e1773 */
                iRet = sprintf((char *)aucTmpString, (char *)" SIB%lu", dwSibLoop);
                strcat((char *)aucShowString, (char *)aucTmpString);
                /*lint -restore*/
            }
        }
        /*lint -save -e1773 */
        strcat((char *)aucShowString, (char *)" }\n");
        /*lint -restore*/
    }
}

/*<FUNC>***********************************************************************
* 函数名称: CimDbgPrintMsg
* 功能描述:
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimDbgPrintMsg(BYTE *pucMsg, WORD32 dwLen)
{
    WORD32 dwLoop = 0;
    BYTE   aucMsgCode[560];
    BYTE   aucFmt[12];
    WORD32 dwCodeLen = 0;

    if ((NULL == pucMsg) || (0 == dwLen))
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: send to  msg error! NULL == pucMsg || dwLen == 0!\n");
        return;
    }

    memset(aucMsgCode, 0, sizeof(aucMsgCode));

    for (dwLoop = 0; dwLoop < dwLen; dwLoop++)
    {
        memset(aucFmt, 0, sizeof(aucFmt));
        /*lint -save -e1773 */
        sprintf((char *)aucFmt, (char *)"\t%02X", pucMsg[dwLoop]);
        strcat((char *)aucMsgCode, (char *)aucFmt);
        /*lint -restore*/
        if ((0 == (dwLoop + 1) % 12) && (0 != dwLoop))
        {
            RnlcLogTrace(PRINT_RNLC_CCM,
                         __FILE__,
                         __LINE__,
                         RNLC_INVALID_WORD,
                         RNLC_INVALID_WORD,
                         RNLC_INVALID_DWORD,
                         0,
                         0,
                         RNLC_DEBUG_LEVEL,
                         (char *)aucMsgCode);
            memset(aucMsgCode, 0, sizeof(aucMsgCode));
        }

    }

    dwCodeLen = (WORD32)strlen((char *)aucMsgCode);
    if (0 < dwCodeLen)
    {
        RnlcLogTrace(PRINT_RNLC_CCM,
                     __FILE__,
                     __LINE__,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_DWORD,
                     0,
                     0,
                     RNLC_DEBUG_LEVEL,
                     (char *)aucMsgCode);
    }
    return;
}

/*<FUNC>***********************************************************************
* 函数名称: CimDbgBroadcastStreamPrint
* 功能描述: 显示MIB,SIB1,SI码流
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimDbgBroadcastStreamPrint(T_BroadcastStream *ptBroadcastStream,
                                T_AllSiEncodeInfo *ptAllSiEncodeInfo)
{
    WORD32 dwSiNumLoop;
    WORD16 wSiLen;

    /* MIB码流打印 */
    CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: MIB stream as follow. len=%d!\n",ptBroadcastStream->ucMibLen);
    CimDbgPrintMsg(ptBroadcastStream->aucMibBuf, ptBroadcastStream->ucMibLen);

    /* SIB1码流打印 */
    CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SIB1 stream as follow. len=%d!\n",ptBroadcastStream->ucSib1Len);
    CimDbgPrintMsg(ptBroadcastStream->aucSib1Buf, ptBroadcastStream->ucSib1Len);

    /* 其它SI码流打印 */
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            /* 此SI编码失败或者不能调度，不下发 */
            CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SI-%lu not be send!\n",dwSiNumLoop + 1);
            continue;
        }

        wSiLen = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen;
        CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SI-%lu stream as follow. len=%d!\n",dwSiNumLoop + 1, wSiLen);
        CimDbgPrintMsg(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg, wSiLen);
    }
}
/*<FUNC>***********************************************************************
* 函数名称: CimDbgConfigSISch
* 功能描述: 调度信息，由于现在的R_SISCHE表为B类表，故相关测试先打桩测试
* 算法描述:
* 全局变量: 无
* Input参数:
* 返 回 值: VOID
**    
*    
* 版    本: V3.1
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimDbgConfigSISch(T_DBS_GetSib1Info_ACK *ptGetSib1InfoAck)
{
    /* SI -1 */
    if (1 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;
    }

    /* SI -1; SI -2 */
    if (2 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib3 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib4 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib5 = 1;

    }

    /* SI -1; SI -2: SI - 3 */
    if (3 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 3;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib3 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib4 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib5 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSIId = 3;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib6 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib7 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib8 = 1;
    }

    /* SI -1; SI -2: SI - 3 */
    if (4 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 3;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib3 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib4 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib5 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSIId = 3;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib6 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib7 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[2].ucSib8 = 1;
    }

    /* si - 10 */
    if (5 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib11 = 1;
    }

    if (6 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib10 = 1;

    }

    /* 将sib2和sib10放在两个SI中 */
    if (7 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib10 = 1;
    }

    /* 将sib2和sib10放在两个SI中 */
    if (8 == g_dwSiSchSwitch)
    {
        ptGetSib1InfoAck->tSib1.ucSIScheNum = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSIId = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucPeriodicity = 0;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib2 = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[0].ucSib10 = 1;

        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSIId = 2;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucPeriodicity = 1;
        ptGetSib1InfoAck->tSib1.atSchedulingInfo[1].ucSib11 = 1;
    }
}


WORD32 CimEnCodeSib11SegList(TWarningMsgSegmentList *ptWarningMsgSegmentList, T_CIMVar *ptCimData,T_CimSIBroadcastVar *ptCimSIBroadcastVar)
{
    WORD32       dwResult = FALSE;
    WORD32    dwLoop = 0;
    WORD16    wAllSegSize = 0;
    WORD16    wTempLen = 0;
    ASN1CTXT  tCtxt;          /* 编解码上下文结构 */

    CCM_NULL_POINTER_CHECK(ptWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptCimData);
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);

    /* 从第0个重复编码，因为要记录SIB码流 */
    for (dwLoop = 0; (dwLoop < ptWarningMsgSegmentList->dwSib11SegNum)
            && (dwLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwLoop++)
    {
        dwResult = CimEncodeFunc(CIM_SI_SIB11, &tCtxt, (VOID *)(&ptWarningMsgSegmentList->atSib11[dwLoop]));
        if (TRUE == dwResult)
        {

            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            if (wTempLen > (CIM_SIB11_BUF_LEN - wAllSegSize))
            {
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptCimSIBroadcastVar->wCellId,"\n SI: CimEnCodeSib11SegList SIB11 buffer overflow!\n");
                return FALSE;
            }

            if (wTempLen > CIM_MAX_CODEC_MSG_LEN)
            {
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptCimSIBroadcastVar->wCellId,"\n SI: CimEnCodeSib11SegList CIM_SIB11_BUF_LEN over Max!\n");
                return FALSE;
            }

            memmove(ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwLoop].aucMsg,
            &(tCtxt.buffer.data[0]),
            wTempLen);
            ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwLoop].wMsgLen = wTempLen;

            memmove(ptCimData->tBcmiEtwsSecondaryInfo.abySib11SegmentListStream + wAllSegSize,
            &(tCtxt.buffer.data[0]),
            wTempLen);
            wAllSegSize += wTempLen;
            ptCimData->tBcmiEtwsSecondaryInfo.dwSib11SegStreamSize[dwLoop] = wTempLen;
        }
    }
    /*记录sib11的分片数目*/
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum = ptWarningMsgSegmentList->dwSib11SegNum;

    return TRUE;
}

WORD32 CimEnCodeSib12SegList(TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList, T_CIMVar *ptCimData,T_CimSIBroadcastVar *ptCimSIBroadcastVar)
{
    WORD32       dwResult = FALSE;
    WORD32    dwLoop = 0;
    WORD16    wAllSegSize = 0;
    WORD16    wTempLen = 0;
    ASN1CTXT  tCtxt;          /* 编解码上下文结构 */

    CCM_NULL_POINTER_CHECK(ptCmasWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptCimData);
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);

    /* 从第0个重复编码，因为要记录SIB码流 */
    for (dwLoop = 0; (dwLoop < ptCmasWarningMsgSegmentList->dwSib12SegNum)
            && (dwLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwLoop++)
    {
        dwResult = CimEncodeFunc(CIM_SI_SIB12, &tCtxt, (VOID *)(&ptCmasWarningMsgSegmentList->atSib12[dwLoop]));
        if (TRUE == dwResult)
        {

            wTempLen = (WORD16)pu_getMsgLen(&tCtxt);
            if (wTempLen > (CIM_SIB12_BUF_LEN - wAllSegSize))
            {
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptCimSIBroadcastVar->wCellId,"\n SI: CimEnCodeSib12SegList SIB12 buffer overflow!\n");
                return FALSE;
            }
            if (wTempLen > CIM_MAX_CODEC_MSG_LEN)
             {
               CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptCimSIBroadcastVar->wCellId,"\n SI: CimEnCodeSib12SegList CIM_SIB12_BUF_LEN over Max!\n");
               return FALSE;
             }
            memmove(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwLoop].aucMsg,
                    &(tCtxt.buffer.data[0]),
                    wTempLen);
            ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwLoop].wMsgLen = wTempLen;
            memmove(ptCimData->tBcmiCmasWarningInfo.abySib12SegmentListStream+ wAllSegSize,
                            &(tCtxt.buffer.data[0]),
                    wTempLen);
            wAllSegSize += wTempLen;
            ptCimData->tBcmiCmasWarningInfo.dwSib12SegStreamSize[dwLoop] = wTempLen;
        }
    }
    /*记录sib12的分片数目*/
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib12SegStreamNum = ptCmasWarningMsgSegmentList->dwSib12SegNum;
    

    return TRUE;
}



/*<FUNC>***********************************************************************
* 函数名称: BcmiSetSib10SpecialTimer
* 功能描述: 设置Sib10定时器,特殊之处在于用OMC Timer进行控制
* 算法描述:
* 全局变量:
* Input参数: TBcmiCellInstData *ptBcmiCellInstData ---- [in]指向BCMI实例数据区的指针
* 输出参数: 无
* 返 回 值: BOOLEAN
**    
*    
* 版    本: V1.0
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib10SpecialTimer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId = INVALID_TIMER_ID; /* 定时器ID */
    WORD32  dwDuration = 0;
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    dwDuration = g_dwRnlcCcmDbgSib10TimerLen;
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI: (CBE OMC HYBRID MODE)CimSetSib10SpecialTimer Timer duration  =%d \n", dwDuration);

    /* 设置定时器 */
    dwTimerId = USF_OSS_SetRelativeTimer((BYTE)TIMER_CCM_CIM_S1WARNING_SIB10, (SWORD32)dwDuration,
                                         (WORD32)NULL);

    if (dwTimerId == INVALID_TIMER_ID)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwTimerId,
                        RNLC_ERROR_LEVEL, 
                        "CimSetSib10SpecialTimer fail dwTimerID=%d \n",
                        dwTimerId);
        return RNLC_FAIL;
    }
    else
    {
        //ptCimCellInstData->tBcmiEtwsPrimaryInfo.dwSib10RepeatTimer = dwTimerId;
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = dwTimerId;
        CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI: CimSetSib10SpecialTimer Success dwTimerID=%d\n", dwTimerId);
    }

    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: BcmiSetSib11Timer
* 功能描述: 设置Sib11定时器
* 算法描述:
* 全局变量:
* Input参数: TBcmiCellInstData *ptBcmiCellInstData ---- [in]指向BCMI实例数据区的指针
* 输出参数: 无
* 返 回 值: BOOLEAN
**    
*    
* 版    本: V1.0
* 修改记录:
*    
    
    *P的方式再分片的情况下最后一次可能发送不全，修改
                                                     为保证最后一个Sib11分片完成(此时有可能会大于N*P时间)
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib11Timer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId           = INVALID_TIMER_ID; /* 定时器ID */
    WORD32  dwDuration          = 0;                /* 定时器超时时间 */
    WORD32  dwSingleSib11Period = 0;                /* 一个完整SIB11的周期 */
    WORD32  dwSiBlockSchTime    = 0;                /* SIB11的调度周期 */

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    /* 计算一个Sib11整周期 */
    dwSiBlockSchTime    = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod);
    dwSingleSib11Period = dwSiBlockSchTime * ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum;

    /* 根据NumberofBroadcastRequest*RepetitionPeriod先算出应该发送多少ms */
    dwDuration = 1000 * ptCpmCcmWarningMsgReq->wRepetitionPeriod * ptCpmCcmWarningMsgReq->wNumberofBroadcastRequest;
    if ((dwDuration % dwSingleSib11Period ) != 0)
    {
        /* 这里面用于对Sib11取整周期,不用减法计算是因为不能保证dwDuration大于dwSingleSib11Period */
        dwDuration = ((dwDuration / dwSingleSib11Period) + 1) * dwSingleSib11Period;
    }

    /* 设置定时器 */
    dwTimerId = USF_OSS_SetRelativeTimer((BYTE)TIMER_CCM_CIM_S1WARNING_SIB11, (SWORD32)dwDuration,
                                         (WORD32)NULL);

    if (dwTimerId == INVALID_TIMER_ID)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwTimerId,
                        RNLC_ERROR_LEVEL, 
                        "BCM: CimSetSib11Timer fail dwTimerID=%d %s %d\n",
                        dwTimerId);
        return RNLC_FAIL;
    }
    else
    {
       // ptCimCellInstData->tBcmiEtwsSecondaryInfo.dwSib11RepeatTimer = dwTimerId;
       ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer =  dwTimerId;
        CCM_SIOUT_LOG(RNLC_INFO_LEVEL,ptCimSIBroadcastVar->wCellId,"\n SI: Sib11 duration  =%d wTimerID=%d\n", dwDuration,dwTimerId);
    }
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: CimSetSib12Timer
* 功能描述: 设置Sib11定时器
* 算法描述:
* 全局变量:
* Input参数: TBcmiCellInstData *ptBcmiCellInstData ---- [in]指向BCMI实例数据区的指针
* 输出参数: 无
* 返 回 值: BOOLEAN
**    
*    
* 版    本: V1.0
* 修改记录:
*    
    
    *P的方式再分片的情况下最后一次可能发送不全，修改
                                                     为保证最后一个Sib11分片完成(此时有可能会大于N*P时间)
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib12Timer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId           = INVALID_TIMER_ID; /* 定时器ID */
    WORD32  dwDuration          = 0;                /* 定时器超时时间 */
    WORD32  dwSingleSib12Period = 0;                /* 一个完整SIB11的周期 */
    WORD32  dwSiBlockSchTime    = 0;                /* SIB11的调度周期 */

    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    /* 计算一个Sib12整周期 */
    dwSiBlockSchTime    = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB12SiPeriod);
    dwSingleSib12Period = dwSiBlockSchTime * ptCimSIBroadcastVar->tCimWarningInfo.dwSib12SegStreamNum;

    /* 根据NumberofBroadcastRequest*RepetitionPeriod先算出应该发送多少ms */
    dwDuration = 1000 * ptCpmCcmWarningMsgReq->wRepetitionPeriod * ptCpmCcmWarningMsgReq->wNumberofBroadcastRequest;
    if ((dwDuration % dwSingleSib12Period ) != 0)
    {
        /* 这里面用于对Sib11取整周期,不用减法计算是因为不能保证dwDuration大于dwSingleSib11Period */
        dwDuration = ((dwDuration / dwSingleSib12Period) + 1) * dwSingleSib12Period;
    }

   WORD32 dwCmasFlag = 0;
   /*此处的复制操作是从高位开始的，切记*/
   memcpy(&dwCmasFlag,ptCpmCcmWarningMsgReq->tMessageIdentifier.data,2);
   /*高位存储的消息id，右移到低位*/
   dwCmasFlag = dwCmasFlag>>16;
   /*把msg id移入变量，存储在高16位*/
   memcpy(&dwCmasFlag,ptCpmCcmWarningMsgReq->tSerialNumber.data,2);

    /* 设置定时器 */
    dwTimerId = USF_OSS_SetRelativeTimer((BYTE)TIMER_CCM_CIM_S1WARNING_SIB11, (SWORD32)dwDuration,
                                         (WORD32)dwCmasFlag);

    if (dwTimerId == INVALID_TIMER_ID)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwTimerId,
                        RNLC_ERROR_LEVEL, 
                        "BCM: CimSetSib12Timer fail dwTimerID=%d %s %d\n",
                        dwTimerId);
        return RNLC_FAIL;
    }
    else
    {
        //ptCimCellInstData->tBcmiCmasWarningInfo.dwSib12RepeatTimer = dwTimerId;
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib12RepeatTimer =  dwTimerId;
        CCM_LOG(RNLC_INFO_LEVEL, "\n SI: Sib12 duration =%d TimerID=%d,Timer para=%d\n", dwDuration,dwTimerId,dwCmasFlag);
    }
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: CimSetSib11SpecialTimer
* 功能描述: 设置Sib11定时器,主要是针对周期为0，次数为1的特殊情况
* 算法描述:
* 全局变量:
* Input参数: TBcmiCellInstData *ptBcmiCellInstData ---- [in]指向BCMI实例数据区的指针
* 输出参数: 无
* 返 回 值: BOOLEAN
**    
*    
* 版    本:
* 修改记录:
*    
    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib11SpecialTimer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId        = INVALID_TIMER_ID; /* 定时器ID */
    WORD32  dwDuration       = 0;
    WORD32  dwSiBlockSchTime = 0;
    /* 入参检查 */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    dwSiBlockSchTime = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod);
    dwDuration =  dwSiBlockSchTime*(ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum);

    /* 设置定时器 */
    dwTimerId = USF_OSS_SetRelativeTimer((BYTE)TIMER_CCM_CIM_S1WARNING_SIB11, (SWORD32)dwDuration,
                                         (WORD32)NULL);

    if (dwTimerId == INVALID_TIMER_ID)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwTimerId,
                        RNLC_ERROR_LEVEL, 
                        "CimSetSib11SpecialTimer fail dwTimerID=%d\n",
                        dwTimerId);
        return RNLC_FAIL;
    }
    else
    {
        //ptCimCellInstData->tBcmiEtwsSecondaryInfo.dwSib11RepeatTimer = dwTimerId;
        ptCimSIBroadcastVar->tCimWarningInfo.dwSib11RepeatTimer = dwTimerId;
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwTimerId,
                        RNLC_ERROR_LEVEL, 
                        "\nSib11SpecialTimer dwTimerID=%d,duration(specialised)=%d\n",
                        dwTimerId,dwDuration);
    }
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: BcmGetSiPeriodMs
* 功能描述: 根据调度周期的索引获取实际的调动时间
* 算法描述: SchedulingInfo_si_Periodicity 如果该信元发生变化，那么此函数需要修改
* 全局变量:
* Input参数: BYTE byIndex ---- 索引
* 返 回 值: WORD32      时间单位为ms
**    
*    
* 版    本: V1.0
* 修改记录:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSiPeriodMs(WORD32 dwSiPeriodIndex)
{
    WORD32 dwReturnTime          = 80;                                    /* SIPERIOD_80 默认取值 */
    WORD32 adwS1PeriodMsTable[7] = {80, 160, 320, 640, 1280, 2560, 5120}; /* according to 36.331 930 6.2.2 */

    if (dwSiPeriodIndex >= (sizeof(adwS1PeriodMsTable)/sizeof(WORD32)))
    {
        /* 字段取值有误，打印告警 */
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwSiPeriodIndex,
                        RNLC_ERROR_LEVEL, 
                        "tSiPeriodIndex=%lu not enumerate(rf8，rf16，rf32，rf64, rf128, rf256, rf512)",
                        dwSiPeriodIndex);
    }
    else
    {
        dwReturnTime = adwS1PeriodMsTable[dwSiPeriodIndex];
    }
    return dwReturnTime;
}
