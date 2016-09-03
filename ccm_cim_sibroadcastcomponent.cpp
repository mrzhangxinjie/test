/*<FH>*************************************************************************
* ��Ȩ���� (C)2013 ����������ͨ
* ģ������: CCM
* �ļ�����: ccm_cim_sibroadcastcomponent.cpp
* ����ժҪ: ���ļ���Ҫʵ�ֹ㲥�����ú͸��£�ETWS��CMAS�ȸ澯
* ����˵��:
* ��ǰ�汾: V3.1
**    
*    
* �޸ļ�¼1:
*    
**<FH>************************************************************************/

/*****************************************************************************/
/*               #include������Ϊ��׼��ͷ�ļ����Ǳ�׼��ͷ�ļ���              */
/*****************************************************************************/
/* ================================== OSSͷ�ļ� ============================*/
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


/* ============================= ������ϵͳ����ͷ�ļ� ======================*/
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

/* =============================RNLC�ڸ���ģ�鹫��ͷ�ļ� ===================*/


/* ==============================CCMģ�鹫��ͷ�ļ�==========================*/
#include "ccm_common.h"
#include "ccm_cmm_struct.h"
#include "ccm_timer.h"
#include "ccm_error.h"
#include "ccm_cim_common.h"
#include "ccm_eventdef.h"
#include "ccm_debug.h"
#include "ccm_print.h"

/* ============================CIMģ�鹫��ͷ�ļ�=========================== */
#include "ccm_cim_sibroadcastcomponent.h"

/* TB Size��ѡ��{0:��֧��DCI-1A��1:��֧��DCI-1C; 2:֧��DCI-1A/1C} */
extern WORD32 g_dwTbSizeMode;

/* ��ȫ�ֱ������������Ƶ��Ե�ʱ���Ƿ�����ⲿϵͳ�ı�־ */
extern T_SystemDbgSwitch g_tSystemDbgSwitch;

/* ��ӡ��Ϣ���أ�1:��*/
extern WORD32 g_dwCimPrintFlag;
 /*��ʾsib11�ǰ���Э��ķ�ʽ����(0)�����ձ���Ҫ��ʽ����(1)*/
extern BYTE  g_ucSib11NonSuspend; 
 /*��ʾ�û����յ�sib11ʱ����������(1) ���ǰ����޸����ڷ���(0)*/
extern BYTE  g_ucSib11WarnMsgSendType;

extern BYTE g_ucSceneCfg;//���߸���ģʽ��ʶ:0��������1�����ߣ�2������
 
WORD32  g_dwRnlcCcmDbgSib10TimerLen = 6000;

WORD32 g_dwAcSigDebug = 0;
WORD32 g_dwAcDataDebug = 0;

WORD32 g_dwSib10SwtichMode = CIM_SIB10_STOPCBEANDTIMER;/* sib10��ֹͣ��ʽ */

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
* ��������: Idle
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Idle(Message *pMsg)
{
    /* ��μ�� */
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
            /*�ز��ۺϹ��ܣ�����㲥������Ҫ֪ͨdcm*/
            if(EV_CMM_CIM_SYSINFO_UPDATE_REQ == pMsg->m_dwSignal)
            {
                /* ��ȡ�㲥���������� */
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
* ��������: WaitSystemInfoUpdateRsp
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::WaitSystemInfoUpdateRsp(Message *pMsg)
{
    /* ��μ�� */
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
* ��������: Init
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
* ��������: CimSendToDcmBrdcastMsgUpdInd
* ��������: �ز��ۺϹ���������ͨ�㲥����ʱ��dcm���͸���ָʾ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
    {
        // ������sib11��ȴû�ж�ȡ�����ݲ���Ҫ����SI �б�
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
* ��������: CimCauseEtwsSib10Add
* ��������: a.��ȡ���ݿ���Ϣ��MIB��SIB1�Լ�����SIB�ֱ����:��ɲ�����ȡ��ӳ���ϵ����
*           b.����ӳ���ϵ����дSI:��д�����SI����
*           c.SI����:��������SI������һ��SI(����SIB2)����ʧ�ܣ�ֹͣ�㲥�·�;
*            ������SI����ɹ���ʧ�ܣ����ϱ�ǣ����tAllSiEncodeInfo��Ϣ��д
*           d.�ȽϹ㲥�����Ƿ����:��֮ǰ�·��Ĺ㲥�Ƚϣ����㲥��Ϣ���䣬�����·�
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])
    {
        // ������sib10��ȴû�ж�ȡ�����ݲ���Ҫ����SI �б�
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
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
     CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    
    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11=1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    //ӳ��Ϊ���û���ķ��͵Ĺ㲥���µ�ԭ��
    //ptsysInfoUpdReq->ucCellSIUpdCause = CAUSE_ETWS_START;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB11])
    {
        // ������sib10��ȴû�ж�ȡ�����ݲ���Ҫ����SI �б�
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
* ��������: CimCauseEtwsSib10Sib11Add
* ��������: ��epc�澯����sib10��sib11�����ҵ�����Ϣ�����õ�����µĴ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Sib11Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )

{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11=1;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS SIB10 SIB11 Send!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimCauseEtwsSib10Stop
* ��������: ETWS �յ�sib10 stop����Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   //���ñ�־λ��������������Ƿ�ֹͣsib10
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 =0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;

    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS Receive SIB10 Stop!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimCauseEtwsSib10SIB11Stop
* ��������: ETWS �յ�sib10 sib11 stop����Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseEtwsSib10SIB11Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   //���ñ�־λ��������������Ƿ�ֹͣsib10
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;

    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: ETWS Receive SIB10 SIB11 Stop!");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimCauseCmasSib12Add
* ��������: CMAS �յ�sib12����Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Add(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
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
    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12=1;   
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
    {
        // ������sib12��ȴû�ж�ȡ�����ݲ���Ҫ����SI �б�
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
* ��������: CimCauseCmasSib12Upd
* ��������: CMAS �յ�sib12���µ���Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Upd(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);

    SchedulingInfoList tSchedulingInfoList;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12=1; 
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    if (FALSE == ptAllSibList->aucSibList[DBS_SI_SIB12])
    {
        // ������sib12��ȴû�ж�ȡ�����ݲ���Ҫ����SI �б�
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
* ��������: CimCauseCmasSib12Kill
* ��������: CMAS �յ�sib12kill����Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Kill(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimCauseCmasSib12Stop
* ��������: CMAS �յ�sib12ֹͣ����Ϣ�Ĵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/

WORD32 CCM_CIM_SIBroadcastComponentFsm::CimCauseCmasSib12Stop(T_AllSibList *ptAllSibList,T_SysInfoUpdReq  *ptsysInfoUpdReq )
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    //���ñ�־λ��������������Ƿ��·� sib10 sib11 sib12
    ptsysInfoUpdReq->wCellId = ptCimSIBroadcastVar->wCellId;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* ��������: CimDealWithWarningConditions
* ��������: ����s1�ڹ����ĸ澯������Ϣ���������Ӧ��صĴ�����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimDealWithWarningConditions( T_AllSibList  *ptAllSibList, T_SysInfoUpdReq  *ptsysInfoUpdReq,
                                                                     TWarningMsgSegmentList *ptWarningMsgSegmentList,
                                                                     TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptsysInfoUpdReq);
    WORD32 dwSIIndex = RNLC_FAIL;

    /*���ñ���dwCheckWarnSchedInfoResult ����0 ��ʾ�����ķ��أ�
    ���������0����֪ͨ�㲥��������ֱ�ӷ���*/
    WORD32 dwCheckWarnSchedInfoResult = 0;

    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)(&ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg);

    switch(ptsysInfoUpdReq->ucCellSIUpdCause)
    {
           
    case CIM_CAUSE_ETWS_SIB10_ADD: 
    {
        if(FALSE == ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /*1. sib10 �澯���٣�����û��sib10�ĵ�����Ϣ*/
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
            /*2. sib11 �澯���٣�����û��sib11�ĵ�����Ϣ*/
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
            /*3. sib10 sib11 �澯���٣�����û��sib10 sib11�ĵ�����Ϣ*/
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
            /*sib10��sib11���У����Ҷ�����Ӧ�ĵ�����Ϣ*/
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
            /*4. sib12 �澯���٣�����û��sib12�ĵ�����Ϣ*/
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
            /*4. sib12 �澯���£�����û��sib12�ĵ�����Ϣ*/
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
       /*���Ѷ��峡����ʲô������*/
        break;
    }

    }

    /*����˴θ澯��etws��أ����ȡsib11��si���ڱ��浽si��������*/
    if((CIM_CAUSE_ETWS_SIB10_ADD <= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    &&(CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD >= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
        {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Deal SIB10 info Now...\n");
                 /*ע���������õĴ˴�s1�ڹ����ĸ澯��ɵ�sib10��Ϣ*/
            CimWarnMsgSib10Hander(ptCpmCcmWarnMsg,ptWarningMsgSegmentList,ptsysInfoUpdReq,ptAllSibList);
        }
            /*����˴θ澯����sib11������Ҫ���ͣ�����дsib11��Ϣ*/
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
    /*����˴θ澯��cmas��أ����ȡsib12��si���ڱ��浽si��������*/
    else if((CIM_CAUSE_CMAS_SIB12_ADD <= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
    &&(CIM_CAUSE_CMAS_SIB12_STOP >= ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        /*����˴θ澯����sib12������Ҫ���ͣ�����дsib12��Ϣ*/
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
* ��������: Handle_SystemInfoUpdateReq
* ��������: a.��ȡ���ݿ���Ϣ��MIB��SIB1�Լ�����SIB�ֱ����:��ɲ�����ȡ��ӳ���ϵ����
*           b.����ӳ���ϵ����дSI:��д�����SI����
*           c.SI����:��������SI������һ��SI(����SIB2)����ʧ�ܣ�ֹͣ�㲥�·�;
*            ������SI����ɹ���ʧ�ܣ����ϱ�ǣ����tAllSiEncodeInfo��Ϣ��д
*           d.�ȽϹ㲥�����Ƿ����:��֮ǰ�·��Ĺ㲥�Ƚϣ����㲥��Ϣ���䣬�����·�
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    memset((VOID*)&tAllSibList, 0, sizeof(tAllSibList));
    memset((VOID*)&tNewBroadcastStream, 0, sizeof(tNewBroadcastStream));
    memset((VOID*)&tAllSiEncodeInfo, 0, sizeof(tAllSiEncodeInfo));
    memset((VOID*)&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));
    memset(&tWarningMsgSegmentList, 0, sizeof(tWarningMsgSegmentList));
    memset(&tCmasWarningMsgSegmentList, 0, sizeof(tCmasWarningMsgSegmentList));

    CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\nSI: New Version! \n");
    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    ucRadioMode = ptCIMVar->ucRadioMode;
     /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);    
    ptsysInfoUpdReq = (T_SysInfoUpdReq *)pMsg->m_pSignalPara;
    tAllSibList.ucResv = ptsysInfoUpdReq->ucCellSIUpdCause;
    /*��¼�´˴ι㲥���µ�ԭ�򵽹㲥����������*/
      ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause = ptsysInfoUpdReq->ucCellSIUpdCause;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Receive SI Update Cause(%d): %s",ptsysInfoUpdReq->ucCellSIUpdCause,CIMSIUPDATECAUSE(ptsysInfoUpdReq->ucCellSIUpdCause));
    /* ��ʼ���㲥ʵ�������� */
    CimBroadcastDataInit(ptsysInfoUpdReq);

    wCellId = ptsysInfoUpdReq->wCellId;

    /*��֮ǰ��valuetagȡ���������С����������ȡ������Ϊ0��������ȡ��ʷֵ*/
    BYTE ucLastValueTag = GetComponentContext()->tCimSIBroadcastVar.tSIBroadcastDate.ucSib1ValueTag;
    /*�˴����ӹ㲥����Ϊ��0���жϣ�������Ϊdbs�����������µ�С��ɾ��
    �������С���������µ�ɾ��������ԭ��������ִ�С��ȷ�����㲥���ٷ����¹㲥*/
    if((SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)&& (0 != ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength))
    {
        /*����޸Ĳ��������ɾ�����⴦��*/
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
    /*���û��֮ǰ�Ĺ㲥������ֱ�Ӹ���Ϊһ��С�������Ĺ㲥*/
    if((SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)&& (0 == ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength))
    {   
        ptsysInfoUpdReq->ucCellSIUpdCause = SI_UPD_CELL_SETUP;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI_UPD_CELL_SETUP result to SI_UPD_CELL_DELADD!\n");
    }
    /* 1.��ȡ���ݿ���Ϣ */
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
    /*����㲥������epc�ĸ澯���*/
    if((ptsysInfoUpdReq->ucCellSIUpdCause >= CIM_CAUSE_ETWS_SIB10_ADD)
    &&(ptsysInfoUpdReq->ucCellSIUpdCause <= CIM_CAUSE_CMAS_SIB12_STOP))
    {
        /*���s1 ��ͬ�澯������в�ͬ�Ĵ������û��s1�澯��᷵��0*/ 
        dwResult = CimDealWithWarningConditions(&tAllSibList,ptsysInfoUpdReq,&tWarningMsgSegmentList,&tCmasWarningMsgSegmentList);

        if (0 != dwResult)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: No EPC Warn Msg schedle info, SI update Stop! Situation = %d\n",dwResult);
            memset(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg, 0, sizeof(ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg));
            return ;
        }
    }
    /*��ͨ�㲥����ʱ���Ƿ���дsib10 ��Ϣ�Ĵ���*/
    CimNmlBrdUpdDealWithSib10Info(&tAllSibList,ptsysInfoUpdReq);
    
    /* 2.MIB��SIB1-sib9 �������s1�澯 ��sib10 �� ��ͨsib��һ����룬���sib11��sib12 ���Է�Ƭ���б��� */
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

    /* 3.����ӳ���ϵ����дSI */
    dwResult = CimSibMappingToSi(&tAllSibList,
                            &tAllSiEncodeInfo);
    if (FALSE == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CIM SIB Mapping to SI fail!\n");
#ifndef VS2008
        return;
#endif
    }

    /* 4.SI����   �������s1�澯��ͬʱҲ���s1�澯��si ���б��� */
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

    /* 5.�ȽϹ㲥�����Ƿ���£��������s1�澯���������ֱ�ӷ��� */
    dwResult = CimSibsCheckAndUpdte(ptsysInfoUpdReq, &tAllSibList,&tNewBroadcastStream);
#ifdef VS2008
    dwResult = TRUE;
#endif
    /*����㲥û�и��£���������������㲥*/
        if (FALSE == dwResult)
        {
            CimSibsDealWithNoUpdate(ptsysInfoUpdReq,&tSysInfoUpdRsp);
            CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SI Check No Update! \n");
            return;
         }
        else
        {
            /* ����и��������±�SIB1 �����ĸ���Ҫ���͵�sib1����*/
            dwResult = CimFourEncodeSib1Stream(ptCimSIBroadcastVar, &tAllSibList,&tNewBroadcastStream);
            if(FALSE == dwResult)
            {
                CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: ReEncode Sib1 fail! \n");
#ifndef VS2008
                return ;
#endif
            }
        }

    /* 6.���ɷ��͸��û���Ĺ㲥���� */
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
    /* 7�����ݲ�ͬ���������õ��������͵��û��� */
    dwResult = CimSendBroadcastMsgToRnlu( );
    if (RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: send to RNLU fail!\n");
#ifndef VS2008
        return ;
#endif
    }
    /* ����ϵͳ��Ϣ�ȴ���ʱ��������Ӧ��Ϣ */
    dwResult = CimSetSystemInfoTimer();
    if (RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: CIM Set System Info Timer Fail!\n");
#ifndef VS2008
        return ;
#endif
    }

    /**����������û��棬����ֱ�ӷ���*/
    if (0 == g_tSystemDbgSwitch.ucRNLUDbgSwitch)
    {
        /*�Լ������û������Ӧ��Ϣ*/
        Message tNoDbgRNLU;
        T_RnluRnlcSystemInfoRsp tRnluRnlcSystemInfoRsp;
        tRnluRnlcSystemInfoRsp.dwResult = 0;
        tRnluRnlcSystemInfoRsp.wCellId = ptCimSIBroadcastVar->wCellId;
        tNoDbgRNLU.m_pSignalPara = static_cast<void*>(&tRnluRnlcSystemInfoRsp);
        Handle_SystemInfoUpdateRsp(&tNoDbgRNLU);
        return ;
    }
    /* ״̬Ǩ�Ƶ��ȴ���Ӧ��Ϣ */
    TranState(CCM_CIM_SIBroadcastComponentFsm, WaitSystemInfoUpdateRsp);

    return;
}

/*<FUNC>***********************************************************************
* ��������: Handle_SystemInfoUpdateRsp
* ��������: �����С�������Ĺ㲥��Ϣ���յ���Ӧ��Ϣ����Ӧ���ת������������
*           ������ǽ�������Ĺ㲥���£���Ӧ��Ϣ�ɹ�ʱ�ض�Ӧ�Ĺ�����Ӧ��Ϣ
*           ��Ӧ��Ϣʧ��ʱ�������ش�����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    memset((VOID*)&tSysInfoUpdRsp, 0, sizeof(tSysInfoUpdRsp));

    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* ��ȡCIMС���㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* ��ȡ��Ӧ��Ϣ������Ϣ */
    ptRnluRnlcSystemInfoRsp = (T_RnluRnlcSystemInfoRsp *)pMsg->m_pSignalPara;
    tSysInfoUpdRsp.dwResult = ptRnluRnlcSystemInfoRsp->dwResult;
    tSysInfoUpdRsp.wCellId =  ptRnluRnlcSystemInfoRsp->wCellId;

    /* ������� */
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

    /* 1.С����������Ĺ㲥���� */
    if ((SI_UPD_CELL_SETUP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (SI_UPD_CELL_ADD_byUNBLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimKillSystemInfoTimer();
        tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

        /* ��������������Ӧ��Ϣ */
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
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);

            return;
        }
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Rsp! Cause:%s!\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
        /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
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
            /*������һ��ʧ�ܾ�ֱ�Ӹ�cmm��ʧ����Ӧ*/
            tSysInfoUpdRsp.dwResult = 1;
            tSysInfoUpdRsp.wCellId = ptRnluRnlcSystemInfoRsp->wCellId;
                  /* ��������������Ӧ��Ϣ */
            dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tSysInfoUpdRspMsg);         
            if (SSL_OK != dwResult)
            {
                /* print */
                 /*�������ط�����ʧ�ܾ͹ҵ���*/
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                    dwResult, 
                                    ptCimSIBroadcastVar->wCellId,
                                    RNLC_ERROR_LEVEL, 
                                    "\n SIBroadcast: CIM Send to CellSetupComponent Rsp Fail!Reuslt =%d, CellId=%d SI Upd Cause: %s\n",
                                    dwResult,ptCimSIBroadcastVar->wCellId,
                                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            /*���Ӵ�ӡ�ж���λ״̬���������ⶨλ*/
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

            /* ��������������Ӧ��Ϣ */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Rsp ! SI Upd Cause = %s\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;       
        }
        else
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:  Not All message has rsp !SI Upd Cause: %s\n",CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            return;
        }
    }

    /* 2.��С����������Ĺ㲥����ʧ�� */
    if (0 != ptRnluRnlcSystemInfoRsp->dwResult)
    {
        /* �����ش����� */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount++;
        if (ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount <= 3)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,  "\n SI: Cim The %u time Send Broadcast Msg To Rnlu fail !SI Upd Cause : %s\n",
            ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount,
            CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));
            /* ɱ����ʱ�� */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Receive RNLU Response upd reason= %d,SI Upd Cause: %s!",
                        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause,
                        CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
    }

    /* 2.��С����������Ĺ㲥���³ɹ����ߴﵽ�����ش����� */
    if ((0 == ptRnluRnlcSystemInfoRsp->dwResult) || (ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount > 3))
    {
        if (SI_UPD_CELL_UNBLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)
        {
            tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

            /* ��������������Ӧ��Ϣ */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
        }

        if ((SI_UPD_CELL_DBS_MODIFY == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
           (SI_UPD_CELL_GPSUNLOCK == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
        {
            /* ��CMM���ع�������Ӧ��Ϣ */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
        }

        /* ɱ����ʱ�� */
        CimKillSystemInfoTimer();
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI : Receive RNLU Response succ upd reason= %d ! SI Upd Cause: %s",
                    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause,
                    CIMSIUPDATECAUSE(ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause));  
        /* ���ͳ�ƴ��� */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoSendCount = 0;
    }
    /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
    TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
    return;
}

/*<FUNC>***********************************************************************
* ��������: Handle_SystemInfoUpdateTimeout
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* ��ȡCIMС���㲥��Ϣ */
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

        // ͳ�ƺ����쳣�����ܷ���
    }

    tSysInfoUpdRsp.dwResult = 1;
    tSysInfoUpdRsp.wCellId   = ptCIMVar->wCellId;

    Message tSysInfoUpdRspMsg;
    tSysInfoUpdRspMsg.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tSysInfoUpdRspMsg.m_wLen = sizeof(tSysInfoUpdRsp);
    tSysInfoUpdRspMsg.m_pSignalPara = static_cast<void*>(&tSysInfoUpdRsp);

    /* 1.С����������Ĺ㲥����,��ɾ��������Ҫ����������Ϣ��
      ����Ҳ�������ش����ƣ�ֱ�ӻ���Ӧ */
    if ((SI_UPD_CELL_SETUP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (SI_UPD_CELL_DELADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        tSysInfoUpdRspMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_RSP;

        /* ��������������Ӧ��Ϣ */
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
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
            return;
        }
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n SI_UPD_CELL_SETUP!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
        /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
        TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        return;
    }

    /* 2.��С����������Ĺ㲥���� */

    /* �����ش����� */
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
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
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
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
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

            /* ��������������Ӧ��Ϣ */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
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
            /* ��CMM���ع�������Ӧ��Ϣ */
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
                /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
                TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
                return;
            }
            /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
            TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        }
        /*����û��治��Ӧ���µĹ㲥����������waitsiupdate̬������*/
        /* ״̬Ǩ�Ƶ�idle״̬���ȴ�С���㲥�������� */
        TranState(CCM_CIM_SIBroadcastComponentFsm, Idle);
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        ptCimSIBroadcastVar->wCellId,
                        RNLC_ERROR_LEVEL, 
                        "\n not SI_UPD_CELL_DBS_MODIFY or un block!Cell id= %d\n",
                        ptCimSIBroadcastVar->wCellId);
        /* ���ͳ�ƴ��� */
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
* ��������: Handle_S1KillWarningReq
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1KillWarningReq(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    TCpmCcmKillMsgReq *ptCpmCcmKillMsgReq            = NULL;
    ptCpmCcmKillMsgReq = (TCpmCcmKillMsgReq *)pMsg->m_pSignalPara;
    /* ��ȡ�㲥���������� */
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
    /* ��ȡ�㲥���������� */
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
* ��������: Handle_S1WarningReq
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningReq(Message *pMsg)
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);



    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /*��Ҫ���Ӷ���Ϣ���ȵ�У��*/
    if ( pMsg->m_wLen != sizeof(TCpmCcmWarnMsgReq))
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: EPC Warn Msg Len Wrong,m_wLen = %u\n",pMsg->m_wLen);
        return;
    }
    // �����Ϸ�������������������ж���ETWS���ܣ�����CMAS����
    WORD32 dwPwsType = GetWarningType(ptCpmCcmWarnMsg->tMessageIdentifier);
    /*��� ����Ϊ���ģʽ*/
    if(1 == g_ucEpcWarningCheckFlag)
    {
        /**����澯��ԭ����һ������ֱ�ӷ��أ�����Ҫ������*/
        if(TRUE == CheckSamePWSReq(ptCpmCcmWarnMsg))
        {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: EPC Warn Msg The Same as Last Time!\n");       
            return ;
        }
    }
    /*�Ƚ����εĸ澯��Ϣ�����ڹ㲥�������������Ա��㲥����ʱʹ��*/
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
        /*Ŀǰs1�ڻ�δ�������ĸ澯��Ϣ��������Ϊδ֪�����쳣*/
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
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(pMsg);
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    T_CIMVar *ptCIMVar = NULL;
    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    Message tEtwsUpdate;
    T_SysInfoUpdReq tSysinfoUpdReq;
    tSysinfoUpdReq.wCellId = ptCIMVar->wCellId;

    tEtwsUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tEtwsUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tEtwsUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tEtwsUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /*����˴θ澯���ϴθ澯��Ϣ��һ���������Ǵ���*/
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
   // ����������˾ͽ����ˣ�sib10  �� sib11�Ĵ������update�����д���

}

WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleCMASWarningReq(Message *pMsg)
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(pMsg);
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    T_CIMVar *ptCIMVar = NULL;
    /* ��ȡCIMС��ʵ����Ϣ */
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

    /*�������cur�ֶΣ����ӵ���ǰ�澯*/
    if(1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    {
        /*�����ǰ���͵ĸ澯�Ѿ��ﵽ��������������ֱ�ӷ���ʧ��*/
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[MAX_CMAS_MSG -1].aucIsCurrentBroadcasting)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 broading now has reach the MAX number!%d\n",MAX_CMAS_MSG);
            return RNLC_FAIL;
        }
        /*����˴θ澯û���ڷ���*/
        tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_ADD;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 with cur! sib12 add!\n");
        SendToSelf(&tCmasUpdate);
        return RNLC_SUCC;
    }
    else
    {
        if(TRUE == CimIsBoardIncludeSib12(ptCpmCcmWarnMsg))
        {
            /*���������cur�ֶΣ����Ұ���sib12�������򸲸ǵ�ǰ�澯*/
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_UPD;
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 without cur! sib12 upd!\n");
            /*����ϵͳ��Ϣ*/
            SendToSelf(&tCmasUpdate);
            return RNLC_SUCC;
        }
        else
        {
            /*���������cur�ֶΣ�����û��sib12�����ݣ���ʱ��ֹͣ���е��ڷ���sib12�澯*/
            tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_CMAS_SIB12_STOP;
            CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 without cur! sib12 STOP!\n");
            /*����ϵͳ��Ϣ*/
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
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    WORD32 dwResultMsgid =0;
    WORD32 dwResultSeril =0;

    /* ��ȡ�㲥���������� */
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
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    WORD16 wCmasMsgId;
    WORD16 wCmasStreamId;
    wCmasMsgId = ptCpmCcmWarnMsg->tMessageIdentifier.data[0]*256 +  ptCpmCcmWarnMsg->tMessageIdentifier.data[1];
    wCmasStreamId =ptCpmCcmWarnMsg->tSerialNumber.data[0]*256 + ptCpmCcmWarnMsg->tSerialNumber.data[1];

    /* ��ȡ�㲥���������� */
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
* ��������: Handle_S1WarningRsp
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningRsp(Message *pMsg)
{

    /* ��ȡCIMС��ʵ����Ϣ */
    T_CIMVar     *ptCIMVar = NULL;
    BYTE   aucSibList[DBS_SI_SPARE] = {0};

    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)pMsg->m_pSignalPara;
    WORD32 dwPwsType = GetWarningType(ptCpmCcmWarnMsg->tMessageIdentifier);
    /*��Ҫ���Ӷ���Ϣ���ȵ�У��*/
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

    /*��ȡ���ݿ��ȡsib1���Ӷ��õ�������Ϣ,ͨ��aucSibList ���س���*/
    {
        SchedulingInfoList tSchedulingInfoList;
        memset((VOID*)&tSchedulingInfoList, 0, sizeof(tSchedulingInfoList));

        T_DBS_GetSib1Info_REQ tGetSib1InfoReq;
        T_DBS_GetSib1Info_ACK tGetSib1InfoAck;

        memset((VOID*)&tGetSib1InfoReq, 0, sizeof(tGetSib1InfoReq));
        memset((VOID*)&tGetSib1InfoAck, 0, sizeof(tGetSib1InfoAck));

        /* 2.��ȡSIB1��Ϣ */
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
        /* �޸�SISCH��Ϣ */
        CimDbgConfigSISch(&tGetSib1InfoAck);

        /* 3.��װ������Ϣ */
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
    /*�����cmas�澯�������Ѿ��ﵽ�����ķ�����������ֱ�ӷ���ʧ��*/
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
    /*��etws�澯�����û�ж�Ӧ�ĵ�����Ϣ��ظ�ʧ��*/
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
    /*������ٵļ�����etwsҲ����cams����û���µĹ�����չǰ�����������ظ�ʧ��*/
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
* ��������: Handle_S1WarningSib10Timeout
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningSib10Timeout()
{
    /* �յ���ʱ����ʱ����CIM��ʧ����Ӧ */
    T_SysInfoUpdReq tSysinfoUpdReq;
    Message message;
    memset((BYTE *)&tSysinfoUpdReq, 0x00, sizeof(tSysinfoUpdReq));
    Message tEtwsUpdate;
    tEtwsUpdate.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tEtwsUpdate.m_wLen = sizeof(tSysinfoUpdReq);
    tEtwsUpdate.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tEtwsUpdate.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;

     /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar); 
    tSysinfoUpdReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tSysinfoUpdReq.ucCellSIUpdCause = CIM_CAUSE_ETWS_SIB10_STOP;
    SendToSelf(&tEtwsUpdate);
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer = INVALID_TIMER_ID;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB10 Timer out happen!\n");
    return;
}

/*<FUNC>***********************************************************************
* ��������: Handle_S1WarningSib11Timeout
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
void CCM_CIM_SIBroadcastComponentFsm::Handle_S1WarningSib11Timeout()
{
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
       /*��sib11�ķ���ֹͣ��ʱ����ʱ�����ᴥ���㲥���£��˴���Ψһ��λ���ͱ�ʾΪ0�ĵط�*/
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
* ��������: CimBroadcastDataInit
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CCM_CIM_SIBroadcastComponentFsm::CimBroadcastDataInit(T_SysInfoUpdReq  *ptsysInfoUpdReq)
{
    T_CIMVar *ptCIMVar = NULL;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    ptCimSIBroadcastVar->wCellId = ptCIMVar->wCellId;
    ptCimSIBroadcastVar->dwSysInfoTimerId   = INVALID_TIMER_ID;
    memset(ptCimSIBroadcastVar->tSib1Buf,0,sizeof(ptCimSIBroadcastVar->tSib1Buf));
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
    ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
    /*ֻ����mac �����ֽ�ӳ�����ܳ���*/
    ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len = 0;
    /*���ֽڶ������ܳ���*/
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib11MsgLength = 0;
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength = 0;
/*��ͨ�㲥����ʱĬ����Ϊ����Ҫ��дsib10��Ϣ*/
    ptCimSIBroadcastVar->tSIBroadcastDate.ucNormalSibWithSib10Info = 0;
    /* С������ʱ��ʼ��*/
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
* ��������: CimNormalBroadMsgWithSib10
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
* ��������: CimSibMappingToSi
* ��������:��SIBӳ�䵽SI��
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSibMappingToSi(T_AllSibList *ptAllSibList,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo)
{
    SchedulingInfoList       *ptScheInfo  = NULL;   /* ������Ϣ */
    SystemInformation_r8_IEs *ptSysInfoR8 = NULL;   /* SI�����ṹ */
    BYTE                     ucSibType;             /* SIB�� */
    WORD32                   dwSibNum;              /* ÿ��SI��SIB���� */
    WORD32                   dwSiNum;               /* SI��Ŀ */
    WORD32                   dwSibNumLoop;          /* ÿ��SI��SIB���� */
    WORD32                   dwSiNumLoop;           /* SI���� */
    SchedulingInfoList  tSchedulingInfoList;        /*������Ϣ�б�*/
    memset(&tSchedulingInfoList,0,sizeof(SchedulingInfoList));
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

    /* ������Ϣ */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    ptScheInfo = &(ptAllSibList->tSib1Info.schedulingInfoList);
    /*��sib11��sib12����Ҫ���ǵ�����si�����ﲻ��Ҫ�������������������Ӧ�����ã�����Ӧ�ô����*/
    /*ע��������ΪCimUpdateSib1Sche�ڲ�������߼�������ǧ�򲻿�ֱ����ptScheInfo����*/
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
    /* ����SI������д */
    for (dwSiNumLoop = 0; dwSiNumLoop < dwSiNum; dwSiNumLoop++)
    {
        ptSysInfoR8 = &(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].systemInformation_r8);
        ptSysInfoR8->m.nonCriticalExtensionPresent = FALSE;
        ptSysInfoR8->sib_TypeAndInfo.n = 0;

        ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].tSiPeriod = ptScheInfo->elem[dwSiNumLoop].si_Periodicity;
        dwSibNum = ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.n;

        if (0 == dwSiNumLoop)
        {
            /* ��һ��SI�а���SIB2 */
            dwSibNum += 1;
        }
        /*��ʶsi��ӳ���sib������ɶ����ƣ��ӵ�λ����λ��10��bit�����ζ�Ӧsib2--sib10*/
        WORD16 wSISibFlag =0;
        /* ����ÿ��SI��SIB������д */
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

            /* �����һ��SI����������SIB2����� */
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
                    /*���澯��δ����С��ɾ���Ĺ㲥���µ�ʱ��*/
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
* ��������: CimEncodeAllSi
* ��������: SI����ʧ���Ƿ���Ҫ����������Ϣ������ʱ�����ǣ��µ��������Ƿ�仯
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllSi(T_AllSibList *ptAllSibList,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo,TWarningMsgSegmentList *ptWarningMsgSegmentList,
        TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation          tSystemInfo;  /* ϵͳ��ϢSI����ṹ */
    ASN1CTXT                   tCtxt;        /* ����������Ľṹ */
    WORD32                                 dwSiNumLoop            =  0;  /* SI�������� */
    WORD16                                 wMsgLen                   =  0;      /* SI�������� */
    WORD16                                 wTbSize                    =  0;      /* SI����ƥ���TB���С */
    WORD32                                 dwSiNumInSchList      = 0;
    WORD32                     dwResult = FALSE;
    SystemInformation_r8_IEs         *ptSystemInfoR8       =  NULL;
    SchedulingInfoList         *ptSchedulingInfoList = NULL;

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

      /*��ʼ��*/
    memset((VOID*)&tBcchDlSchMsg, 0, sizeof(tBcchDlSchMsg));
    memset((VOID*)&tBcchDlSchMsgC1, 0, sizeof(tBcchDlSchMsgC1));
    memset((VOID*)&tSystemInfo, 0, sizeof(tSystemInfo));
    memset((VOID*)&tCtxt, 0, sizeof(tCtxt));
    
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI Encoding Now...\n");
    
   /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
   
    ptSchedulingInfoList = &(ptAllSibList->tSib1Info.schedulingInfoList);
    dwSiNumInSchList = ptSchedulingInfoList->n;

    /* SI��ĿΪѭ������ */
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
                /* SIB2�ڵ�һ��SI�У�������ʧ����ֹͣ�·��㲥 */
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
            /* SI ����ɹ�����,�����ж������������Ƿ���ϵ������� */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SI-%d stream len(=%u) beyond TB size boundary!\n",
                            dwSiNumLoop + 1, wMsgLen);
                if (0 == dwSiNumLoop)
                {
                    /* SIB2�ڵ�һ��SI�У�������ʧ����ֹͣ�·��㲥 */
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
                /* SI����ɹ���CMAC�����������ȣ��򱣴����� */
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid = TRUE;
                memcpy(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg,
                       &(tCtxt.buffer.data[0]), wMsgLen);

                /* SI�ĳ���ȡΪƥ���TB���С */
                ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen = wTbSize;
                
                CCM_CIM_LOG(RNLC_INFO_LEVEL, "SI: Normal SI-%lu Broadcast Stream! len = %d, MAC Tb Size = %lu", dwSiNumLoop+ 1,wMsgLen,wTbSize);
                /* ��ӡ��������Ϣ */
                if (1 == g_dwCimPrintFlag)
                {
                     CimDbgPrintMsg(&(tCtxt.buffer.data[0]), wMsgLen);
                }
            }
        }
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Normal SI Encode Finish!\n");
    /*����澯��Ϣ�ı���*/
    BOOL bAllSIENcodeFlag = FALSE;
    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: ETWS SIB11 SegmentList Encoding...\n");  
        bAllSIENcodeFlag = CimEncodeAllSegSi(ptWarningMsgSegmentList);
        /*������з�Ƭ������ʧ�ܣ��򲻷���sib11*/
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
        /*������з�Ƭ������ʧ�ܣ��򲻷���sib12*/
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
* ��������: CimSibsCheckAndUpdte
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptAllSibList);
    CCM_NULL_POINTER_CHECK(ptNewBroadcastStream);

    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    if ((SI_UPD_CELL_SETUP == ptsysInfoUpdReq->ucCellSIUpdCause)
       ||(SI_UPD_CELL_UNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause)
       ||(SI_UPD_CELL_ADD_byUNBLOCK == ptsysInfoUpdReq->ucCellSIUpdCause))
    {
        /*С����������Ҫ���й㲥���µ��ж�*/
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: Cell SetUp/UnBlcock/DelAdd ,SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    if(SI_UPD_CELL_DELADD == ptsysInfoUpdReq->ucCellSIUpdCause)
    {
       /* SIB�б��б仯��ValueTag++ */
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
        /*EPC �澯����Ҫ���й㲥���µ��ж�*/
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: EPC warning SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 1.�Ƚ�SIB�б� ,ֻ�Ƚ�sib1--sib10����*/
    /*lint -save -e732 */
    dwResult = memcmp(ptAllSibList->aucSibList, ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList, sizeof(BYTE) * (CIM_MAX_SIB_LIST_LEN - 2));
    /*lint -restore*/
    if (0 != dwResult)
    {
        /* SIB�б��б仯��ValueTag++ */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag++;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag
        = (BYTE)((ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag) % (31 + 1));
        /*    */
        ptCIMVar->ucSib1ValueTagCopy = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB list Update SI Update Check True! Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 2.����·���SIB�б�û�б仯�����ȱȽ�SIB2��SIB2֮������� */
    /*lint -save -e732 */
    dwResult = memcmp(ptNewBroadcastStream->aucAllSibBuf,
                      ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucAllSibBuf,
                      sizeof(BYTE) * CIM_ALL_SIB_BUF_LEN);
    /*lint -restore*/
    if (0 != dwResult)
    {
        /* SIB�б��б仯��ValueTag++ */
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag++;
        ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag
        = (BYTE)((ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag) % (31 + 1));

        /*    */
        ptCIMVar->ucSib1ValueTagCopy = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB Stream Update SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }

    /* 3.�Ƚ�MIB ��������*/
    if(ptNewBroadcastStream->ucMibLen != ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucMibLen)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: MIB Length different  SI Update Check TRue!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    /*4. �Ƚ�MIB����*/
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

    /* 5.�Ƚ�SIB1 ��������*/
    if(ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen != (WORD16) ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucSib1Len)
    {
        CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: SIB1 Length different  SI Update Check True!Cell id = %d\n",ptCimSIBroadcastVar->wCellId);
        return TRUE;
    }
    /*6.�Ƚ�sib1 ����*/
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
    /*�������������û�и���*/
    return FALSE;
}
/*<FUNC>***********************************************************************
* ��������: CimSibsDealWithNoUpdate
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSibsDealWithNoUpdate(T_SysInfoUpdReq  *ptsysInfoUpdReq,
        T_SysInfoUpdRsp    *ptSysInfoUpdRsp)
{

    T_CIMVar     *ptCIMVar = NULL;

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptsysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdRsp);

    /* ��ȡCIMС��ʵ����Ϣ */
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* ��ʾδ���£�����Ҫ�·��㲥 */
    CCM_CIM_LOG(RNLC_WARN_LEVEL,
            "\n SI: CIM Check System not Update! CellId=%d! \n",
            ptCIMVar->wCellId);
    /* ���ϵͳ��Ϣû�и��£��Ǹ�Ҳ��CIM ���� CMM��һ����Ӧ��Ϣ������Ǽܹ���ԭ�� */
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
    /*�������sib11�澯�����si*/
    BYTE    ucSiTransNum  = 0;        /* ��SI window��SI������� */   
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_SystemInfoETWS_up        *ptSystemInfoETWS_up = NULL;
    T_RnlcRnluSystemInfoHead  tRnlcRnluHeadInfo;       /* ���͵�USM�Ľṹ */
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
    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /*�������sib11 ֹͣ�йص���Ϣ����ʱҲ����Ҫ����sib11��Ϣ�ģ�����ԭ��Ϊsib11 stop*/
    if((CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
    (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        tRnlcRnluHeadInfo.wSystemInfoCause = CAUSE_SIB11_STOP;
        tRnlcRnluHeadInfo.m.dwSib11Present =0;
    }
    else
    {
        /*�������Ǹ澯����sib11 ����Ҫ������ʵ����*/
        tRnlcRnluHeadInfo.wSystemInfoCause = CAUSE_SIB11_START;
    }
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));   
    /*����Ϣͷ��Ϣ����*/
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff, &tRnlcRnluHeadInfo, sizeof(T_RnlcRnluSystemInfoHead));
    /*ת��ָ������*/
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
    /*����һ���û��������ж�Э���м�����������жϵ��ֶ�*/
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
    /*�������sib11�澯�����si*/

    WORD16  wAlignmentLen = 0;        /* 4�ֽڶ���󳤶� */
    WORD16 dwSib12HeaderLen =0;
    BYTE    ucSiTransNum  = 0;        /* ��SI window��SI������� */
    static BYTE    aucSISib12MsgBuff[CIM_SYSINFO_MSG_SIB11_BUF_LEN];  /*��� sib12 �澯�Ĺ㲥���������� ��ǰ��С�޸��޶�*/
    T_RnlcRnluSystemInfoHead  tRnlcRnluHeadInfo;       /* ���͵�USM�Ľṹ */
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
    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));
       
    /*����Ϣͷ��Ϣ����*/
    memcpy(ptSystemInfoCMAS_Head_up, &tRnlcRnluHeadInfo, sizeof(T_RnlcRnluSystemInfoHead));
    /*ת��ָ������*/
    ptSystemInfoCMAS_up =(T_SystemInfoCMAS_up *)(aucSISib12MsgBuff+ sizeof(T_RnlcRnluSystemInfoHead));
    /*si������дԭ�������ڹ����������Ĵ�С*/
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
    /*����һ���û��������ж�Э���м�����������жϵ��ֶ�*/
    ptSystemInfoCMAS_up->ucInfiniteSend = ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg.m.bitConcurrentWarningMessageIndicatorPresent;
    /*************************************************************************************/
    WORD16 wAllSiLen = 0;
    for (WORD32 dwSegLoop = 0;(dwSegLoop < ptCmasWarningMsgSegmentList->dwSib12SegNum)
        && (dwSegLoop < RNLC_CCM_MAX_SEGMENT_NUM); dwSegLoop++)
    {
        if (FALSE == ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSegLoop].ucValid)
        {
            /* ��SI����ʧ�ܻ��߲��ܵ��ȣ����·� */
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
        /*�����·��͵�sib12 �б����������£����Ա�֤������ά���ķ����б��ǿɿ���*/
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
        /*�����cmas kill����Ϣ��ֻ��Ҫ��дɾ���б�Ȼ��ֱ�ӷ���*/
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
        /*�����cmas stop����Ϣ��ֻ��Ҫ��дɾ���б�Ȼ��ֱ�ӷ���*/
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
    /*������Ӧsi��������������*/
    for (WORD16 wSib12SegNum =0;wSib12SegNum<(WORD16)ptCmasWarningMsgSegmentList->dwSib12SegNum;wSib12SegNum++)
    {   
        if ( TRUE == ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].ucValid)
        {
            wAlignmentLen = (WORD16)CimEncodeAlignment(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);  /* ��Ӧsi��������󳤶� */
            /*������ʱ����һ����֤������ı�����cmas����Ҫ�ع�*/
            if((dwSib12HeaderLen + wSib12Len + wAlignmentLen) > CIM_SYSINFO_MSG_SIB11_BUF_LEN)
            {
               CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 buf over flow! \n");
               continue;
            }
            memcpy(ptSystemInfoCMAS_up->tCmasAddList.aucData+ wSib12Len, ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].aucMsg, 
            ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);
            wSib12Len += wAlignmentLen;
            /* �ȴ�ӡ���������������ⶨλ���������ڴ˴�����һ������*/
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CMAS SI-%lu Broadcast Stream! len = %d\n", wSib12SegNum + 1,ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[wSib12SegNum].wMsgLen);

            /* ��ӡ��������Ϣ */
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
* ��������: CimSaveMsgSendToRnlu
* ��������: �������ɵĹ㲥������si���������������������ڷ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSaveMsgSendToRnlu(T_SysInfoUpdReq *ptSysInfoUpdReq,
                                                                        WORD16 wMsgLen,
                                                                        BYTE *ptBroadCastMsgStream)
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptBroadCastMsgStream);
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause = ptSysInfoUpdReq->ucCellSIUpdCause;
    ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength = wMsgLen;
    memmove(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff, ptBroadCastMsgStream, wMsgLen);
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimComposeMsgToRnlu
* ��������: �㲥����FDD
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptSysInfoUpdReq);
    CCM_NULL_POINTER_CHECK(ptNewSibList);
    CCM_NULL_POINTER_CHECK(ptNewBroadcastStream);
    CCM_NULL_POINTER_CHECK(ptAllSiEncodeInfo);

    WORD16  wSib8SiNo     = 0;        /* ����SIB8��Ϣ */
    static BYTE    aucSIBroadcastMsgBuff[CIM_SYSINFO_MSG_BUF_LEN];  /*�����ͨ�㲥����������*/  
    WORD32  dwSiNumLoop   = 0;        /* ����SI ����ṹ */
    WORD16  wMsgLen       = 0;        /* ��Ҫ���͸�RNLU ����Ϣ���ȣ�4�ֽڶ��� */
    WORD16  wMaxLen       = 0;        /* ��󻺳������� */
    WORD16  wMibLen       = 0;        /* MIB���� */
    WORD16  wSib1TbSize   = 0;        /* SIB1ƥ���TB���С */
    WORD16  wAlignmentLen = 0;        /* 4�ֽڶ���󳤶� */
    BYTE    ucSiTransNum  = 0;        /* ��SI window��SI������� */
    BYTE    ucValidSiNum  = 0;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;
    T_RnlcRnluSystemInfoHead  *ptRnlcRnluSystemInfoHead =NULL;    
    T_SystemInfoMaster_up      *ptSystemInfoMaster_up = NULL;
    T_SystemInfo1_up              *ptSystemInfo1_up = NULL;
    T_SystemInfo_up                *ptSystemInfo_up = NULL;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: Composing Normal SI Msg to RNLU now...\n");
    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    wMaxLen = sizeof(aucSIBroadcastMsgBuff) - 16; /* Ԥ�������򣬽��pc-lint */  
    memset(aucSIBroadcastMsgBuff, 0, sizeof(aucSIBroadcastMsgBuff));
    
    /* ���͹����Ĺ㲥��Ϣʵ�ʸ�ʽ���£�
    TBcmUsmSystemInfoHead �� TSystemInfoMster_up���䳤����TSystemInfo1_up���䳤��
    ��TSystemInfoList_up���䳤�� */

    /***START***********************��дTBcmUsmSystemInfoHead  **********************************/
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
        /* ����SI��Ϣ */
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
    // ���Ұ�������
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
    ptRnlcRnluSystemInfoHead->wTimeStamp = 0;  /* sib8��Ϣ��ʱ��Ϊ0 */
    ptRnlcRnluSystemInfoHead->dwModiPeriod = ptNewSibList->tSib2Info.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff;
    ptRnlcRnluSystemInfoHead->dwSiWndSize = ptNewSibList->tSib1Info.si_WindowLength;
    /*������дSIB8���������begin*/
    CimFillSib8No(&(ptNewSibList->tSib1Info.schedulingInfoList), &wSib8SiNo);
    if(0xffff == wSib8SiNo)
    {
         ptRnlcRnluSystemInfoHead->dwSib8CfgInfo = 0xffffffff;
    }
    else
    {
        //  ptRnlcRnluSystemInfoHead->dwSib8CfgInfo = g_dwSysTimeAndLongCodeExist<<31;/*ϵͳʱ��ͳ���״̬��дָʾ*/
        /*sib8cfginfo�еĸ�����bit ��ʾ���û�����Լ����00 ϵͳʱ��ͳ���״̬������ 01 ��дϵͳʱ�� 10 ������*/
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
        /*�����Ϣͷ�е�sib8��Ϣ*/
        if ( g_bCcGPSLockState )
        {
             ptRnlcRnluSystemInfoHead->dwSib8CfgInfo &= 0xDFFFFFFF;/*��λ������bit��ֵΪ0*/
        }
        else
        {
             ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= 1<<29;/*������bit��ֵΪ1*/
        }

        DWORD dTmp = wSib8SiNo;
        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= dTmp<<25;/*����bit����7���ظ�ֵ,�Ѿ���λ�������*/

        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= ptNewSibList->dwSib8SystemTimeInfoOffset<<13;

        ptRnlcRnluSystemInfoHead->dwSib8CfgInfo |= ptNewSibList->dw1XrttLongCodeOffset;
    }
    /*������дSIB8���������end*/
    /*si���������¹滮�Ľӿڽṹ���ƹ����ģ���ͷ��Ϣ����д*/
    ucValidSiNum = 0;
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (TRUE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            ucValidSiNum++;
        }
    } 
    ptRnlcRnluSystemInfoHead->ucSiNum = ucValidSiNum;

    /* ��дSystemInfoCause  ��Ϊʵ�ַ��㣬Ĭ����Ϊ�澯���������ͨ���£������Ϊ��ͨ����*/ 
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
        /*����и澯������Ĭ����ϵͳ�㲥����*/
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIMODIFY;
        /*���е�����Ϣ������ȷ�������·���ʱ���޸ĸ���ԭ��Ϊ�澯*/
        if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10) ||
            (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11))
        {
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIB10_START ;
        }
    }
    if((CIM_CAUSE_ETWS_SIB10_STOP == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        /*ֻ��sib11���͵ĳ����ǲ��ᷢ����ͨ�㲥�ģ�����Ϊ��
           ��ֹ����������������ԭ��*/
        (CIM_CAUSE_ETWS_SIB11_ADD == ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting)
        {
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIB10_STOP ;
        }
        else
        {
            /*����дΪsi���£���Ӱ�죬��Ϊ�����������û����ʱ���ʱ��������ͨ�㲥*/
            ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_SIMODIFY;
        }
    }
    if((CIM_CAUSE_CMAS_SIB12_ADD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_UPD == ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_KILL== ptSysInfoUpdReq->ucCellSIUpdCause)||
        (CIM_CAUSE_CMAS_SIB12_STOP== ptSysInfoUpdReq->ucCellSIUpdCause))
    {
        /*����и澯������Ĭ����ϵͳ�㲥����*/
        ptRnlcRnluSystemInfoHead->wSystemInfoCause = CAUSE_CMAS_INFO;
    }
    if (wMsgLen >= wMaxLen)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: send to RNLU Buffer area is very minor. CellId=%d\n",ptCimSIBroadcastVar->wCellId);
        return FALSE;
    }
    wMsgLen += sizeof(T_RnlcRnluSystemInfoHead);
    
        /* ***END***********************��дTBcmUsmSystemInfoHead  **********************************/

        /* ***START***********************��дTSystemInfoMster_up  **********************************/
        ptSystemInfoMaster_up = (T_SystemInfoMaster_up *)(aucSIBroadcastMsgBuff + wMsgLen );
        wMibLen = ptNewBroadcastStream->ucMibLen;
        ptSystemInfoMaster_up->wMsgLen = (WORD16)wMibLen;
        wAlignmentLen = (WORD16)CimEncodeAlignment(wMibLen);  /* ����󳤶� */
        /*����mib��ĵ�ǰ����*/
        wMsgLen  = wMsgLen + sizeof(WORD16) + 2*sizeof(BYTE) + wAlignmentLen;
        if (wMsgLen >= wMaxLen)
        {
            CCM_CIM_LOG(RNLC_ERROR_LEVEL,
                                        "\n SI: send to RNLU Buffer area is very minor. CellId=%d\n",
                                        ptCimSIBroadcastVar->wCellId);
                                        return FALSE;
        }
        memcpy(ptSystemInfoMaster_up->aucData, ptNewBroadcastStream->aucMibBuf, wMibLen);
        /*����һ�ݵ��㲥�������������ڱȽϸ���*/
        memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucMibBuf, ptNewBroadcastStream->aucMibBuf, wMibLen);
        ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucMibLen = (BYTE)wMibLen;
   
        /* ***END***********************��дTSystemInfoMster_up  **********************************/

        /* ***START***********************��дTSystemInfo1_up  ************************************/
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

        wAlignmentLen = (WORD16)CimEncodeAlignment(wSib1TbSize);  /* ����󳤶� */ 
        /*����sib1��ĵ�ǰ����*/
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
        /*����ָ�뵽������sib1 ��ĩβ*/
        ptSystemInfo1_up = (T_SystemInfo1_up *)(aucSIBroadcastMsgBuff + wMsgLen );
    }
    /*����һ��sib1 ���㲥�������������ڱȽϸ���*/
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[0].aucSib1Buf, ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen);
    ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream.ucSib1Len = (BYTE)ptCimSIBroadcastVar->tSib1Buf[0].wMsgLen;
    /* ***END***********************��дTSystemInfo1_up  ************************************/

    ptSystemInfo_up = (T_SystemInfo_up *)(aucSIBroadcastMsgBuff + wMsgLen );
    /****************��дÿ��si��Ϣ  start**************************/
    ucSiTransNum = (BYTE)CimGetSiMaxTransNum((BYTE)(ptNewSibList->tSib1Info.si_WindowLength));
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            /* ��SI����ʧ�ܻ��߲��ܵ��ȣ����·� */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SI-%lu not be send!\n",dwSiNumLoop + 1);
            continue;
        }
        /*������ͨsi*/
        ptSystemInfo_up->dwSiPeriod = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].tSiPeriod;   /* �������� */
        ptSystemInfo_up->ucSiTransNum = ucSiTransNum;   /* SI�������� */
        ptSystemInfo_up->wMsgLen = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen; /*  SI�������� */
        ptSystemInfo_up->wSegNum = 1;  /*  ��Ƭ������û�п��Ƿ�Ƭ */
        ptSystemInfo_up->wSegSize = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen;
        /*������Ӧsi��������������*/
        memcpy(ptSystemInfo_up->aucData, ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg, ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen);
        wAlignmentLen = (WORD16)CimEncodeAlignment(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen);  /* ��Ӧsi��������󳤶� */

        /*���ϵ�ǰsi ��ĵ�ǰ����*/
        wMsgLen = wMsgLen + sizeof(T_SystemInfo_up) + wAlignmentLen;
        /*����ָ�뵽��ǰsib1��ĩβ����Ϊ��һ��sib1������׼��*/
        ptSystemInfo_up = (T_SystemInfo_up *)(aucSIBroadcastMsgBuff + wMsgLen );       

    }
  
    /* ��ӡ����ǰ������Ϣ */
    CimDbgShowCurBroadcastInfo(&(ptNewSibList->tSib1Info.schedulingInfoList));

    /*ȫ�������Ѿ����ɣ����淢�͸��û������Ϣ��cim�������� ��������*/
    WORD32 dwResult = CimSaveMsgSendToRnlu(ptSysInfoUpdReq,wMsgLen,aucSIBroadcastMsgBuff);
    if(RNLC_FAIL == dwResult)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL,
                        "\n SI: Save BroadCast Msg To SI Date Fail! CellId=%d\n",
                        ptCimSIBroadcastVar->wCellId);
    }
    /*�ڷ���sib11 ���� ֹͣsib11�ĳ�����Ҫ��sib11������������*/
    if (( 1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimComposeSib11MsgToRnlu(ptRnlcRnluSystemInfoHead,ptWarningMsgSegmentList,ptNewSibList);                                                                                                                         
    }
    /*�ڷ���sib12 ���� ֹͣsib12�ĳ�����Ҫ��sib12������������*/
    if (( 1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12)||
        (CIM_CAUSE_CMAS_SIB12_KILL == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause )||
        (CIM_CAUSE_CMAS_SIB12_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause))
    {
        CimComposeSib12MsgToRnlu(ptRnlcRnluSystemInfoHead,ptCmasWarningMsgSegmentList,ptNewSibList);                                                                                                                         
    }
    
    CimSaveAllSibsStream(ptNewBroadcastStream, ptAllSiEncodeInfo, ptNewSibList);

    /* ��ӡ��������Ϣ */
    if (1 == g_dwCimPrintFlag)
    {
        CimDbgBroadcastStreamPrint(ptNewBroadcastStream, ptAllSiEncodeInfo);
    }
    return TRUE;
}



/*<FUNC>***********************************************************************
* ��������: CimSaveAllSibsStream
* ��������: ����������ʵ��������
* �㷨����:
* ȫ�ֱ���:
* Input����:
* �� �� ֵ:
**    
* �������:
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CCM_CIM_SIBroadcastComponentFsm::CimSaveAllSibsStream(T_BroadcastStream *ptNewBroadcastStream,
        T_AllSiEncodeInfo *ptAllSiEncodeInfo,
        T_AllSibList *ptAllSibList)
{
    BYTE                   bySibName;             /* SIB�� */
    WORD32                 dwSibNum;              /* ÿ��SI��SIB���� */
    WORD32                 dwSibNumLoop;          /* ÿ��SI��SIB���� */
    WORD32                 dwSiNumLoop;           /* SI���� */
    BYTE                   aucSibList[DBS_SI_SPARE] = {0};
    SchedulingInfoList    *ptScheInfo = NULL;   /* ������Ϣ */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_VOID(ptNewBroadcastStream);
    CCM_NULL_POINTER_CHECK_VOID(ptAllSiEncodeInfo);
    CCM_NULL_POINTER_CHECK_VOID(ptAllSibList);

    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* ��ʼ��sib List*/
    memmove((void*)aucSibList, (void*)ptAllSibList->aucSibList, CIM_MAX_SIB_LIST_LEN*sizeof(BYTE));

    /* ������Ϣ */
    ptScheInfo = &(ptAllSibList->tSib1Info.schedulingInfoList);

    /* ����SI������д */
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            dwSibNum = ptScheInfo->elem[dwSiNumLoop].sib_MappingInfo.n;

            for (dwSibNumLoop = 0; dwSibNumLoop < dwSibNum; dwSibNumLoop++)
            {
                if ((0 == dwSiNumLoop) && (0 == dwSibNumLoop))
                {
                    /* SIB2���ڵ�SI���ڱ���ʧ�ܣ����·�����SIB�б����޳� */
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

    /* ����SIB�б� */
    memcpy(ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList, aucSibList, sizeof(ptCimSIBroadcastVar->tSIBroadcastDate.aucSibList));

    /* �������� */
    memcpy(&(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream), ptNewBroadcastStream, sizeof(ptCimSIBroadcastVar->tSIBroadcastDate.tBroadcastStream));

    return;
}

/*<FUNC>***********************************************************************
* ��������: CimSendBroadcastMsgToRnlu
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSendBroadcastMsgToRnlu( )
{

    T_CIMVar     *ptCIMVar = NULL;
    BYTE           * pucMsg = NULL;
    WORD16      wMsgLe = 0;
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /* ��ȡCIMС��ʵ����Ϣ */
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
    /*sib12 �澯���͡�*/
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
            /*�����澯��ʱ��*/    
            CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),12);
        }
        pucMsg = (BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff);
        wMsgLe = ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib12MsgLength;
        /*����Ҫ��ʵ���û����Ӧ��ʱ��*/
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
        /* ������� */
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

    /*��ͨ�㲥���͡�����Ҫ��ʵ���û����Ӧ��ʱ��*/
    if (1 == g_tSystemDbgSwitch.ucRNLUDbgRSPSwitch)
    {
        pucMsg = (BYTE*) ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff;
        wMsgLe = (WORD16) ptCimSIBroadcastVar->tSIBroadcastDate.wCimMsgLength;
        /*����Ǹ澯�������������Ĺ㲥���� �ж��Ƿ��б�Ҫ������ͨ�㲥*/
        if((CIM_CAUSE_ETWS_SIB10_STOP_SIB11_ADD == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_SIB11_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        (CIM_CAUSE_ETWS_SIB10_STOP == ptCimSIBroadcastVar->tSIBroadcastDate.ucSysInfoUpdCause)||
        /*ֻ��sib11��ʼ���ӵĳ����ǲ�Ӧ�÷�����ͨ�㲥����������������ԭ��*/
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
                /*�����͵ı�־λ��0*/
                ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting = 0;
                /*ɱ��ԭ���Ķ�ʱ��*/
                if(INVALID_TIMER_ID != ptCimSIBroadcastVar->tCimWarningInfo.dwSib10RepeatTimer)
                {
                    CimKillWarningSib10Timer();
                }
                /* ������� */
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
        /*����ͨ�㲥����Ĺ㲥������ֱ���·���ͨ�㲥*/
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
            /* ������� */
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
            /*ͳ�Ƽ�¼������ε���ͨsi�������*/
            CimRecordToRNLUDbgInfo();
        }
    /*���������esle ���Ұ��� sib10������������ӡ��������λ*/
    if(1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)
    { 
            CCM_CIM_LOG(RNLC_INFO_LEVEL,  "\n SI: This SI To RNLU Include SIB10, CellId=%d!\n",ptCimSIBroadcastVar->wCellId);
        /*�����澯��ʱ��*/
        CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),10);
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
            /*��sib10���ͱ�־λ����1 ֻ�������ﴦ�������յķ���Ϊ����*/
        ptCimSIBroadcastVar->tCimWarningInfo.ucIsSib10Broadcasting = 1;
    }
    }

    
    /*sib11 �澯���͡�*/
    /*��ʶ��ι㲥�Ѿ��·���ȥ����Ҫ�ѱ��εķ��͵ı�־λ��0 �ȴ��´θ澯����,������ظ澯���ͱ�ʾ��1*/
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
            /*�����澯��ʱ��*/    
            CimSetWarningTimer(&(ptCimSIBroadcastVar->tCimWarningInfo.tThisTimeCpmCcmWarnMsg),11);
        }
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        ptCimSIBroadcastVar->tSIBroadcastDate.wAllSib11Len = 0;
        pucMsg = (BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff);
        wMsgLe = ptCimSIBroadcastVar->tSIBroadcastDate.wCimSib11MsgLength + sizeof(T_RnlcRnluSystemInfoHead) + sizeof(T_SystemInfoETWS_up);

        /*����Ҫ��ʵ���û����Ӧ��ʱ��*/
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
        /* ������� */
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

    /*0 ��ʾɾ����Ӧ�ĸ澯��Ϣ*/
    if(0 == ucOperatType)
    {
        for (WORD16 wCmasIndex = 0; wCmasIndex<MAX_CMAS_MSG; wCmasIndex++)
        {
            if((wCmasMsgId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID)&&
            (wCmasStreamId == ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId))
            {
                /*��Ӧ��ʾ��0*/
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].aucIsCurrentBroadcasting = 0;
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasStreamId = 0;
                ptCimSIBroadcastVar->tCimWarningInfo.tCmasCurrentState[wCmasIndex].wCmasMsgID = 0;
                /*���·����б�*/
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
    /*1 ��ʾ���Ӷ�Ӧ�ĸ澯��Ϣ*/
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
* ��������: CimSetSystemInfoTimer
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSetSystemInfoTimer(VOID)
{
    WORD32   dwTimerId = INVALID_TIMER_ID;
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = NULL;

    /* ��ȡCIM�㲥��Ϣ */
    ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    /* ����С��ID��ȡTimerId */
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
* ��������: CimKillSystemInfoTimer
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
* ��������: CimSetWarningTimer
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimSetWarningTimer(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsgReq, BYTE ucTimerType)
{
    T_CIMVar     *ptCIMVar = NULL;
    WORD32       dwResult  = RNLC_SUCC;
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarnMsgReq);
    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    switch (ucTimerType)
    {
        case 10:
        {
            if(PWS_TYPE_ETWS == GetWarningType(ptCpmCcmWarnMsgReq->tMessageIdentifier))
            {
                /* ������͵�����֪ͨ��Ϣ��
                ��Repetition Period=0��Number of Broadcast Request=1������Primary֪ͨ
                ��˲�������֪ͨ��ʱ�� */

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
                /* ������͵��Ǹ�֪ͨ��Ϣ��������֪ͨ��ʱ�� */
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
* ��������: CimKillWarningSib10Timer
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimKillWarningSib10Timer()
{
    WORD32   dwResult = RNLC_SUCC;
     /* ��ȡ�㲥���������� */
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
* ��������: CimKillWarningSib11Timer
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimKillWarningSib11Timer()
{
    WORD32   dwResult = RNLC_SUCC;
    /* ��ȡ�㲥���������� */
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
* ��������: CimSendMsgToOtherComponent
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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

    /* ��������������Ӧ��Ϣ */
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
* ��������: CimNmlBrdUpdDealWithSib10Info
* ��������: ������ͨ�㲥����ʱ�Ƿ���дsib10��Ϣ������
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimNmlBrdUpdDealWithSib10Info(T_AllSibList  *ptAllSibList, T_SysInfoUpdReq  *ptSysInfoUpdReq)
{
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK_BOOL(ptAllSibList);
    CCM_NULL_POINTER_CHECK_BOOL(ptSysInfoUpdReq);
    SchedulingInfoList tSchedulingInfoList;
    WORD32 dwResult = 0;

    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
      /*ֻ�е���ͨ�㲥���£�����֮ǰsib10���ڷ���ʱ�Ŵ���*/
    if(TRUE == CimNormalBroadMsgWithSib10())
    {
        /*��ȷ���˴���ͨ�㲥�����Ƿ����޸ĵ�sib10�ĵ�����Ϣ������������Ҫ�ж�*/
        if(TRUE == ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /*ע������ʹ�õ����ϴγɹ����͵ĸ澯����Ϣ��д��sib10 ��Ϣ*/
            TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg = (TCpmCcmWarnMsgReq *)(&ptCimSIBroadcastVar->tCimWarningInfo.tLastCpmCcmWarnMsg);
            BOOL bResultSib10 = CimFillSib10Info(ptCpmCcmWarnMsg, ptSysInfoUpdReq, ptAllSibList);
            if ( !bResultSib10)
            {
                ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10 = 0;
                CCM_CIM_LOG(RNLC_WARN_LEVEL, "\n SI: Normal SI Update with sib10 sending  Fill SIB10 info fail!\n");  
                CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType10);

                tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
                CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
                /*������õ�����Ϣ��������Ӱ���ֵ����sib10����ʱ��ͨ�㲥����
                �ж�sib10��Ϣ����д���*/
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
* ��������: CimWarnMsgHander
* ��������: ����etws�澯��Ϣ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
* ��������: CimWarnMsgHander
* ��������: ����etws�澯��Ϣ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
            /* ����������Ϣ */
            CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType11);

            tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
            CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        }
    }
    else
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11 = 0;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib11Hander FAIL ,because there is no sib11 info!(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);  
        /* ����������Ϣ */
        CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType11);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    }
    return RNLC_SUCC;
}


/*<FUNC>***********************************************************************
* ��������: CimWarnMsgHander
* ��������: ����etws�澯��Ϣ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
    /*��s1�ڵĸ澯�������Ѿ��������Ƿ���sib12�������жϣ��������ֱ����д*/
    BOOL  bResult = CimFillSib12Info(ptCpmCcmWarnMsg, ptCmasWarningMsgSegmentList);
    if ( !bResult)
    {
        ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib12 = 0;
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: CimWarnMsgSib11Hander FAIL ,because there is no sib11 info!(%u)\n",ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11);  
        /* ����������Ϣ */
        CimAdjustScheInfoList(&ptAllSibList->tSib1Info.schedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
        tSchedulingInfoList = ptAllSibList->tSib1Info.schedulingInfoList;
        CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
        return RNLC_FAIL ;
    }
    /* ��������sib12�ĵ�һƬ��Ϣ������ûɶ�ã���ʱ�ȷ��� */
    memmove(&ptAllSibList->tSib12Info, &ptCmasWarningMsgSegmentList->atSib12[0], sizeof(SystemInformationBlockType12_r9));
    return RNLC_SUCC;
    
}

/*<FUNC>***********************************************************************
* ��������: CimIsBoardIncludeSib10
* ��������:�ж��Ƿ���sib10
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
* ��������: CimFillSib10Info
* ��������:���sib10������
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
        /* ��д��֪ͨ */
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
    /* ��д��֪ͨ */
    memmove(&ptAllSibList->tSib10Info, &tSib10, sizeof(SystemInformationBlockType10));
    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimSecondaryWarningInfoHandle
* ��������:���sib11����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
    /* �ж��Ƿ����sib11 */
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
        /* ��������sib11��Ϣ */
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
* ��������: CimIsBoardIncludeSib11
* ��������:�Ƿ���sib11
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
    /*����Э��413��ӵĶ����޷��ͳ�����֧��*/
    if((1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    &&(0 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest)
    &&(0 != ptCpmCcmWarnMsg->wRepetitionPeriod))
    {
        return TRUE;
    }
    /*��r9Э�������¼ӵ�����ֶ���Ҫ�����ж�*/
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
* ��������: CimIsBoardIncludeSib12
* ��������:�Ƿ���sib12
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
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
    /*����Э��413��ӵĶ����޷��ͳ�����֧��*/
    if((1 == ptCpmCcmWarnMsg->m.bitConcurrentWarningMessageIndicatorPresent)
    &&(0 == ptCpmCcmWarnMsg->wNumberofBroadcastRequest)
    &&(0 != ptCpmCcmWarnMsg->wRepetitionPeriod))
    {
        return TRUE;
    }
    /*��r9Э�������¼ӵ�����ֶ���Ҫ�����ж�*/
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
* ��������: CimFillSib11Info
* ��������:���sib11
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimFillSib11Info(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TWarningMsgSegmentList *ptWarningMsgSegList)
{
    WORD32                   dwSegNum                        = 0;/* ��Ƭ���� */
    WORD32                   dwLastSegSize                   = 0;/* ���һƬ��С */
    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptWarningMsgSegList);
    if (0==ptCpmCcmWarnMsg->tWarningMessageContents.numocts)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Warning Message Contents is 0!\n");
        return FALSE;
    }
    /* ��Ƭ���� */
    dwSegNum = (ptCpmCcmWarnMsg->tWarningMessageContents.numocts + g_dwS1WarnMsgLen - 1) / g_dwS1WarnMsgLen;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB11 SegNum is %d! \n", dwSegNum);
    if (RNLC_CCM_MAX_SEGMENT_NUM<dwSegNum)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 seg num is %d!\n", dwSegNum);
        return FALSE;
    }
    /* ���һ����Ƭ�Ĵ�С */
    dwLastSegSize = ptCpmCcmWarnMsg->tWarningMessageContents.numocts%g_dwS1WarnMsgLen;
    if ( (0 == dwLastSegSize) && (0 != ptCpmCcmWarnMsg->tWarningMessageContents.numocts) )
    {
        dwLastSegSize = g_dwS1WarnMsgLen;
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:dwLastSegSize = %u \n",dwLastSegSize);
    /* ����Ƭ��Ϣ��д */
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
        /* ��һ����Ƭ������дcodingscheme */
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

        /* ��дmessageidentifier��serial number */
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
* ��������: CimFillSib12Info
* ��������:���sib11
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimFillSib12Info(TCpmCcmWarnMsgReq *ptCpmCcmWarnMsg,
        TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList)
{
    WORD32                   dwSegNum                        = 0;/* ��Ƭ���� */
    WORD32                   dwLastSegSize                   = 0;/* ���һƬ��С */

    CCM_NULL_POINTER_CHECK_BOOL(ptCpmCcmWarnMsg);
    CCM_NULL_POINTER_CHECK_BOOL(ptCmasWarningMsgSegmentList);
    if (0==ptCpmCcmWarnMsg->tWarningMessageContents.numocts)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: CMAS Warning Message Contents is 0!\n");
        return FALSE;
    }
    /* ��Ƭ���� */
    dwSegNum = (ptCpmCcmWarnMsg->tWarningMessageContents.numocts + g_dwS1WarnMsgLen - 1) / g_dwS1WarnMsgLen;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI: SIB12 SegNum is %d!\n", dwSegNum);
    if (RNLC_CCM_MAX_SEGMENT_NUM<dwSegNum)
    {
        CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 seg num is %d!\n", dwSegNum);
        return FALSE;
    }
    /* ���һ����Ƭ�Ĵ�С */
    dwLastSegSize = ptCpmCcmWarnMsg->tWarningMessageContents.numocts%g_dwS1WarnMsgLen;
    if ( (0 == dwLastSegSize) && (0 != ptCpmCcmWarnMsg->tWarningMessageContents.numocts) )
    {
        dwLastSegSize = g_dwS1WarnMsgLen;
    }
    CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n SI:dwLastSegSize = %u \n",dwLastSegSize);

    /* ����Ƭ��Ϣ��д */
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
        /* ��һ����Ƭ������дcodingscheme */
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

        /* ��дmessageidentifier��serial number */
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
* ��������: CimEncodeAllSegSi
* ��������: �����Ƭ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllSegSi(TWarningMsgSegmentList   *ptWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation_r8_IEs   *ptSystemInfoR8 = NULL;
    SystemInformation          tSystemInfo;  /* ϵͳ��ϢSI����ṹ */
    ASN1CTXT                   tCtxt;         /* ����������Ľṹ */
    WORD32                     dwSiNumLoop;  /* SI�������� */
    WORD16                     wMsgLen;   /* SI�������� */
    WORD16                     wTbSize;   /* SI����ƥ���TB���С */
    WORD32                     dwResult = 0;
    WORD16                     wCurETWSLen = 0;
    WORD16                      wAlignmentLen = 0;
    /*��������Ǹ��ݽṹ���㣬�ǹ̶���*/
    CCM_NULL_POINTER_CHECK_BOOL(ptWarningMsgSegmentList);
    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    /*����һ����־λ����ʶ�Ƿ�ȫ����Ƭ��si������ʧ��*/
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
            /* �쳣̽���ϱ� */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Encoded. SI-%d which include SIB11 encode fail!\n",dwSiNumLoop);
            continue ;
        }
        else
        {
            /* SI ����ɹ�����,�����ж������������Ƿ���ϵ������� */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                /* SI�������������ȳ�����CMAC�ĵ������� */
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB11 Encoded. SegNum-%d stream len(=%u) beyond TB size boundary!\n",
                dwSiNumLoop + 1, wMsgLen);
                continue;
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB11 Encoded. SegNum-%lu stream len(=%lu) with TB size(=%lu)\n",
                dwSiNumLoop + 1, wMsgLen, wTbSize);

                /* SI����ɹ���CMAC�����������ȣ�����ʵ�ʵı��뱣�����������ֽڶ�������⿿�������ĳ��ȱ�֤ */
                ptWarningMsgSegmentList->tAllSib11SegSiList.atSiEcode[dwSiNumLoop].ucValid = TRUE;
                wAlignmentLen = (WORD16)CimEncodeAlignment(wTbSize); 
                /*����һ��Ҫһ���ڴ�Խ��ı���*/
                if((wCurETWSLen + wAlignmentLen) > CIM_SYSINFO_MSG_SIB11_BUF_LEN)
                {
                  /*��������˳��ȣ�����ֱ�ӷ��ش���*/
                  CCM_CIM_LOG(RNLC_ERROR_LEVEL,"\n SI: WARN MSg BUF OVER FLOW!\n");
                  return FALSE;
                }
                /*��Ʊ��*/
                memmove((BYTE *)(ptCimSIBroadcastVar->tSIBroadcastDate.aucCimSib11Buff + sizeof(T_RnlcRnluSystemInfoHead ) + sizeof(T_SystemInfoETWS_up)  + wCurETWSLen ),
                        &(tCtxt.buffer.data[0]), wMsgLen);

                /* ��ӡ��������Ϣ */
                if (1 == g_dwCimPrintFlag)
                {
                    CimDbgPrintMsg(&(tCtxt.buffer.data[0]),wMsgLen);
                }  
                
                wCurETWSLen = wCurETWSLen + wAlignmentLen;

                /* SI�ĳ���ȡΪƥ���TB���С */
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
* ��������: CimEncodeAllCmasSegSi
* ��������: �����Ƭ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.0
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BOOL CCM_CIM_SIBroadcastComponentFsm::CimEncodeAllCmasSegSi(TCmasWarningMsgSegmentList  *ptCmasWarningMsgSegmentList)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    SystemInformation_r8_IEs   *ptSystemInfoR8 = NULL;
    SystemInformation          tSystemInfo;  /* ϵͳ��ϢSI����ṹ */
    ASN1CTXT                   tCtxt;         /* ����������Ľṹ */
    WORD32                     dwSiNumLoop;  /* SI�������� */
    WORD16                     wMsgLen;   /* SI�������� */
    WORD16                     wTbSize;   /* SI����ƥ���TB���С */
    WORD32                     dwResult = 0;

    CCM_NULL_POINTER_CHECK_BOOL(ptCmasWarningMsgSegmentList);
   /*����һ����־λ����ʶ�Ƿ�ȫ����Ƭ��si������ʧ��*/
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
            /* �쳣̽���ϱ� */
            CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 Encoded. SI-%d which include SIB12 encode fail!\n",dwSiNumLoop);
        }
        else
        {
            /* SI ����ɹ�����,�����ж������������Ƿ���ϵ������� */
            wMsgLen = (WORD16)pu_getMsgLen(&tCtxt);
            wTbSize = (WORD16)CimToMacAlignment(wMsgLen);
            if (0 == wTbSize)
            {
                /* SI�������������ȳ�����CMAC�ĵ������� */
                CCM_CIM_LOG(RNLC_ERROR_LEVEL, "\n SI: SIB12 Encoded. SegNum-%d stream len(=%u) beyond TB size boundary!\n",
                dwSiNumLoop + 1, wMsgLen);
            }
            else
            {
                CCM_CIM_LOG(RNLC_INFO_LEVEL,"\n SI: SIB12 Encoded. SegNum-%lu stream len(=%lu) with TB size(=%lu)\n",
                dwSiNumLoop + 1, wMsgLen, wTbSize);

                /* SI����ɹ���CMAC�����������ȣ��򱣴����� */
                ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].ucValid = TRUE;
                memcpy(ptCmasWarningMsgSegmentList->tAllSib12SegSiList.atSiEcode[dwSiNumLoop].aucMsg,
                &(tCtxt.buffer.data[0]), wMsgLen);

                /* SI�ĳ���ȡΪƥ���TB���С */
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

    /* С��ID */
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
* ��������: CimCellDelAddSendOldSIToRnlu(VOID)
* ��������: С��ɾ��ʱ����һ��֮ǰ������si�����������еľɹ㲥����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
        /*��������ʽȫ�����½ӿ�ͷ��Ϣ�е�si upd cause �� cellinbroad�ֶ�*/
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
            /*�����幹����״̬��������*/
            TCimComponentInfo     *ptCimComponentInfo = \
                               (TCimComponentInfo *)pMsg->m_pSignalPara;

            ucMasterSateCpy = ptCimComponentInfo->ucState;
            CCM_CIM_LOG(DEBUG_LEVEL,"\n SI: CCM_CIM_SIBroadcastComponentFsm get State %d.\n",ucMasterSateCpy);
            /* ��ȡCIM�㲥��Ϣ,���㲥���͵�valuetag���浽����Ĺ㲥���������� */
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
* ��������: HandleMasterToSlave
* ��������: ��ת������������slave̬
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
* ��������: HandleSlaveToMaster
* ��������: ��ת��ʱ�Ĳ��������¶�ȡ���ݿ����ɹ㲥��Ϣ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::HandleSlaveToMaster(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);

        /*�Ƚ�SI ����״̬ת��IDLE*/
        TranStateWithTag(CCM_CIM_SIBroadcastComponentFsm, Idle,S_CIM_SIB_IDLE);
    
    Message tCCMasterSlave;
    T_SysInfoUpdReq tSysinfoUpdReq;

    /* ��ȡ�㲥���������� */
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);

    tSysinfoUpdReq.wCellId = ptCimSIBroadcastVar->wCellId;
    tCCMasterSlave.m_wSourceId = ID_CCM_CIM_SIBroadcastComponent;
    tCCMasterSlave.m_wLen = sizeof(tSysinfoUpdReq);
    tCCMasterSlave.m_pSignalPara = static_cast<void*>(&tSysinfoUpdReq);
    tCCMasterSlave.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;
    /*��ʱû�п��ǽ��澯��״̬������ͬ��������*/
    Handle_SystemInfoUpdateReq(&tCCMasterSlave);    
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimRecordToRNLUDbgInfo
* ��������: ��ת��ʱ�Ĳ��������¶�ȡ���ݿ����ɹ㲥��Ϣ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CCM_CIM_SIBroadcastComponentFsm::CimRecordToRNLUDbgInfo(void)
{
    WORD32 dwIndex = 0;
    WORD32 dwTemIndex = 0;
    T_CIMVar *ptCIMVar                       = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    T_CimSIBroadcastVar *ptCimSIBroadcastVar = (T_CimSIBroadcastVar*)&(GetComponentContext()->tCimSIBroadcastVar);
    
    /*�ҵ�����������ͷ��Ϣ*/
    T_RnlcRnluSystemInfoHead *ptSystemInfo   = (T_RnlcRnluSystemInfoHead*)ptCimSIBroadcastVar->tSIBroadcastDate.aucCimMsgBuff;
    /*�����С������ԭ�򣬲��Ҳ���С��ɾ������Ľ���������α���������ĵ�һ��Ԫ����*/
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
        /*��С��������ԭ�������δ洢����Ĵεļ�¼*/
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
            /*�����¼�Ѿ���������1.ǰ�����һ����¼֮ǰ�ļ�¼*/
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
           
            /*���һ����¼����¼Ϊ���εļ�¼*/
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].ucCellInBroad   = ptSystemInfo->ucCellIdInBoard;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].wCellId         = ptSystemInfo->wCellId;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].wRNLUSIUpdCause = ptSystemInfo->wSystemInfoCause;
            ptCIMVar->tRNLUSiInfoRecord[SI_TO_RNLU_INFO_NUM - 1].ucUsedFlag      = 1;      
        }
        else     
        {
            /*�����¼��û��������������һ���հ׵ļ�¼��д���μ�¼*/
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].ucCellInBroad   = ptSystemInfo->ucCellIdInBoard;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].wCellId         = ptSystemInfo->wCellId;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].wRNLUSIUpdCause = ptSystemInfo->wSystemInfoCause;
            ptCIMVar->tRNLUSiInfoRecord[dwIndex].ucUsedFlag      = 1;
        }
    }

    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* ��������: CimGetAllSystemInfo
* ��������: a.��MIB/SIB1/SIB2�Ļ�ȡʧ�ܣ�ֹͣ�㲥�������̣�������Ϣ��SIB1��
*           b.��ȡ������Ϣ�ɹ������ǲ�����SIB2��ֹͣ�㲥����
*           c.���ݵ�����Ϣ��ȡSIB3~SIb9��ص����ݱ�
*             ��ʧ�ܸ��µ�����Ϣ�����·���Ӧ��SIB����ӡ�澯��Ϣ���㲥��Ϣ��������
*           d.����SIB1�µĵ����б�:�����ݿ��е�����Ϣ�µ�����SIB���ڻ�ȡ��Ϣʧ�ܣ�
*             ��Ҫ�������б��µĴ�SIɾ��
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

        /* 1.��ȡMIB��Ϣ */
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

        /* 2.��ȡSIB1��Ϣ */
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
        /* �޸�SISCH��Ϣ */
        CimDbgConfigSISch(&tGetSib1InfoAck);

        /* 3.��װ������Ϣ */
        dwResult = CimGetSIScheInfo(&tGetSib1InfoAck, aucSibList, &tSchedulingInfoList);
        if (FALSE == dwResult)
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: CIM Get SISCHE Info fail!\n");
            return FALSE;
        }

        /* ���û��Sib2�����·��㲥 */
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

        /* ��Ϊsetup��һ��ָ�룬���������Ҫ����ָ�������ָ�򣬷���Ϊ��ָ�� */
        ptAllSibList->tSib2Info.radioResourceConfigCommon.soundingRS_UL_ConfigCommon.u.setup =
            &(ptAllSibList->tSibPointStruct.tSrsUlCfgSetUp);

        /* 4.��ȡSIB2��Ϣ */
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

        /* 5.��ȡSIB3��Ϣ */
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
                /* ���������б� */
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

        /* 6.��ȡSIB4��Ϣ *//*lint -save -e690 */
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
                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType4);
            }

            dwResult = CimFillSib4Info(wCellId, ucRadioMode, &tGetSib4InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* ���������б� */
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

        /* 7.��ȡSIB5��Ϣ */
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
                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType5);
            }

            dwResult = CimFillSib5Info(wCellId, ucRadioMode, &tGetSib5InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType5);
            }
        }
    }

    {
        T_DBS_GetSib6Info_REQ tGetSib6InfoReq;
        T_DBS_GetSib6Info_ACK tGetSib6InfoAck;

        memset((VOID*)&tGetSib6InfoReq, 0, sizeof(tGetSib6InfoReq));
        memset((VOID*)&tGetSib6InfoAck, 0, sizeof(tGetSib6InfoAck));

        /* 8.��ȡSIB6��Ϣ */
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
                /* ���������б� */
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

        /* 9.��ȡSIB7��Ϣ */
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
                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType7);
            }

            dwResult = CimFillSib7Info(wCellId, &tGetSib7InfoAck, ptAllSibList);
            if (FALSE == dwResult)
            {
                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType7);
            }
        }
    }

    {
        T_DBS_GetSib8Info_REQ tGetSib8InfoReq;
        T_DBS_GetSib8Info_ACK tGetSib8InfoAck;

        memset((VOID*)&tGetSib8InfoReq, 0, sizeof(tGetSib8InfoReq));
        memset((VOID*)&tGetSib8InfoAck, 0, sizeof(tGetSib8InfoAck));

        /* 10.��ȡSIB8��Ϣ */
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
                /* ���������б� */
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

        /* 11.��ȡSIB9��Ϣ */
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

                /* ���������б� */
                CimAdjustScheInfoList(&tSchedulingInfoList, aucSibList, sibType9);
            }

            CimFillSib9Info(&tGetSib9InfoAck, ptAllSibList);
        }
    }

    /* ���µ�����Ϣ */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);
    memcpy(ptAllSibList->aucSibList, aucSibList, sizeof(aucSibList));
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId, "\n SI: Fill Normal SIB Info Finish!\n");
    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimEncodeAllSibs_Warning
* ��������: a.������MIB/SIB1/SIB2ʧ�ܣ���ֹͣ�㲥�·�
*           b.������SIB3~SIB9ʱ��������ĳ��SIB����ʧ�ܣ�����������б����·���SIB
*           c.����SIB1�µĵ����б�:�����ݿ��е�����Ϣ�µ�����SIB���ڱ���ʧ�ܣ�
*             ��Ҫ�������б��µĴ�SIɾ��
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: WORD32
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* MIB���� */
    dwResult = CimEncodeFunc(CIM_SI_MIB, &tCtxt, (VOID *)(&ptAllSibList->tMibInfo));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucMibLen = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucMibBuf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucMibLen);

        if (CIM_MIB_BUF_LEN < ptNewBroadcastStream->ucMibLen)
        {
            /* ����Խ�� */
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

    /* SIB1���� */
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);

        /* ucSib1ValueTagOffset��ʾValueTag��SIB1�������һ���ֽڵ�ValueTag��ʼλ�õ�ƫ��ֵ��
        8��ʾÿ���ֽڵ�bit����5��ʾValueTag��ռ��bitλ�� */
        ptNewBroadcastStream->ucSib1ValueTagOffset = (BYTE)((8 - tCtxt.buffer.bitOffset) + 5);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);
        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* ����Խ�� */
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

    /* SIB2���� */
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
            /* ����Խ�� */
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

    /* ����SIB���� */
    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB3])
    {
        /* SIB3���� */
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
                /* ����Խ�� */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB3_LEN=%d over CIM_MAX_STREAM_LEN.\n", wTempLen);
                dwReturn = 3;
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: NCODE_SIB3 fail.\n");
            /* ���������б� */
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType3);
            dwReturn = 3;
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB4])
    {
        /* SIB4���� */
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
                /* ����Խ�� */
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
        /* SIB5���� */
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
                /* ����Խ�� */
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
        /* SIB6���� */
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
                /* ����Խ�� */
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
        /* SIB7���� */
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
                /* ����Խ�� */
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
        /* SIB8���� */
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
                /* ����Խ�� */
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
        /* SIB9���� */
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
                /* ����Խ�� */
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
    /*��s1�澯����sib10��Ҫ���ͺ�sib10���ڷ���ʱ����ͨ�㲥����ʱ�Ŵ���*/
    if((1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib10)||
        (1 == ptCimSIBroadcastVar->tSIBroadcastDate.ucNormalSibWithSib10Info))
    {
        if (TRUE ==  ptAllSibList->aucSibList[DBS_SI_SIB10])
        {
            /* SIB10���� */
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
                    /* ����Խ�� */
                    CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB10_LEN=%d over BCM_SIB10_BUF_LEN.\n",wTempLen);
                    dwReturn = 10;
                }
                else
                {
                    /* ��Sib10����������ʵ�������� */
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
        /*���sib10û�й�ϵ��Ϊ�˱�֤�������⣬������һ����sib1��ȥ���������Ϣ*/
        CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType10);
    }


    /******************************SIB11 start***********************************************/

    if (1 == ptCimSIBroadcastVar->tCimWarningInfo.ucSendSib11)
    {
        /*ȥ��ԭ��������sib11 ��sib12 ����Ĺ��̣���������sib���������ٷ���ptNewBroadcastStream��
        ֻ���浽cim��ʵ������*/
        /* ����SIB11 ��Ƭ */
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
        /* ����SIB12 ��Ƭ */
        dwResult = CimEnCodeSib12SegList(ptCmasWarningMsgSegmentList, ptCimData,ptCimSIBroadcastVar);
        if (FALSE == dwResult)
        {
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType12_v920);
        }
        /******************************SIB12 end*************************************************/
    }
    /* ���µ�����Ϣ */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);

    return dwReturn;
}

/*<FUNC>***********************************************************************
* ��������: CimEncodeAllSibs
* ��������: a.������MIB/SIB1/SIB2ʧ�ܣ���ֹͣ�㲥�·�
*           b.������SIB3~SIB9ʱ��������ĳ��SIB����ʧ�ܣ�����������б����·���SIB
*           c.����SIB1�µĵ����б�:�����ݿ��е�����Ϣ�µ�����SIB���ڱ���ʧ�ܣ�
*             ��Ҫ�������б��µĴ�SIɾ��
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: WORD32
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* MIB���� */
    dwResult = CimEncodeFunc(CIM_SI_MIB, &tCtxt, (VOID *)(&ptAllSibList->tMibInfo));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucMibLen = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucMibBuf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucMibLen);

        if (CIM_MIB_BUF_LEN < ptNewBroadcastStream->ucMibLen)
        {
            /* ����Խ�� */
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

    /* SIB1���� */
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);

        /* ucSib1ValueTagOffset��ʾValueTag��SIB1�������һ���ֽڵ�ValueTag��ʼλ�õ�ƫ��ֵ��
           8��ʾÿ���ֽڵ�bit����5��ʾValueTag��ռ��bitλ�� */
        ptNewBroadcastStream->ucSib1ValueTagOffset = (BYTE)((8 - tCtxt.buffer.bitOffset) + 5);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);

        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* ����Խ�� */
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

    /* SIB2���� */
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
            /* ����Խ�� */
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

    /* ����SIB���� */
    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB3])
    {
        /* SIB3���� */
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
                /* ����Խ�� */
                CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId, "\n SI: SIB3_LEN=%d over CIM_MAX_STREAM_LEN.\n",wTempLen);
                dwReturn = 3; 
            }
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: NCODE_SIB3 fail.\n");

            /* ���������б� */
            CimAdjustScheInfoList(&tSchedulingInfoList, ptAllSibList->aucSibList, sibType3);
            dwReturn = 3; 
        }
    }

    if (TRUE == ptAllSibList->aucSibList[DBS_SI_SIB4])
    {
        /* SIB4���� */
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
                /* ����Խ�� */
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
        /* SIB5���� */
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
                /* ����Խ�� */
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
        /* SIB6���� */
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
                /* ����Խ�� */
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
        /* SIB7���� */
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
                /* ����Խ�� */
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
        /* SIB8���� */
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
                /* ����Խ�� */
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
        /* SIB9���� */
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
                /* ����Խ�� */
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

    /* ���µ�����Ϣ */
    CimUpdateSib1Sche(&tSchedulingInfoList, ptAllSibList);

    return dwReturn;
}
/*<FUNC>***********************************************************************
* ��������: CimGetSIScheInfo
* ��������: ��ȡ���ݿ��R_SISCHE��ȡ���õĵ�����Ϣ������SIBӳ�䵽��Ӧ��SI��
*           ��Ϊ���ʼ�ĵ�����Ϣ���������ܵ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* �������ݿ��е�SI���ü�¼����д������Ϣ�б� */
    ptSchedulingInfoList->n = ptGetSib1InfoAck->tSib1.ucSIScheNum;

    /* ����SIB2��SI���ӵ����б��еĵ�2��SI��ʼ��д */
    dwSiLoop = 1;
    for (dwRcdLoop = 0; dwRcdLoop < ptGetSib1InfoAck->tSib1.ucSIScheNum; dwRcdLoop++)
    {
        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib2)
        {
            /* SIB2����,���ж��Ƿ��ǵ�һ��SI */
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

        /* ��ʼ��SIBӳ����Ϣ�б��е�Ԫ�ظ���Ϊ0 */
        ptSibMapInfo = &(ptSchedulingInfoList->elem[dwSiNo].sib_MappingInfo);
        ptSibMapInfo->n = 0;

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib3)
        {
            /* SIB3���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType3;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB3] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib4)
        {
            /* SIB4���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType4;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB4] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib5)
        {
            /* SIB5���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType5;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB5] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib6)
        {
            /* SIB6���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType6;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB6] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib7)
        {
            /* SIB7���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType7;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB7] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib8)
        {
            /* SIB8���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType8;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB8] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib9)
        {
            /* SIB9���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType9;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB9] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib10)
        {
            /* SIB10���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType10;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB10] = TRUE;
        }

        if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib11)
        {
            /* SIB11���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType11;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB11] = TRUE;
        }
    if (TRUE == ptGetSib1InfoAck->tSib1.atSchedulingInfo[dwRcdLoop].ucSib12)
        {
            /* SIB12���� */
            ptSibMapInfo->elem[ptSibMapInfo->n] = sibType12_v920;
            ptSibMapInfo->n++;

            pucSibsList[DBS_SI_SIB12] = TRUE;
        }    
    }

    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimAdjustScheInfoList
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimAdjustScheInfoList(SchedulingInfoList *ptSchedulingInfoList, BYTE *pucSibsList, SIB_Type eSibType)
{
    WORD32 dwSiLoop   = 0;  /* SI���� */
    WORD32 dwSibLoop  = 0;  /* ÿ��SI�е�SIB����  */
    WORD32 dwSibCount = 0;  /* ÿ��SI�е�SIB��Ŀ  */
    WORD32 dwTempLoop = 0;

    for (dwSiLoop = 0; (dwSiLoop < ptSchedulingInfoList->n) && (dwSiLoop < 32); dwSiLoop++)
    {
        /* ÿ��SI�е�SIB���� */
        dwSibCount = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n;

        for (dwSibLoop = 0; (dwSibLoop < dwSibCount) && ( dwSibLoop < 31 ); dwSibLoop++)
        {
            if (eSibType == ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop])
            {
                /* �������SIB������λ */
                for (dwTempLoop = dwSibLoop + 1; dwTempLoop < dwSibCount; dwTempLoop++)
                {
                    ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop - 1]
                    = ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop];
                }

                /* ����Чλ */
                ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwTempLoop - 1] = 0XFF;
                (ptSchedulingInfoList->elem[dwSiLoop].sib_MappingInfo.n)--;
            }
        }
    }

    /* ����ʱSIB�б���ɾ����SIB */
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
* ��������: CimGetSchePeriodInfo
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSchePeriodInfo(SchedulingInfoList *ptSchedulingInfoList,  BYTE *pucSibsList,SIB_Type eSibType)
{    

    CCM_NULL_POINTER_CHECK(pucSibsList);
    WORD32 dwSiLoop   = 0;  /* SI���� */
    WORD32 dwSibLoop  = 0;  /* ÿ��SI�е�SIB����  */
    WORD32 dwSibCount = 0;  /* ÿ��SI�е�SIB��Ŀ  */
    for (dwSiLoop = 0; (dwSiLoop < ptSchedulingInfoList->n) && (dwSiLoop < 32); dwSiLoop++)
    {
        /* ÿ��SI�е�SIB���� */
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
    /* ��ʼ���㲥ʵ�������� */
}

/*<FUNC>***********************************************************************
* ��������: CimUpdateSib1Sche
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimUpdateSib1Sche(SchedulingInfoList *ptSchedulingInfoList, T_AllSibList *ptAllSibList)
{
    SchedulingInfo  *ptSchedulingInfo = NULL;
    WORD32                dwSiNo      = 0;
    WORD32                dwSiLoop    = 0;

    ptAllSibList->tSib1Info.schedulingInfoList.n = 0;

    /* 32��ֵ��Դ:Э�鶨��ṹ */
    for (dwSiLoop = 0; dwSiLoop < 32 ; dwSiLoop++)
    {
        if(dwSiLoop < ptSchedulingInfoList->n)
        {
            /*����ptSchedulingInfoList��Ҫ��ȥ��ȥ���ģ����Ѻ�����Ǩ�Ʋ���*/
            /* ��ȡSI */
            ptSchedulingInfo = &(ptSchedulingInfoList->elem[dwSiLoop]);
            /* ��SI�ǿ�(��Ӧ��SIBs�Ѿ�ɾ��)�����ǲ��ų���һ��SIΪ�յ����(SIB2) */
            if ((0 < ptSchedulingInfo->sib_MappingInfo.n) || (0 == dwSiLoop))
            {
                memcpy(&(ptAllSibList->tSib1Info.schedulingInfoList.elem[dwSiNo]), ptSchedulingInfo, sizeof(SchedulingInfo));
                ptAllSibList->tSib1Info.schedulingInfoList.n++;
                dwSiNo = ptAllSibList->tSib1Info.schedulingInfoList.n;
            }
        }
        else
        {
            /*���Ѿ�����ptSchedulingInfoList ������ĺ���ĵ�����Ϣ���㣬
            ���Է�ֹ���ֺ����ֵ�ٱ�Ӧ�õ��쳣���*/
            if(dwSiNo > (dwSiLoop +1))
            {
                /*�������������dwsino�ǲ������dwsiloop ��1 �ģ��������Ȼ���С��*/
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
* ��������: CimFillSib2ResCfgForSuperCell
* ��������: ��bpl1�峬��С������rs�ο��ź���Ҫ��ȡ����С���������е�
                          cp���ֵ�����ܶ�ȡС�����е�����ֵ
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    /*�������������ֱ�ӷ��ش��󵥰�����*/
    if( dwResult != RNLC_SUCC)
    {
         CCM_SIOUT_LOG(RNLC_FATAL_LEVEL,wCellId,"\n SI: [SI]Call USF_GetBplBoardTypeByCellId fail!\n"); 
         return RNLC_INVALID_DWORD;  
    }
    else
    {
       if (USF_BPLBOARDTYPE_BPL1 == wBoardType)
        {
           /*  ��дPDSCH���ò���pdsch_Configurationpdsch_Configuration  ��rs�Ĳο��ź��ֶ�*/
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
                 /*��ӡ����*/
                 CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: BPL1 Get Cp Info Fail !\n");
                return RNLC_INVALID_DWORD;
            }
            TSuperCPInfo      *ptSuperCpInfo = NULL;
            /* Э��: [-60,50]��Dbs=IE+60 */
            WORD16 wMaxCellSpeRefSigPwr = 0;   
            /*ѡȡ���п���cp ��Ϣ������rs�ο��źŹ���*/
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
                /*��ʼ��Ϊ��һ��cp��rs�źŹ��ʣ���ֹ����cp�������õ����*/
                ptPdschCfg->referenceSignalPower = (tDbsGetSuperCpInfoAck.atCpInfo[0].wCPSpeRefSigPwr)/10 - 60;
            }
            else
            {
                /* Э��: [-60,50]��Dbs=IE+60 */
                ptPdschCfg->referenceSignalPower =(wMaxCellSpeRefSigPwr/10)-60;
            }
        }
    }
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* ��������: CimFillSib2RadioResCfgCommon
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* 2.1. ��дRACH����rach_Configuration */
    ptRachCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.rach_ConfigCommon);

    /* 2.1.1 ��дǰ����ϢpreambleInfo */
    /* Э��: ����ENUM={n4,n8,n12,n16,n20,n24,n28,n32,n36,n40,n44,n48,n52,n56,n60,n64} */
    ptRachCfg->preambleInfo.numberOfRA_Preambles
    = (RACH_ConfigCommon_numberOfRA_Preambles)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucNumRAPreambles;

    /* ���ݿⲻ֧�ֿ�ѡ,���� */
    ptRachCfg->preambleInfo.m.preamblesGroupAConfigPresent = 1;

    /* Э��: ����ENUM={n4,n8,n12,n16,n20,n24,n28,n32,n36,n40,n44,n48,n52,n56,n60} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA
    = (RACH_ConfigCommon_sizeOfRA_PreamblesGroupA)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucSizeRAGroupA;

    /* Э��: ����ENUM={b56,b144,b208,b256} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.messageSizeGroupA
    = (RACH_ConfigCommon_messageSizeGroupA)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucSelPreGrpThresh;

    /* Э��: ����ENUM={minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18} */
    ptRachCfg->preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB
    = (RACH_ConfigCommon_messagePowerOffsetGroupB)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMsgPwrOfstGrpB;

    ptRachCfg->preambleInfo.preamblesGroupAConfig.extElem1.numocts = 0;

    /* 2.1.2 ��д������������powerRampingParameters */
    /* Э��: ����ENUM={dB0,dB2,dB4,dB6} */
    ptRachCfg->powerRampingParameters.powerRampingStep
    = (RACH_ConfigCommon_powerRampingStep)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucPrachPwrStep;

    /* Э��: ����ENUM={dBm-120,dBm-118,dBm-116,dBm-114,dBm-112,dBm-110,dBm-108,dBm-106, */
    /*               dBm-104,dBm-102,dBm-100,dBm-98,dBm-96,dBm-94,dBm-92,dBm-90} */
    ptRachCfg->powerRampingParameters.preambleInitialReceivedTargetPower
    = (RACH_ConfigCommon_preambleInitialReceivedTargetPower)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucPreInitPwr;

    /* 2.1.3 ��д�����������Ϣra_SupervisionInformation */
    /* Э��: ����ENUM={n3,n4,n5,n6,n7,n8,n10,n20,n50,n100,n200} */
    ptRachCfg->ra_SupervisionInfo.preambleTransMax
    = (RACH_ConfigCommon_preambleTransMax)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMaxRetransNum;

    /* Э��: ����ENUM={sf2,sf3,sf4,sf5,sf6,sf7,sf8,sf10} */
    ptRachCfg->ra_SupervisionInfo.ra_ResponseWindowSize
    = (RACH_ConfigCommon_ra_ResponseWindowSize)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucRARspWinSize;

    /* Э��: ����ENUM={sf8,sf16,sf24,sf32,sf40,sf48,sf56,sf64} */
    ptRachCfg->ra_SupervisionInfo.mac_ContentionResolutionTimer
    = (RACH_ConfigCommon_mac_ContentionResolutionTimer)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMACContResTimer;

    /* 2.1.4 ��дMsg3����ش�����maxHARQ_Msg3Tx */
    ptRachCfg->maxHARQ_Msg3Tx
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tRach_ConfigCommon.ucMaxHARQMsg3Tx;
    ptRachCfg->extElem1.numocts = 0;

    /* 2.2 ��дBCCH���ò���bcch_Configuration */
    ptBcchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.bcch_Config);
    ptBcchCfg->modificationPeriodCoeff
    = (BCCH_Config_modificationPeriodCoeff_Root)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tBcch_Config.ucBcchModPrdPara;

    /* 2.3 ��дPCCH���ò���pcch_Configuration */
    ptPcchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pcch_Config);
    ptPcchCfg->defaultPagingCycle
    = (PCCH_Config_defaultPagingCycle)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPcch_Config.ucPagDrxCyc;
    ptPcchCfg->nB
    = (PCCH_Config_nB)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPcch_Config.ucnB;

    /* 2.4 ��дPRACH���ò���prach_Configuration */
    ptPrachCfgSib = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.prach_Config);
    ptPrachCfgSib->rootSequenceIndex
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.wLogRtSeqStNum;

    ptPrachCfgSib->prach_ConfigInfo.prach_ConfigIndex
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucPrachConfig;

    /* Э��: ����BOOL  */
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

    /* 0903Э��仯:ֵ��(0..104)��Ϊ(0..94) */
    ptPrachCfgSib->prach_ConfigInfo.prach_FreqOffset
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPrach_Config.ucPrachFreqOffset;

    /* 2.5 ��дPDSCH���ò���pdsch_Configurationpdsch_Configuration */
    ptPdschCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pdsch_ConfigCommon);

    /* Э��: [-60,50]��Dbs=IE+60 */
    ptPdschCfg->referenceSignalPower
    = (WORD16)(ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPdsch_ConfigCommon.wCellSpeRefSigPwr/10) - 60;
    ptPdschCfg->p_b
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPdsch_ConfigCommon.ucPB;

    /* 2.6 ��дPUSCH���ò���pusch_Configuration */
    ptPuschCfgBasic = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pusch_ConfigCommon.pusch_ConfigBasic);
    ptPuschCfgBasic->n_SB
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschNsb;
    ptPuschCfgBasic->hoppingMode
    = (PUSCH_ConfigCommon_hoppingMode)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschFHpMode;

    /* 0903Э��仯: (0..63)��Ϊ(0..98) */
    ptPuschCfgBasic->pusch_HoppingOffset
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucPuschhopOfst;

    /* Э��: ����BOOL {0:Not support; 1:Support} */
    if (0 == ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPusch_ConfigCommon.ucUl64QamDemSpInd)
    {
        ptPuschCfgBasic->enable64QAM = FALSE;
    }
    else
    {
        ptPuschCfgBasic->enable64QAM = TRUE;
    }


    ptUlRefSignal = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pusch_ConfigCommon.ul_ReferenceSignalsPUSCH);

    /* Э��: ����BOOL {0:Disable; 1:Enable} */
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

    /* 2.7 ��дPUCCH���ò���pucch_Configuration */
    ptPucchCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.pucch_ConfigCommon);
    ptPucchCfg->deltaPUCCH_Shift
    = (PUCCH_ConfigCommon_deltaPUCCH_Shift)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucPucchDeltaShf;
    ptPucchCfg->nRB_CQI
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucNumPucchCqiRB;
    ptPucchCfg->nCS_AN
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.ucPucchNcsAn;

    /* mXA0003551:SR�ŵ���+�뾲̬A/N�ŵ���+A/N Repetition�ŵ��� */
    ptPucchCfg->n1PUCCH_AN
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchSemiAn +
      ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchSr +
      ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tPucch_ConfigCommon.wNPucchAckRep;

    /* 2.8 ��д����Sounding�������ò���soundingRsUl_ConfigCommon */
    ptSrsUlCfg = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.soundingRS_UL_ConfigCommon);

    /* Э��: TS36.331v850 ����CHOICE(release,setup) */
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

        /* ��ѡ(Condition of TDD) */
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

    /* 2.9 ��д������·�������ò���uplinkPowerControlCommon */
    ptUlPowCtrl = &(ptAllSibList->tSib2Info.radioResourceConfigCommon.uplinkPowerControlCommon);

    /* Э��: [-126,24]��Dbs = IE+126 */
    ptUlPowCtrl->p0_NominalPUSCH
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucPoNominalPusch1 - 126;

    ptUlPowCtrl->alpha
    = (UplinkPowerControlCommon_alpha)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucAlfa;

    /* Э��: [-127,-96]��Dbs = IE+127*/
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

    /* Э��: [-1,6]��Dbs = IE+1 */
    ptUlPowCtrl->deltaPreambleMsg3
    = ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.tUplinkPowerControlCommon.ucdtaPrmbMsg3 - 1;

    /* 2.10 ul_CyclicPrefixLength */
    ptAllSibList->tSib2Info.radioResourceConfigCommon.ul_CyclicPrefixLength
    = (UL_CyclicPrefixLength)ptGetSib2InfoAck->tSib2.tRadioResourceConfigCommon.ucPhyChCPSel;

    ptAllSibList->tSib2Info.radioResourceConfigCommon.extElem1.numocts = 0;

    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimGetAllowMeasBwOfFreq
* ��������: ��ȡͬƵ���������������С����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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


    /* û��������Ϣ����ȡĬ�ϵ���Сֵ */
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

    /* ��ʼ�� */
    ucMinDlBw = mbw100 + 1;

    /* ��С�������жϲ���¼ */
    for (dwNbrCellLoop = 0;
            (dwNbrCellLoop < dwNbrCellNum) && ( dwNbrCellLoop < DBS_MAX_INTRA_FREQ_NBRNUM_PER_CELL);
            dwNbrCellLoop++)
    {
        if (1 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucBlackNbrCellInd)
        {
            /* �����Ǻ�����С�� */
            continue;
        }

        /* �����eNB�ڵ�С�������ȡSRVCEL��ȡ����ز��� */
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

            /* ��ȡ��������Ƶ�ʺ�ϵͳ���� */
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
            /* ��������eNBС����ȡADJCEL��ȡ����ز��� */
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

            /* ��ȡ��������Ƶ�ʺ�ϵͳ���� */
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
* ��������: CimFillSib7NccPermitted
* ��������: ��дSIB7��Permitted NCC��Ϣ
* �㷨����: 1.��ȡ��������Ϣ������NCC��ARFCN��Ƶ�α�ʶ;
*           2.�ر�ģ��������С����ARFCNλ��[512,810]������Ҫ�жϸ���С����Ƶ�α�ʶ�Ƿ�
*           �͵�ǰARFCN���ϵ�Ƶ�α�ʶ��ͬ, ���ARFCNֵλ��[512,810]����ôUE���ֲ�������
*           DCS1800Ƶ�λ���PCS1900Ƶ��
*           3.�������С����ARFCN���ڵ�ǰ��ARFCN����(����ԪcarrierFreqs)������
*           ��������NCCΪ��������ncc_permitted����Ӧ��bitλ��1��
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
        /* ���������ı�ʶ��ѯ��NCC */
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
            /* �����С����ARFCNλ��[512,810]���������ж�Ƶ��ָʾ */
            if (((512 <= tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN) && (tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN <= 810)) &&
                    (ptCarrierFreqsGERAN->bandIndicator != tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.ucBandIndicator))
            {
                /* ���Ƶ�β�һ�������ٿ��Ǵ�С����NCC */
                continue;
            }

            /* �жϸ�С����Ƶ���Ƿ��ڵ�ǰƵ�㼯���� */
            if (ptCarrierFreqsGERAN->startingARFCN == tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.wBCCHARFCN)
            {
                /* �������ʼARFCN��ͬ���򽫶�Ӧ��bitλ��1�������Ҷ�ӦNCC[0~7] */
                *pucNccPermitted |= 0X80 >> tGetGSMAdjCellInfoByCellIdAck.tGSMAdjCellInfo.ucNCC;
                continue;
            }

            /* to do:����ʣ���ARFCN�б��в�ѯ,��ΪĿǰ���ݿ��ݲ�֧��ʣ��ARFCN����д */
        }
    }

    return;
}

/*<FUNC>***********************************************************************
* ��������: CimGetBandLocation
* ��������: �ж�Ƶ��byBandClass�Ƿ���Ƶ���б�ptBandList�д��ڣ�������ڣ�
*           pdwBandLocation��¼�±�λ�ã�����TRUE�������ڷ���FALSE
* �㷨����:
* ȫ�ֱ���:
* Input����:
* �� �� ֵ:
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetBandLocation(BYTE ucBandClass, NeighCellListCDMA2000 *ptBandList, WORD32 *pdwBandLocation)
{
    WORD32 dwLoop      = 0;
    WORD32   dwBandExist = RNLC_FAIL;

    /* �ж�Ƶ���Ƿ���� */
    for (dwLoop = 0; (dwLoop < ptBandList->n) && (dwLoop < 16); dwLoop++)
    {
        if (ucBandClass == ptBandList->elem[dwLoop].bandClass)
        {
            /* ��¼Ƶ��λ�� */
            *pdwBandLocation = dwLoop;
            dwBandExist = RNLC_SUCC;
        }
    }

    return dwBandExist;
}

/*<FUNC>***********************************************************************
* ��������: BcmdaFillSib8NeighCellList
* ��������: �ж�Ƶ��wFreq�Ƿ���Ƶ���б�ptFreqList�д��ڣ�������ڣ�
            pwFreqLocation��¼�±�λ�ã�����TRUE�������ڷ���FALSE
* �㷨����:
* ȫ�ֱ���:
* Input����:
* �� �� ֵ:
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
BYTE CimGetFreqLocation(WORD16 wFreq, NeighCellsPerBandclassListCDMA2000 *ptFreqList,
                        WORD32 *pwFreqLocation)
{
    WORD32 dwLoop = 0;
    BYTE   ucFreqExist = FALSE;

    /* Ƶ���Ƿ��ѱ���� */
    for (dwLoop = 0; (dwLoop < ptFreqList->n) && (dwLoop < 16); dwLoop++)
    {
        if (wFreq == ptFreqList->elem[dwLoop].arfcn)
        {
            /* ��¼Ƶ��λ�� */
            *pwFreqLocation = dwLoop;
            return ucFreqExist;
        }
    }

    return ucFreqExist;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib8CdmaNbrList
* ��������: ��CDMANBR���е�������ʶΪ������CDMAADJCEL�����ҵ���������������Ϣ:bandclass,
*           arfcn��phycellID,Ȼ����bandclassΪ�����дarfcn�б�,����arfcnΪ�����д
*           С���б�
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
        /* ��ȡCDMANBR */
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

        /* �жϸ�Ƶ���Ƿ��Ѿ���� */
        dwBandExist = CimGetBandLocation(tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.ucBandClass,
                                        ptNbrElement, &dwBandLocation);
        if (RNLC_SUCC == dwBandExist)
        {
            /* �жϸ�����Ƶ���Ƿ��Ѿ���� */
            ucFreqExist = CimGetFreqLocation(tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq,
                         &ptNbrElement->elem[dwBandLocation].neighCellsPerFreqList,&dwFreqLocation);

            if (TRUE == ucFreqExist)
            {
                /* ���Ӹ�Ƶ���µ����� */
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
                /* Ƶ��û�������С��һ��û����������Ӹ�Ƶ�㣬�����Ӹ�Ƶ���µ�С�� */
                ptFreqList = &ptNbrElement->elem[dwBandLocation].neighCellsPerFreqList;

                /* ����Ƶ�� */
                //if ((ptFreqList->n < 16) && (ptCellList->n < 16))
                {
                    ptFreqList->elem[ptFreqList->n].arfcn = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq;

                    /* ����С�� */
                    ptCellList = &ptFreqList->elem[ptFreqList->n].physCellIdList;
                    ptCellList->elem[ptCellList->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;

                    ptFreqList->n++;  /*Ƶ������1*/
                    ptCellList->n++;  /*С������1*/
                }
            }
        }
        else
        {
            //if ((ptNbrElement->n < 16) && (ptFreqList->n < 16) && (ptCellList->n < 16))
            {
                /* Ƶ��û�����������Ƶ�㡢С��һ��û���������
                �����Ӹ�����Ƶ��,�����Ӹ�Ƶ���µ�Ƶ�㣬������Ӹ�Ƶ���µ�С�� */
                ptNbrElement->elem[ptNbrElement->n].bandClass = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.ucBandClass;

                /* ����Ƶ�� */
                ptFreqList = &ptNbrElement->elem[ptNbrElement->n].neighCellsPerFreqList;
                ptFreqList->elem[ptFreqList->n].arfcn = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wCarriFreq;

                /* ����С�� */
                ptCellList = &ptFreqList->elem[ptFreqList->n].physCellIdList;
                ptCellList->elem[ptCellList->n] = tGetCDMAAdjCellInfoByCellIdAck.tCDMAAdjCellInfo.wPNOffset;

                ptNbrElement->n++;  /* Ƶ������1 */
                ptFreqList->n++;    /* Ƶ������1 */
                ptCellList->n++;    /* С������1 */
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
* ��������: CimFillMibInfo
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillMibInfo(T_DBS_GetMibInfo_ACK *ptGetMibInfoAck, T_AllSibList *ptAllSibList)
{
    CCM_NULL_POINTER_CHECK(ptGetMibInfoAck);
    CCM_NULL_POINTER_CHECK(ptAllSibList);

    CCM_LOG(RNLC_INFO_LEVEL, "\n SI: Fill MIB Info! \n");
    
    /* ��дϵͳ����dl_Bandwidth */
    ptAllSibList->tMibInfo.dl_Bandwidth = (MasterInformationBlock_dl_Bandwidth)ptGetMibInfoAck->tMib.ucSysBandWidth;
    
    /* ��дPHICH����phich_Configuration */
    ptAllSibList->tMibInfo.phich_Config.phich_Duration = (PHICH_Config_phich_Duration)ptGetMibInfoAck->tMib.ucPhichDuration;
    ptAllSibList->tMibInfo.phich_Config.phich_Resource = (PHICH_Config_phich_Resource)ptGetMibInfoAck->tMib.ucNg;
    
    /* ��дϵͳ֡��systemFrameNumber */
    ptAllSibList->tMibInfo.systemFrameNumber.numbits = 8; /* Э��涨8bit */
    ptAllSibList->tMibInfo.systemFrameNumber.data[0] = 0; /* SFNƫ��λ�ã�3+3=6bit */
    /* ��׼Э��汾*/
    if(0 == g_ucSceneCfg||2 == g_ucSceneCfg)
    {
         ptAllSibList->tMibInfo.spare.numbits = 10;            /* Э��涨10bit */
         ptAllSibList->tMibInfo.spare.data[0] = 0;
         ptAllSibList->tMibInfo.spare.data[1] = 0;
    }
    /* ���߸����汾*/
    else 
    {
         ptAllSibList->tMibInfo.spare.numbits = 10;            /* Э��涨10bit */
         ptAllSibList->tMibInfo.spare.data[0] = 170;           /* ��������Ҫ��10bitΪ1010101010 */
         ptAllSibList->tMibInfo.spare.data[1] = 170;    
    }

    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib1Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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


    /* 1. ���С�����������ϢcellAccessRelatedInformation */
    ptCellAccessInfo = &(ptAllSibList->tSib1Info.cellAccessRelatedInfo);

    /* 1.1 ���PLMN�б�plmn_IdentityList */

    /*
    MCC��MNC��ת��˵��:
    MCC�̶�����Ϊ3������λ��ÿλ��ȡֵ��ΧΪ0~9
    MNC���ȿ���Ϊ2��3������Чλ��ȡֵ��Χ��0~9������020��20��ʾ�ĺ����ǲ�ͬ��

    ���ݿ��CCM������ֵ��һ��WORD16��ֵ������F��ʾ��Ч
    ����˵��:MCC = 460����ô���ݴ�����ֵΪ0xF460
    MNC = 020�������ݿ������ֵΪ0xF020; MNC = 20�������ݿ������ֵΪ0xFF20;
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
        /*��Ʊ��*/
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

        /* �ж�MCC�Ƿ�Ϸ� */
        if (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] & 0x000F) > 9)
            || (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] >> 4) & 0x000F) > 9)
            || (((ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] >> 8) & 0x000F) > 9))
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID, "\n SI: MCC is error. MCC=%d\n",ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop]);

            return FALSE;
        }
        wMcc = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.awMCC[dwIdLoop] & 0x0fff;
        ptPlmnId->mcc.n = 3;
        ptPlmnId->mcc.elem[0] = (wMcc >> 8)  & 0x000F;        /* �� */
        ptPlmnId->mcc.elem[1] = (wMcc >> 4)  & 0x000F;        /* ʮ */
        ptPlmnId->mcc.elem[2] = wMcc & 0x000F;                   /* �� */

        /* �ж�MNC�Ƿ�Ϸ� */
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
            ptPlmnId->mnc.elem[0] = (wMnc >> 4)  & 0x000F;       /* ʮ */
            ptPlmnId->mnc.elem[1] =  wMnc & 0x000F;                 /* �� */
        }
        else
        {
            wMnc = wMnc & 0x0fff;
            ptPlmnId->mnc.n = 3;
            ptPlmnId->mnc.elem[0] =  (wMnc >> 8)  & 0x000F;       /* �� */
            ptPlmnId->mnc.elem[1] =  (wMnc >> 4)  & 0x000F;  /* ʮ */
            ptPlmnId->mnc.elem[2] =  wMnc & 0x000F;        /* �� */
        }

        ptCellAccessInfo->plmn_IdentityList.elem[dwIdLoop].cellReservedForOperatorUse
        = (PLMN_IdentityInfo_cellReservedForOperatorUse)ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.aucCellResvForOprUse[dwIdLoop];
    }

    /* 1.2 ���׷������trackingAreaCode */
    ptCellAccessInfo->trackingAreaCode.numbits = 16;  /* [16,16] */
    ptCellAccessInfo->trackingAreaCode.data[0]
    = (ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wTAC) >> 8;     /* TA���8bit */
    ptCellAccessInfo->trackingAreaCode.data[1]
    = (ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wTAC) & 0x00FF; /* TA���8bit */

    /* 1.3 ���С����ʶcellIdentity */
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

    /* 1.4 �������cellBarred��intraFreqReselection��csg_Indication */
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

    /* 1.5 �ݲ�֧��CSGID */
    ptCellAccessInfo->m.csg_IdentityPresent = 0;

    /* 2. ���С��ѡ�������ϢcellSelectionInfo */
    ptCellSelectionInfo = &(ptAllSibList->tSib1Info.cellSelectionInfo);
    ptCellSelectionInfo->q_RxLevMin
    = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucSelQrxLevMin - 70; /* [-70,-22] */

    /* ���ݿⲻ֧�ֿ�ѡ,���� */
    ptCellSelectionInfo->m.q_RxLevMinOffsetPresent = 1;
    ptCellSelectionInfo->q_RxLevMinOffset
    = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucQrxLevMinOfst; /* [1,8]*/

    /* 3. �������书��p_Max�����ݿⲻ֧�ֿ�ѡ,���� */
    ptAllSibList->tSib1Info.m.p_MaxPresent = 1;
    ptAllSibList->tSib1Info.p_Max = ptGetSib1InfoAck->tSib1.ucP_Max - 30; /* [-30,33] */

    /* 4. ���Ƶ��ָʾfrequencyBandIndicator */
    ptAllSibList->tSib1Info.freqBandIndicator = (ASN1INT)(ptGetSib1InfoAck->tSib1.ucFreqBandIndicator) ; /* [1,64] */

    /* 5. ������Ϣ��ȷ��ʵ�ʿ����·���SIB����д */

    /* 6. ��дTDD���ò��� */
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

    /* 7. ��дSI���ڳ���si-WindowLength */
    ptAllSibList->tSib1Info.si_WindowLength = (SystemInformationBlockType1_si_WindowLength)ptGetSib1InfoAck->tSib1.ucSiWindowLength;

    /* 8. ֵ��ǩsystemInformationValueTag */
    ptAllSibList->tSib1Info.systemInfoValueTag = ucLastValueTag;

    /*9. R9Э���������ֿ�ʼ*/
    ptAllSibList->tSib1Info.m.nonCriticalExtensionPresent = 0;
    /*����r890Э���������ִ���*/
    ptAllSibList->tSib1Info.m.nonCriticalExtensionPresent = 1;
    /*��д��������ָʾ*/
    /*��ȡָ��r9��Ϣ��չ�Ľṹ��ָ��*/
    ptRNineExtension = &(ptAllSibList->tSib1Info.nonCriticalExtension.nonCriticalExtension);
    /*2.��ʼ����ȡ���ݿ������*/
    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = ptGetSib1InfoAck->tSib1.tcellAccessRelatedInfo.wCID;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    /*3��ʼ����û�ȡ�Ľ���ṹ��*/
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
        /*4.����usf��ܻ�ȡС����¼����*/
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
            /*����r9Э���������ִ���*/
            ptAllSibList->tSib1Info.nonCriticalExtension.m.nonCriticalExtensionPresent = 1;

            /*����Я��ims֧��*/
            ptRNineExtension->m.ims_EmergencySupport_r9Present = 1;
            ptRNineExtension->ims_EmergencySupport_r9 = true_5;
        }
    }
    /*��дQqualmin �� Qqualminoffset�ֶ�*/
    ptAllSibList->tSib1Info.nonCriticalExtension.m.nonCriticalExtensionPresent = 1;
    ptRNineExtension->m.cellSelectionInfo_v920Present = 1;
    ptRNineExtension->cellSelectionInfo_v920.m.q_QualMinOffset_r9Present = 1;
    ptRNineExtension->cellSelectionInfo_v920.q_QualMinOffset_r9 = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucQqualminoffset;
    ptRNineExtension->cellSelectionInfo_v920.q_QualMin_r9 = ptGetSib1InfoAck->tSib1.tcellSelectionInfo.ucCellSelQqualMin-34;
    /*9. R9Э���������� ����*/
    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib2Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib2Info(BYTE ucRadioMode, T_DBS_GetSib2Info_ACK *ptGetSib2InfoAck, T_AllSibList *ptAllSibList,WORD16 wCellId)
{
    SystemInformationBlockType2_ac_BarringInfo  *ptAccessBar = NULL;
    WORD32   dwSpecAcNum  = 0;
    BYTE     ucSpecAcCfg  = 0;      /* AC11~AC15���õ�λͼ��Ϣ */
    WORD32   dwSpecAcLoop = 0;
    WORD32   dwResult     = FALSE;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId,"\n SI: Fill SIB2 Info! \n");

    /* 1. ��д�����ֹ��ϢaccessBarringInformation�����ݿⲻ֧�ֿ�ѡ������  */
    ptAllSibList->tSib2Info.m.ac_BarringInfoPresent = 1;
    ptAccessBar = &(ptAllSibList->tSib2Info.ac_BarringInfo);

    /* 1.1 ��д���ڽ������еĽ�������accessBarringForEmergencyCalls */
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
    /* 1.2 ��д��������Ľ�������accessBarringForSignalling�����ݿⲻ֧�ֿ�ѡ,���� */
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
    /* 1.3 ��д���ڷ�����еĽ�������accessBarringForOriginatingCalls�����ݿⲻ֧�ֿ�ѡ,���� */
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
    /* 2. ��д������Դ���ù�������radioResourceConfigCommon */
    dwResult = CimFillSib2RadioResCfgCommon(ptGetSib2InfoAck, ptAllSibList);
    if (FALSE == dwResult)
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: Failed to fill radio resource cfg!\n");

        return FALSE;
    }
    /* 2 + ��bpl1����pdsch�ŵ��е�rs�ο��ź��ֶ�Ϊ����С��������cp�����ֵ */
    dwResult = (WORD32)CimFillSib2ResCfgForSuperCell(ptGetSib2InfoAck, ptAllSibList,wCellId);
    if (RNLC_SUCC != dwResult)
    {
       /*���ʧ�ܣ�����С�����е�ֵ���ݶ�Ϊ����������*/
       CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId,"\n SI: BPL1 Failed to fill radio resource SIB2 RS cfg!\n");
    }
    /* 3. ��дue_TimersAndConstant */
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

    /* 4. ��дfrequencyInformation */
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

    /* 5. ��дmbsfn_SubframeConfiguration Ŀǰ��֧�� */
    ptAllSibList->tSib2Info.m.mbsfn_SubframeConfigListPresent = 0;

    /* 6. ��дtimeAlignmentTimerCommon */
    ptAllSibList->tSib2Info.timeAlignmentTimerCommon = (TimeAlignmentTimer)ptGetSib2InfoAck->tSib2.ucTimeAlignTimer;

    /* 7. ��дlateR8NonCriticalExtension */
    ptAllSibList->tSib2Info.m.lateNonCriticalExtensionPresent = 0;
    ptAllSibList->tSib2Info.lateNonCriticalExtension.numocts = 0;
    ptAllSibList->tSib2Info.m._v3ExtPresent = 1;

    /*R9Э��������Ҫ��� */
    /* 8. ��дssac_BarringForMMTEL_Voice_r9 */
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


    /* 9. ��дssac_BarringForMMTEL_Video_r9 */
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
    /*R9Э��������Ҫ��� */

    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib3Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* 1. ���С����ѡ��������cellReselectionInfoCommon */
    ptCellRslComm = &(ptAllSibList->tSib3Info.cellReselectionInfoCommon);
    /* 3. ��дintraFreqCellReselectionInfo */
    ptIntraCellRsl = &(ptAllSibList->tSib3Info.intraFreqCellReselectionInfo);

    /* 1.1 q_Hyst */
    ptCellRslComm->q_Hyst
    = (SystemInformationBlockType3_q_Hyst)ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucQhyst;

    /* 1.2 speedDependentReselection�������û��������þ����Ƿ��·�,���� */
    ptCellRslComm->m.speedStateReselectionParsPresent = 0;
    ptIntraCellRsl->m.t_ReselectionEUTRA_SFPresent = 0;
    if (1 == ptGetSib3InfoAck->tSib3.tCellReselectionInfoCommon.ucReselParaBaseSpeedFlag)
    {
        /*��дCellReselectionInfoCommon   �����ٶȵ���ز���*/
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
        /*��дeutran�µĻ����ٶȵ���ѡ��ز���*/
        ptIntraCellRsl->m.t_ReselectionEUTRA_SFPresent = 1;
        ptIntraCellRsl->t_ReselectionEUTRA_SF.sf_Medium
        = (SpeedStateScaleFactors_sf_Medium)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctReselIntraSFM;
        ptIntraCellRsl->t_ReselectionEUTRA_SF.sf_High
        = (SpeedStateScaleFactors_sf_High)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctReselIntraSFH;         
    }

    /* 2. ��дcellReselectionServingFreqInfo */
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

    /* ���������:D=(P+140)/2 ,Ĭ��ֵ��-130��Э������Ϊ[-70 -22],��ʱ���˴��룬ȷ�Ϻ����*/
    ptIntraCellRsl->q_RxLevMin
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraQrxLevMin - 70;

    /* Э��: [-30,33]��Dbs=IE+30�����ݿⲻ֧�ֿ�ѡ,����  */
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

    /* ���ͬƵС����ѡ����UE�������� */
    ucAllowMeasBw = (BYTE)CimGetAllowMeasBwOfFreq(wCellId, ucRadioMode, ptGetSib3InfoAck);

    if (ucAllowMeasBw == ptGetSib3InfoAck->tSib3.ucDlSysBandWidth)
    {
        /* Э��:�������Ԫ����д����UEȡ����С������ϵͳ������Ϊ����������� */
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
    ptIntraCellRsl->neighCellConfig.data[0] = 0; /* ���ݿ�ȱ���ֶΣ�д��Ϊ0 */

    ptIntraCellRsl->t_ReselectionEUTRA
    = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.uctRslIntraEutra;

    /*R9Э������ ��� */

    /* 4. ��дlateR8NonCriticalExtension */
    ptAllSibList->tSib3Info.m.lateNonCriticalExtensionPresent = 0;
    ptAllSibList->tSib3Info.m._v3ExtPresent = 1;

    /* 5. ��дs_IntraSearch_v920 */
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
    /* 6. ��дs_NonIntraSearch_v920 */
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
    /* 7. ��дq_QualMin_r9 */
    ptAllSibList->tSib3Info.m.q_QualMin_r9Present = 1;
    ptAllSibList->tSib3Info.q_QualMin_r9
    = (Q_QualMin_r9)ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucIntraFreqQqualMin - 34;

    /* 8. ��дthreshServingLowQ_r9  ��Ҫ�����û����õĿ��ؾ����Ƿ��·� --�㷨���*/
    ptAllSibList->tSib3Info.m.threshServingLowQ_r9Present = 0;
 
    if(1 == ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucThreshSrvLowQSwitch)
    {
        ptAllSibList->tSib3Info.m.threshServingLowQ_r9Present = 1;
        ptAllSibList->tSib3Info.threshServingLowQ_r9 = ptGetSib3InfoAck->tSib3.tIntraFreqCellReselectionInfo.ucThreshSrvLowQ;
    }
    /*R9Э������ ��� */
    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib4Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib4Info(WORD16 wCellId, BYTE ucRadioMode, T_DBS_GetSib4Info_ACK *ptGetSib4InfoAck, T_AllSibList *ptAllSibList)
{
    BYTE    ucNtetradNum    = 0;
    BYTE    ucTetradNum     = 0;
    BYTE    ucPciRangeIndx  = 0;
    WORD16  wPciCnt         = 0;
    BYTE    ucResult        = 0; /* ��ȡ���ݿ���¼���ؽ�� */
    WORD16  wNbrPhyCellId   = 0; /* ��С������ID */
    WORD32  dwNbrDlCntrFreq = 0; /* ��С����������Ƶ�� */
    WORD16  wNbrCellNum     = 0; /* ������Ŀ */
    WORD32  dwNbrCellLoop   = 0;
    WORD32  dwEmptyItem     = 0;
    WORD32  dwRangeLoop     = 0;
    WORD32  wPciStart       = 0;
    BYTE    aucPciRange[]   = {1, 2, 3, 4, 6, 8, 12, 16, 21, 24, 32, 42, 63, 126}; /* PCI range after div by 4 */
    BYTE    aucBlkPciList[CIM_MAX_EUTRAN_PCI + 2] = {0}; /* ����������С��ID�б� */

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

        /* û��������ʱ���·�SIB4��Э��Ӧ��������� */
        return FALSE;
    }

    /* 0. ��ʼ��SIB4�������Ԫ�·���־λ */
    ptAllSibList->tSib4Info.m.intraFreqNeighCellListPresent = 0;
    ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent = 0;

    /* �ݲ�֧�� */
    ptAllSibList->tSib4Info.m.csg_PhysCellIdRangePresent = 0;
    ptAllSibList->tSib4Info.m.lateNonCriticalExtensionPresent = 0;

    /* 1. ��������б�ͺ������б� */
    ptNeighCellList = &(ptAllSibList->tSib4Info.intraFreqNeighCellList);
    ptBlackCellList = &(ptAllSibList->tSib4Info.intraFreqBlackCellList);
    ptNeighCellList->n = 0;
    ptBlackCellList->n = 0;

    /* ��С�������жϲ���¼ */
    for (dwNbrCellLoop = 0; dwNbrCellLoop < wNbrCellNum; dwNbrCellLoop++)
    {
        /*EC:������ ����������ó�connect̬����㲥���ų���������Ϣ*/
            if(1 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucStateInd)
                {
                CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI:SIB4 Cell(ID= %d) set connect state.ingnored! \n",tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].tLTECellId.wCellId);
              continue;
                }
        /* �����eNB�ڵ�С�������ȡSRVCEL��ȡ����ز��� */
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

            /* ��ȡ��������Ƶ�ʺ�����С��Id */
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
            /* ��������eNBС����ȡADJCEL��ȡ����ز��� */
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

            /* ��ȡ��������Ƶ�ʺ�ϵͳ���� */
            wNbrPhyCellId   = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.wPhyCellId;
            dwNbrDlCntrFreq = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.dwDlCenterFreq;
        }

        /* ucBlackNbrCellInd: ENUMERATE{0:false, 1:true} */
        if (0 == tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucBlackNbrCellInd)
        {
            /* ��д�������б�,���16�� */
            if (ptNeighCellList->n < 16)
            {
                dwEmptyItem = ptNeighCellList->n;
                ptNeighCellList->elem[dwEmptyItem].physCellId = wNbrPhyCellId;
                ptNeighCellList->elem[dwEmptyItem].q_OffsetCell
                = (Q_OffsetRange)tGetAllIntraFreqLTENbrCellInfoAck.atIntraFreqNbrCellInfo[dwNbrCellLoop].ucQOfStCell;
                ptNeighCellList->elem[dwEmptyItem].extElem1.numocts = 0;

                /* ���������б������ */
                ptNeighCellList->n++;
            }
        }
        else
        {
            /* ��д�������б� */
            if (wNbrPhyCellId <= CIM_MAX_EUTRAN_PCI)
            {
                /* ������С��IDΪ����������������������1 */
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

        /* Э��˵�����б��ǿ�ѡ�ģ�����û��˵û�а��б��ʱ��SIB4�Ͳ����ˣ����Բ�Ҫreturn */
        /* return FALSE; */
    }

    if (1 == ptAllSibList->tSib4Info.m.intraFreqBlackCellListPresent)
    {
        /* ���������б������ڵ�PCI���,��дrange */
        wPciCnt = 0;

        aucBlkPciList[CIM_MAX_EUTRAN_PCI + 1] = 0;
        for (dwNbrCellLoop = 0; dwNbrCellLoop < (CIM_MAX_EUTRAN_PCI + 2); dwNbrCellLoop++)
        {
            if (1 == aucBlkPciList[dwNbrCellLoop])
            {
                wPciCnt++; /* ����������PCI���� */
            }
            else
            {
                if (0 == wPciCnt)
                {
                    continue;
                }

                ucNtetradNum = wPciCnt % 4;

                /* �������PCI��������4����϶���дrange */
                ucTetradNum = (BYTE)(wPciCnt / 4);
                wPciStart = dwNbrCellLoop - wPciCnt;
                while (ucTetradNum > 0)
                {
                    /* Ѱ����ӽ���range */
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
                        /* û���ҵ����ʵ�range�����ϵͳ�������������ᷢ�� */
                        break;
                    }

                    /* ��д�������б�������16�� */
                    if (ptBlackCellList->n < 16)
                    {
                        dwEmptyItem = ptBlackCellList->n;
                        ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                        ptPciRange->start = (ASN1INT)wPciStart;
                        ptPciRange->m.rangePresent = 1;
                        ptPciRange->range = (PhysCellIdRange_range)ucPciRangeIndx;

                        /* ���º������б������ */
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

                /* ��д����ƥ��range��PCI */
                for (dwRangeLoop = 0; dwRangeLoop < (WORD32)ucNtetradNum; dwRangeLoop++)
                {
                    if (ptBlackCellList->n < 16)
                    {
                        dwEmptyItem = ptBlackCellList->n;
                        ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                        ptPciRange->start = (ASN1INT)wPciStart;
                        ptPciRange->m.rangePresent = 0;

                        /* ���º������б������ */
                        ptBlackCellList->n++;
                    }
                    wPciStart++;
                }

                /* ���ü����� */
                wPciCnt = 0;
            }

        }

    }

    /* csg_PhysCellIdRange �ݲ�֧�� */

    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib5Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    WORD32  dwNbrCellNum     = 0;  /* ������Ŀ */
    WORD32  dwNbrCellNumLoop = 0;
    WORD32  dwCfLoop         = 0;
    WORD32  dwCfNum          = 0;  /* Carrier Frequency���� */
    WORD32  dwNbrDlCntrFreq  = 0;
    WORD32  dwEmptyItem      = 0;
    WORD32  dwNbrCellLoop    = 0;
    WORD32  dwRangeLoop      = 0;
    WORD32  wPciStart        = 0;
    BYTE    aucBlkPciList[CIM_MAX_EUTRAN_PCI + 2] = {0}; /* ����������С��ID�б� */
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

    /* ����SIB5����Ƶ�б��Ǳ�����д�ģ��Ҳ���Ϊ0������Ƶ����Ϣ���û�����õĻ���
    ��SIB�������·� */
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
    /*��ȡ��Ƶ�����б���Ϣ*/ 
    tGetAllInterFreqLTENbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllInterFreqLTENbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllInterFreqLTENbrCellInfo_REQ, (VOID *)&tGetAllInterFreqLTENbrCellInfoReq, (VOID *)&tGetAllInterFreqLTENbrCellInfoAck);
    if (FALSE == ucResult)
    {
       CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB5 USF EV_DBS_GetAllInterFreqLTENbrCellInfo_REQ FAIL! \n");
    }
    /* ���SIB5������Ϣ */
    dwCfNum = ptAllSibList->tSib5Info.interFreqCarrierFreqList.n;
    for (dwCfLoop = 0; dwCfLoop < dwCfNum; dwCfLoop++)
    {
        ptFreqInfo = &(ptAllSibList->tSib5Info.interFreqCarrierFreqList.elem[dwCfLoop]);

        ptFreqInfo->m.p_MaxPresent = 1;
        ptFreqInfo->m.cellReselectionPriorityPresent = 1;
        ptFreqInfo->m.interFreqNeighCellListPresent = 0;
        ptFreqInfo->m.interFreqBlackCellListPresent = 0;

        ptFreqInfo->dl_CarrierFreq = ptGetSib5InfoAck->tSib5.tCelSelInfo.awInterCarriFreq[dwCfLoop];

        /* Э��: [-70,-22] Dbs=IE+70 */
        ptFreqInfo->q_RxLevMin = ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterQrxLevMin[dwCfLoop] - 70;

        /* Э��: [-30,33] Dbs=IE+30 */
        ptFreqInfo->p_Max = ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterPmax[dwCfLoop] - 30;
        ptFreqInfo->t_ReselectionEUTRA = ptGetSib5InfoAck->tSib5.tCelSelInfo.auctRslInterEutra[dwCfLoop];

        /*��д���ٶ��йص���ѡ��������Ҫ�����û�ѡ��Ŀ��ؾ����Ƿ��·�--�㷨���*/
        ptFreqInfo->m.t_ReselectionEUTRA_SFPresent = 0;
        if (1 == ptGetSib5InfoAck->tSib5.tCelSelInfo.ucReselParaBaseSpeedFlag)
        {
            /*��������Ǵ򿪵�״̬���·���صĲ���*/
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
        ptFreqInfo->neighCellConfig.data[0] = 0;    /* ��ʱд��0 */

        ptFreqInfo->q_OffsetFreq = (Q_OffsetRange)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucQOffsetFreq[dwCfLoop];

        /*���R9Э��������Ҫ����Ĳ��� */
        /*��д��rsrq��صļ����ֶΣ�����Э���⼸���ֶε��·������Ҫ��sib3�е�rsrq�ֶ�һ��*/
        ptFreqInfo->m._v2ExtPresent = 0;
        ptFreqInfo->m.threshX_Q_r9Present = 0;
        ptFreqInfo->m.q_QualMin_r9Present = 0;
        if(1 == ptGetSib5InfoAck->tSib5.tCelSelInfo.ucThreshSrvLowQSwitch)
        {       
            /*���sib3����rsrq��صĿ����Ǵ򿪵ģ����·�*/
            ptFreqInfo->m._v2ExtPresent = 1;
            ptFreqInfo->m.threshX_Q_r9Present = 1;
            ptFreqInfo->m.q_QualMin_r9Present = 1;
            /*��ȷ��������ж�Ӧ�ı���������q_QualMin_r9 */
            ptFreqInfo->q_QualMin_r9 = (Q_QualMin_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterFreqQqualMin[dwCfLoop]-34;
            ptFreqInfo->threshX_Q_r9.threshX_HighQ_r9 = (ReselectionThresholdQ_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThreshXHighQ[dwCfLoop];
            ptFreqInfo->threshX_Q_r9.threshX_LowQ_r9 =  (ReselectionThresholdQ_r9)ptGetSib5InfoAck->tSib5.tCelSelInfo.aucInterThreshXLowQ[dwCfLoop];
        }
        /*RЭ���������ӽ��� */

        /*���û�������б�������д�����б���ص���Ϣ��ֱ�ӷ��أ����û�������б��·��㲥������*/
        if (0 != tGetAllInterFreqLTENbrCellInfoAck.dwResult)
        {
            CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: SIB5 Not get neighbor cell list! \n");
            continue;
        }
        /*����ȫ������д���������Ϣ�����ݣ����û�������б�ǰ���Ѿ����أ�����ִ������*/
        /* �ڰ��������ȳ�ʼΪ0 */
        ptFreqInfo->interFreqNeighCellList.n = 0;
        ptFreqInfo->interFreqBlackCellList.n = 0;

        /* ��ȡ������¼ */
        ucMinDlBw = mbw100 + 1;

        ptNeighCellList = &(ptFreqInfo->interFreqNeighCellList);
        ptBlackCellList = &(ptFreqInfo->interFreqBlackCellList);
        dwNbrCellNum = tGetAllInterFreqLTENbrCellInfoAck.wInterFreqLTENbrCellNum;

        /* ���д��벻��ɾ���������ͬ��Ƶ�ĺڱ�����*/
        memset(aucBlkPciList, 0, sizeof(aucBlkPciList));

        for (dwNbrCellNumLoop = 0; dwNbrCellNumLoop < dwNbrCellNum; dwNbrCellNumLoop++)
        {
            /*EC:������ ����������ó�connect̬����㲥���ų���������Ϣ */
            if(1 == tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].ucStateInd)
            {
                CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI:SIB5 Cell has been set only connect state,it will be ingnored! \n");
                continue;
            }
            /* ��eNB�ڽ�С������ȡ DBTABLE_R_SRVCEL */
            if ((tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMcc == ptGetSib5InfoAck->tSib5.wMCC) &&
            (tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.wMnc == ptGetSib5InfoAck->tSib5.wMNC) &&
            (tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].tLTECellId.dwENodeBId == ptGetSib5InfoAck->tSib5.dwEnodebID))
            {
                /* ����������ʶ��ȡ��С������������Ƶ�� */
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

                /* ��ȡ��������Ƶ�ʺ�����С��Id */
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
                /* ��������eNBС����ȡADJCEL��ȡ����ز��� */
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

                /* ��ȡ��������Ƶ�ʺ�����С��Id */
                wNbrPhyCellId   = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.wPhyCellId;
                dwNbrDlCntrFreq = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.dwDlCenterFreq;
                ucNbrDlBw       = tGetAdjCellInfoByCellIdAck.tLTEAdjCellInfo.ucDlSysBandWidth;
            }

            /* �ж��Ƿ���Ƶ�������Լ��Ƿ�����������Ƶ���� */
            if ((dwNbrDlCntrFreq == ptGetSib5InfoAck->tSib5.dwDlCenterFreq) ||
                (dwNbrDlCntrFreq != (WORD32)ptFreqInfo->dl_CarrierFreq))
            {
                continue;
            }

            /* ���ڰ����� */
            /* ucBlackNbrCellInd: ENUMERATE{false:0, true:1} */
            if (0 == tGetAllInterFreqLTENbrCellInfoAck.atInterFreqNbrCellInfo[dwNbrCellNumLoop].ucBlackNbrCellInd)
            {
                if (ptNeighCellList->n < 16)
                {
                    /* �����Ƶ�����������б���ÿ��Ƶ�������16������ */
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
                /* ��д�������б� */
                if (wNbrPhyCellId <= CIM_MAX_EUTRAN_PCI)
                {
                    /* ������С��IDΪ��������Ӧλ��1 */
                    aucBlkPciList[wNbrPhyCellId] = 1;
                    ptFreqInfo->m.interFreqBlackCellListPresent = 1;
                }
                else
                {
                     CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId, "\n SI: SIB5 Invalid Black Neighbour PhyCellId(%lu)!\n",wNbrPhyCellId);
                }
            }

        } /* end��ȡ������¼  */

        /* ����������ptFreqInfo->allowedMeasBandwidth */
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
            /* ���������б������ڵ�PCI���,��дrange */
            wPciCnt = 0; /* ����������PCI���� */
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
                        /* Ѱ����ӽ���range */
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
                            break; /* û���ҵ����ʵ�range */
                        }

                        if (ptBlackCellList->n < 16)
                        {
                            dwEmptyItem = ptBlackCellList->n;
                            ptPciRange = &(ptBlackCellList->elem[dwEmptyItem]);
                            ptPciRange->start = (ASN1INT)wPciStart;
                            ptPciRange->m.rangePresent = 1;
                            ptPciRange->range = (PhysCellIdRange_range)ucPciRangeIndx;

                            ptBlackCellList->n++; /* ���º������б������ */
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

                            ptBlackCellList->n++; /* ���º������б������ */
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
* ��������: CimFillSib6Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib6Info(WORD16 wCellId,BYTE ucRadioMode, T_DBS_GetSib6Info_ACK *ptGetSib6InfoAck, T_AllSibList *ptAllSibList)
{
    WORD32              dwCfListLoop    = 0;      /* ������Ƶ�б���� */
    WORD32              dwCfListSize    = 0;

    CarrierFreqUTRA_FDD *ptFddCfList    = NULL;
    CarrierFreqUTRA_TDD *ptTddCfList    = NULL;

    CCM_SIOUT_LOG(RNLC_INFO_LEVEL,wCellId, "\n SI: Fill SIB6 Info!\n");
    ucRadioMode = ucRadioMode;
    /* FDD */
    ptAllSibList->tSib6Info.m.carrierFreqListUTRA_FDDPresent = 1;

    if (0 == ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum)
    {
        /* ��Ƶ��ĿΪ0��������Ƶ��ϢΪ��ѡ�ģ����Լ�ʹ��Ƶ����Ϊ0��Ҳ�·���SIB */
        ptAllSibList->tSib6Info.m.carrierFreqListUTRA_FDDPresent = 0;
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = 0;    /* (1,16) */
    }
    else if (ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum > CIM_FDD_CARRIER_FREQ_List)
    {
        /* �����б��� */
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = CIM_FDD_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL, wCellId,"\n SI: SIB6 carrierFreqListUTRA_FDD.n=%u beyond maxmum(%u)!\n",
                ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum,
                CIM_FDD_CARRIER_FREQ_List);
    }
    else
    {
        ptAllSibList->tSib6Info.carrierFreqListUTRA_FDD.n = ptGetSib6InfoAck->tSib6.ucUtranFDDFreqNum;
    }

    /* ��дFDD��Ƶ�б���Ϣ */
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

        /*R9Э��������д����*/
        /*����sib3��rsrq�ֶ��Ƿ��·��������Ƿ��·�sib6����ص�rsrq�ֶ�--Э��涨*/
        ptFddCfList->m.threshX_Q_r9Present = 0;
        if(1 == ptGetSib6InfoAck->tSib6.ucThreshSrvLowQSwitch)
        {
            /*���sib3���·��Ŀ��ش򿪣���Ҳ�·�*/
            ptFddCfList->m.threshX_Q_r9Present = 1;
            /*�ν�ΰadd20120920*/
            ptFddCfList->m._v2ExtPresent = 1;
            ptFddCfList->threshX_Q_r9.threshX_HighQ_r9 = ptGetSib6InfoAck->tSib6.aucUtranThreshXHighQFdd[dwCfListLoop];
            ptFddCfList->threshX_Q_r9.threshX_LowQ_r9 =  ptGetSib6InfoAck->tSib6.aucUtranThreshXLowQFdd[dwCfListLoop];
        }
        /*R9Э��������ӽ���*/
    }

    /* TDD */
    ptAllSibList->tSib6Info.m.carrierFreqListUTRA_TDDPresent = 1;

    if (0 == ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum)
    {
        /* ��Ƶ��ĿΪ0��������Ƶ��ϢΪ��ѡ�ģ����Լ�ʹ��Ƶ����Ϊ0��Ҳ�·���SIB */
        ptAllSibList->tSib6Info.m.carrierFreqListUTRA_TDDPresent = 0;
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = 0;    /* (1,16) */
    }
    else if (ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum > CIM_TDD_CARRIER_FREQ_List)
    {
        /* �����б��� */
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = CIM_TDD_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId,"\n SI: SIB6 carrierFreqListUTRA_TDD.n=%u beyond maxmum(%u)!\n",
                ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum,
                CIM_TDD_CARRIER_FREQ_List);
    }
    else
    {
        ptAllSibList->tSib6Info.carrierFreqListUTRA_TDD.n = ptGetSib6InfoAck->tSib6.ucUtranTDDFreqNum;
    }

    /* ��дTDD��Ƶ�б���Ϣ */
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

    /*��дutran�����ٶȵ���ѡ������������ش����·�*/
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
* ��������: CimFillSib7Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    
    /*��дgsm�����ٶȵ���ѡ����*/
    ptAllSibList->tSib7Info.m.t_ReselectionGERAN_SFPresent = 0;
    if (1 == ptGetSib7InfoAck->tSib7.ucReselParaBaseSpeedFlag)
    {
        /*������ش����·������ٶȵ���ѡ����*/
        ptAllSibList->tSib7Info.m.t_ReselectionGERAN_SFPresent = 1;
        ptAllSibList->tSib7Info.t_ReselectionGERAN_SF.sf_Medium
        = (SpeedStateScaleFactors_sf_Medium)ptGetSib7InfoAck->tSib7.uctReselGeranSFM;
        ptAllSibList->tSib7Info.t_ReselectionGERAN_SF.sf_High
        = (SpeedStateScaleFactors_sf_High)ptGetSib7InfoAck->tSib7.uctReselGeranSFH;
    }
    /* ��ȡGSMNBR */
    tGetAllGSMNbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllGSMNbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllGSMNbrCellInfo_REQ, (VOID *)&tGetAllGSMNbrCellInfoReq, (VOID *)&tGetAllGSMNbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllGSMNbrCellInfoAck.dwResult))
    {
        /*SIB6-SIB7�Ż���sib7���·�����������ϣ�û������Ҳ���·�*/
        CCM_SIOUT_LOG(RNLC_INFO_LEVEL, wCellId,"\n SI: SIB7 No neighbour cell list!\n");
    }

    if (0 == ptGetSib7InfoAck->tSib7.ucGeranFreqNum)
    {
        /* ��Ƶ��ĿΪ0��������Ƶ��ϢΪ��ѡ�ģ����Լ�ʹ��Ƶ����Ϊ0��Ҳ�·���SIB */
        ptAllSibList->tSib7Info.m.carrierFreqsInfoListPresent = 0;
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = 0; /* (1,16) */
    }
    else if (ptGetSib7InfoAck->tSib7.ucGeranFreqNum > CIM_GERAN_CARRIER_FREQ_List)
    {
        /* �����б�Χ */
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = CIM_GERAN_CARRIER_FREQ_List;
        
        CCM_SIOUT_LOG(RNLC_WARN_LEVEL,wCellId,"\n SI: SIB7 carrierFreqsInfoList.n=%d beyond CIM_GERAN_CARRIER_FREQ_List!\n",ptGetSib7InfoAck->tSib7.ucGeranFreqNum);
    }
    else
    {
        ptAllSibList->tSib7Info.carrierFreqsInfoList.n = ptGetSib7InfoAck->tSib7.ucGeranFreqNum;
    }

    /* ��д�б���Ϣ */
    for (dwNfListLoop = 0; dwNfListLoop < ptGetSib7InfoAck->tSib7.ucGeranFreqNum; dwNfListLoop++)
    {
        ptCarrierFreqsInfo = &(ptAllSibList->tSib7Info.carrierFreqsInfoList.elem[dwNfListLoop]);

        /* 1. ��дCarrierFreqsGERAN */
        ptCarrierFreqsGERAN = &(ptCarrierFreqsInfo->carrierFreqs);
        ptCarrierFreqsGERAN->startingARFCN = ptGetSib7InfoAck->tSib7.awStartARFCN[dwNfListLoop];
        ptCarrierFreqsGERAN->bandIndicator = (BandIndicatorGERAN)ptGetSib7InfoAck->tSib7.aucBandIndicator[dwNfListLoop];

        /* to do:Ӧ��Ϊ���ݿ�������ֶΣ�ĿǰĬ�ϲ�����ʾ��ʽ */
        ptCarrierFreqsGERAN->followingARFCNs.t = CIM_EXPLICITLIST_ARFCNS;
        if (CIM_EXPLICITLIST_ARFCNS == ptCarrierFreqsGERAN->followingARFCNs.t)
        {
            ptExpListOfARFCNs = &(ptAllSibList->tSibPointStruct.tExpListOfArfcns[dwNfListLoop]);
            ptCarrierFreqsGERAN->followingARFCNs.u.explicitListOfARFCNs = ptExpListOfARFCNs;

            /* ��дʣ��ARFCN��The remaining ARFCN values in the set are explicitly listed one by one.*/
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

            /* ���ݿ�Ŀǰû�������ֶ� */
            ptVarBitMapARFCN->numocts = 0;
        }
        else
        {
            CCM_SIOUT_LOG(RNLC_ERROR_LEVEL,wCellId, "\n SI: SIB7 ptCarrierFreqsGERAN->followingARFCNs.t=%d false.\n",ptCarrierFreqsGERAN->followingARFCNs.t);
            return FALSE;
        }

        /* 2. ��дCarrierFreqsInfoGERAN_commonInfo */
        ptCarrierFreqsInfoCommon = &(ptCarrierFreqsInfo->commonInfo);
        ptCarrierFreqsInfoCommon->m.cellReselectionPriorityPresent = 1;
        /*    */
        ptCarrierFreqsInfoCommon->m.p_MaxGERANPresent = 0;
        ptCarrierFreqsInfoCommon->cellReselectionPriority = ptGetSib7InfoAck->tSib7.aucGeranReselPrio[dwNfListLoop];

        /* ��ʼ��NCC_Permitted */
        ptCarrierFreqsInfoCommon->ncc_Permitted.numbits = 8;
        ptCarrierFreqsInfoCommon->ncc_Permitted.data[0] = 0;

        /* ��дSIB7��Permitted NCC��Ϣ */
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
* ��������: CimFillSib8Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��ȡCDMANBR */
    tGetAllCDMANbrCellInfoReq.wCallType = USF_MSG_CALL;
    tGetAllCDMANbrCellInfoReq.wCellId   = wCellId;
    ucResult = UsfDbsAccess(EV_DBS_GetAllCDMANbrCellInfo_REQ, (VOID *)&tGetAllCDMANbrCellInfoReq, (VOID *)&tGetAllCDMANbrCellInfoAck);
    if ((FALSE == ucResult) || (0 != tGetAllCDMANbrCellInfoAck.dwResult))
    {
        CCM_SIOUT_LOG(RNLC_ERROR_LEVEL, wCellId,"\n SI: SIB8 No CDMA neighbour Cell!\n");
    }

    /* 1. ��дIE: systemTimeInfo */
    if(0 == g_dwDebugSysTimeAndLongCode)
    {
        ptAllSibList->tSib8Info.m.systemTimeInfoPresent   = 0;
        /*����ط���Ҫ��ֵ����׮��������Ϊ0������Ϳ��ܻ��������*/
        g_dwSysTimeAndLongCodeExist = 0;
    }
    else
    {
        g_dwSysTimeAndLongCodeExist = 1;
        ptAllSibList->tSib8Info.m.systemTimeInfoPresent   = 1;
        ptSystemTimeInfo = &(ptAllSibList->tSib8Info.systemTimeInfo);
        if ( !g_bCcGPSLockState )
        {
            ptSystemTimeInfo->cdma_EUTRA_Synchronisation = FALSE;/*��ʼֵ����Ϊ��ͬ��*/
            ptSystemTimeInfo->cdma_SystemTime.t = 2;/*Ϊ�˱���ͨ��*/
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
                    ptSystemTimeInfo->cdma_EUTRA_Synchronisation = TRUE;/*��ʼֵ����Ϊ��ͬ��*/
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

    /* 2. ��дsearchWindowSize */
    ptAllSibList->tSib8Info.m.searchWindowSizePresent = 1;
    ptAllSibList->tSib8Info.searchWindowSize = ptGetSib8InfoAck->tSib8.ucSearchWinSize;

    /* 3. ��дHRPD���� */
    ptAllSibList->tSib8Info.m.parametersHRPDPresent   = 1;
    ptParaHrpd = &(ptAllSibList->tSib8Info.parametersHRPD);

    /* 3.1 ��дHRPD������preRegistrationInfoHRPD */
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

        /* Э��: INT[0,255] ��ѡcond PreRegAllowed */
        ptPreRegInfo->preRegistrationZoneId = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.wPreRegZoneId;
    }

    /* Э��: ��ԪsecondaryPreRegistrationZoneIdList  SIZE[1,2] ����INT[0,255] ��ѡ */
    ptPreRegInfo->m.secondaryPreRegistrationZoneIdListPresent = 1;
    ptZoneIdList = &(ptPreRegInfo->secondaryPreRegistrationZoneIdList);
    ptZoneIdList->n = 2;
    ptZoneIdList->elem[0] = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.awSecPreRegZoneId[0];
    ptZoneIdList->elem[1] = ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.awSecPreRegZoneId[1];

    /* 3.2 ��дHRPD������cellReselectionParametersHRPD */
    ptParaHrpd->m.cellReselectionParametersHRPDPresent = 1;
    ptCellRslPara = &(ptParaHrpd->cellReselectionParametersHRPD);

    /* ��дbandClassList */
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

    /* ���bandClassList�б� */
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

    /* to do ���������Ϣ */
    if (1 == ptParaHrpd->m.cellReselectionParametersHRPDPresent)
    {
        /* ���CDMA HRPD ������Ϣ */
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

        /*��дcdma�����ٶȵ���ѡ��ز�����*/   
        ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 0;
        if (1 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucReselParaBaseSpeedFlag)
        {
            /*��������򿪣����·������ٶȵ���ز���*/
            ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 1;
            ptSpeedScal = &(ptCellRslPara->t_ReselectionCDMA2000_SF);
            ptSpeedScal->sf_Medium = (SpeedStateScaleFactors_sf_Medium)ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.uctReselHrpdSFM;
            ptSpeedScal->sf_High   = (SpeedStateScaleFactors_sf_High)ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.uctReselHrpdSFH;
        }
    }

    /* 4. ��д1XRTT��ز��� */
    ptAllSibList->tSib8Info.m.parameters1XRTTPresent  = 1;
    pt1xrttPara = &(ptAllSibList->tSib8Info.parameters1XRTT);

    /* 4.1 ��дCS Fall Backע�����csfb_RegistrationParam1XRTT */
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

    /* 4.2 ��дlongCodeState1XRTT */
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


    /* 4.3 ��д1XRTTС����ѡ����cellReselectionParameters1XRTT */
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
        /* ���CDMA 1XRTT ������Ϣ */
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
        /* ��д��ѡ��CDMA2000 1XRTTС���о���ʱ������ */
        ptCellRslPara->t_ReselectionCDMA2000 = ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1Xrtt;

        /*��д�����ٶȵ���ѡ������������ش����·������￪�ظ���hrpd��*/
        ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 0; 
        if (1 == ptGetSib8InfoAck->tSib8.tCDMAHrpdRslInfo.ucReselParaBaseSpeedFlag)
        {
            /* ��д��\����״̬��ѡ��CDMA2000 1XRTTС��ʱ��������� */
            ptCellRslPara->m.t_ReselectionCDMA2000_SFPresent = 1; 
            ptSpeedScal = &(ptCellRslPara->t_ReselectionCDMA2000_SF);
            ptSpeedScal->sf_Medium = (SpeedStateScaleFactors_sf_Medium)ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1XrttSFM;
            ptSpeedScal->sf_High = (SpeedStateScaleFactors_sf_High)ptGetSib8InfoAck->tSib8.tCDMA1xrttRslInfo.uctResel1XrttSFH;
        }
    }

    /* 5. ��дIE: lateR8NonCriticalExtension���ݲ���д */
    ptAllSibList->tSib8Info.m.lateNonCriticalExtensionPresent   = 0;

    /* 6. ��дIE: csfb_SupportForDualRxUEs_r9��*/
    ptAllSibList->tSib8Info.m.csfb_SupportForDualRxUEs_r9Present   = 1;
    ptAllSibList->tSib8Info.csfb_SupportForDualRxUEs_r9 =
    (ASN1BOOL)ptGetSib8InfoAck->tSib8.ucCsfbSuptDualRx;

    /* 7. ��дIE: cellReselectionParametersHRPD_v920 */
    if(0 == ptAllSibList->tSib8Info.cellReselectionParametersHRPD_v920.neighCellList_v920.n)
    {
        ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present   = 0;
    }
    else
    {
        ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present   = 1;
    }

    /* 8. ��дIE: cellReselectionParameters1XRTT_v920 */

    if(0 == ptAllSibList->tSib8Info.cellReselectionParameters1XRTT_v920.neighCellList_v920.n)
    {
        ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present   = 0;
    }
    else
    {
        ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present   = 1;
    }    

    /* 9. ��дIE: csfb_RegistrationParam1XRTT_v920 */
    if(1 == ptGetSib8InfoAck->tSib8.ucPowerDownReg_r9)
    {
        ptAllSibList->tSib8Info.csfb_RegistrationParam1XRTT_v920.powerDownReg_r9 = true_8;
        ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present   = 1;
    }
    else
    {
        ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present   = 0;
    }

    /* 10. ��дIE: ac_BarringConfig1XRTT_r9��*/
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

    /* 11. ��дIE: __v3ExtPresent */   
    ptAllSibList->tSib8Info.m._v3ExtPresent = ptAllSibList->tSib8Info.m.csfb_SupportForDualRxUEs_r9Present
                                             |ptAllSibList->tSib8Info.m.cellReselectionParametersHRPD_v920Present
                                             |ptAllSibList->tSib8Info.m.cellReselectionParameters1XRTT_v920Present
                                             |ptAllSibList->tSib8Info.m.csfb_RegistrationParam1XRTT_v920Present
                                             |ptAllSibList->tSib8Info.m.ac_BarringConfig1XRTT_r9Present;

    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimFillSib9Info
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimFillSib9Info(T_DBS_GetSib9Info_ACK *ptGetSib9InfoAck, T_AllSibList *ptAllSibList)
{

    CCM_LOG(RNLC_INFO_LEVEL, "\n SI: Get SIB9 Info!aucHnbName[0]=%d\n",ptGetSib9InfoAck->tSib9.aucHnbName[0]);
    /* ��ʱ��ʵ�� */
    ptAllSibList->tSib9Info.m.hnb_NamePresent = 1;
    ptAllSibList->tSib9Info.hnb_Name.numocts = 1;
    ptAllSibList->tSib9Info.hnb_Name.data[0] = 1;

    ptAllSibList->tSib9Info.m.lateNonCriticalExtensionPresent = 0;

    return TRUE;
}
/* MIB&SIBs��Ԫ���end */

/*<FUNC>***********************************************************************
* ��������: CimEncodeFunc
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeFunc(BYTE ucEncodeInfoType, ASN1CTXT *ptCtxt, VOID *pMsg)
{
    int             iStat = ASN_OK;           /* ������ */
    static BYTE     aucEncodeBuf[1024 * 15];  /* �㲥���뻺����,��СΪ4K */

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCtxt);
    CCM_NULL_POINTER_CHECK(pMsg);

    memset(aucEncodeBuf, 0, sizeof(aucEncodeBuf));
/*��������������*/
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
        /* codec������Ϣ��ӡ */
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
    /* ����ɹ� */
    return TRUE;
}

/*<FUNC>***********************************************************************
* ��������: CimDecodeFunc
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimDecodeFunc(BYTE ucDecodeInfoType, ASN1CTXT *ptCtxt, VOID *pMsg)
{
    int    iStat = ASN_OK;   /* ��������־ */

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCtxt);
    CCM_NULL_POINTER_CHECK(pMsg);

    iStat = (*(g_atCimSIMsgInfoTable[ucDecodeInfoType].pDecoder))(ptCtxt, pMsg);
    if (ASN_OK != iStat)
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "\n SI: %s Decode fail! nStat = %d\n",g_atCimSIMsgInfoTable[ucDecodeInfoType].pchMsgName, iStat);
        /* codec������Ϣ��ӡ */
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
* ��������: CimToMacAlignment
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimToMacAlignment(WORD16 wMsgLen)
{
    WORD32 dwTableLoop;
    WORD32 dwStartPoint;  /* TB Size������ */
    WORD32 dwMidPoint;    /* TB Size����е� */
    WORD32 dwEndPoint;    /* TB Size����յ� */
    const WORD16 *pwTbSizeTable = NULL;

    static WORD16 awTbSizeFor1A[] =   /* ��������DCI-1A��TB Size�� */
    {
        4, 7, 9, 11, 13, 15, 18, 22, 26, 28, 32,
        37, 41, 47, 49, 55, 57, 61, 63, 69, 73, 75,
        79, 85, 87, 93, 97, 105, 113, 121, 125, 133, 141,
        145, 149, 157, 161, 173, 185, 201, 217, 225, 233, 277
    };

    static WORD16 awTbSizeFor1C[] =   /* ��������DCI-1C��TB Size�� */
    {
        5, 7, 9, 15, 17, 18, 22, 26, 28, 32, 35,
        37, 41, 42, 49, 61, 69, 75, 79, 87, 97, 105,
        113, 125, 133, 141, 153, 161, 173, 185, 201, 217
    };

    static WORD16 awTbSizeFor1A1C[] =   /* ������DCI-1A/1C��TB Size�� */
    {
        4, 5, 7, 9, 11, 13, 15, 17, 18, 22, 26,
        28, 32, 35, 37, 41, 42, 47, 49, 55, 57, 61,
        63, 69, 73, 75, 79, 85, 87, 93, 97, 105, 113,
        121, 125, 133, 141, 145, 149, 153, 157, 161, 173, 185,
        201, 217, 225, 233, 277
    };

    /* TB Size��ѡ��{0:��֧��DCI-1A��1:��֧��DCI-1C; 2:֧��DCI-1A/1C} */
    if (0 == g_dwTbSizeMode)
    {
        pwTbSizeTable = awTbSizeFor1A;
        dwEndPoint = (sizeof(awTbSizeFor1A) / sizeof(WORD16)) - 1; /* �±��0��ʼ */
    }
    else if (1 == g_dwTbSizeMode)
    {
        pwTbSizeTable = awTbSizeFor1C;
        dwEndPoint = (sizeof(awTbSizeFor1C) / sizeof(WORD16)) - 1; /* �±��0��ʼ */
    }
    else
    {
        pwTbSizeTable = awTbSizeFor1A1C;
        dwEndPoint = (sizeof(awTbSizeFor1A1C) / sizeof(WORD16)) - 1; /* �±��0��ʼ */
    }

    /* �������TB Size��֧�ַ�Χ,�򷵻�0 */
    if (pwTbSizeTable[dwEndPoint] < wMsgLen)
    {
        CCM_LOG(RNLC_WARN_LEVEL, "\n SI: Message code stream len(=%lu) beyond TB size boundary(=%lu)!\n",
                     wMsgLen, pwTbSizeTable[dwEndPoint]);
        return 0;
    }

    /* ��Ϣ����һ�㴦��TB Size����м䲿�֣����ô��е����������������� */
    /* ���ۺ��ָ�ʽ��TB size������[startPoint,midPoint)��(midPoint,endPoint] */
    /* ����>=2, ��ˣ����´��벻�ῼ���������ص���������� */
    dwMidPoint = dwEndPoint / 2;
    if (wMsgLen == pwTbSizeTable[dwMidPoint])
    {
        return pwTbSizeTable[dwMidPoint];
    }
    else if (wMsgLen < pwTbSizeTable[dwMidPoint])
    {
        /* �����Ϣ���ȴ��ڱ����벿��,����е����ͷ���� */
        dwStartPoint = dwMidPoint - 1;
        dwEndPoint = 0; /* �յ�Ϊ��ͷ */
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

        /* ���һֱ��������ͷ��û�з���ƥ���TB SIZE,���жϱ�ͷ�Ƿ���� */
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
        /* �����Ϣ���ȴ��ڱ���Ұ벿��,����е����β���� */
        dwStartPoint = dwMidPoint + 1; /* ���Ϊ�е��ұ� */
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
* ��������: CimToMacAlignment
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimReEncodeSib1(T_CimSIBroadcastVar *ptCimSIBroadcastVar, T_AllSibList *ptAllSibList,
                       T_BroadcastStream  *ptNewBroadcastStream)
{
    BCCH_DL_SCH_MessageType    tBcchDlSchMsg;
    BCCH_DL_SCH_MessageType_c1 tBcchDlSchMsgC1;
    ASN1CTXT  tCtxt;          /* ����������Ľṹ */
    WORD32 dwResult = FALSE;

    /* ���SIB1���� */
    ptNewBroadcastStream->ucSib1Len = 0;
    memset(ptNewBroadcastStream->aucSib1Buf, 0, sizeof(ptNewBroadcastStream->aucSib1Buf));

    /* ʹ�ø��º��ValueTage���±��� */
    ptAllSibList->tSib1Info.systemInfoValueTag = ptCimSIBroadcastVar->tSIBroadcastDate.ucSib1ValueTag;
    tBcchDlSchMsg.t = T_BCCH_DL_SCH_MessageType_c1;
    tBcchDlSchMsg.u.c1 = &tBcchDlSchMsgC1;
    tBcchDlSchMsgC1.t = T_BCCH_DL_SCH_MessageType_messageClassExtension;
    tBcchDlSchMsgC1.u.systemInformationBlockType1 = &(ptAllSibList->tSib1Info);

    /* ���±���SIB1 */
    dwResult = CimEncodeFunc(CIM_SI_SIB1, &tCtxt, (VOID *)(&tBcchDlSchMsg));
    if (TRUE == dwResult)
    {
        /* ����������Ϣ */
        ptNewBroadcastStream->ucSib1Len = (BYTE)pu_getMsgLen(&tCtxt);
        memcpy(ptNewBroadcastStream->aucSib1Buf, &(tCtxt.buffer.data[0]), ptNewBroadcastStream->ucSib1Len);

        if (CIM_SIB1_BUF_LEN < ptNewBroadcastStream->ucSib1Len)
        {
            /* ����Խ�� */
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
* ��������: CimFourEncodeSib1Stream
* ��������: ����sib10 ��sib11��sib12��������������ϳ����µ��ĸ�sib1
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: word32
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    /*��û��sib11 ��sib12 ����������*/
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

    /**ֻ��sib11��sib1��������*/
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
        /*����ǷǷ��ģ���Ϊ��һ��Ĭ��ֵ*/
        ucSavedSiPeriod = rf16_1;
    }
    /*lint -restore*/
    tSchedulingInfoList.elem[wCurrentN].si_Periodicity = (SchedulingInfo_si_Periodicity) ucSavedSiPeriod;
    (tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n)++;
    (tSchedulingInfoList.n)++;
    /* ���µ�����Ϣ */
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

    /**������sib11��sib12��sib1��������*/
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
        /*����ǷǷ��ģ���Ϊ��һ��Ĭ��ֵ*/
        ucSavedSiPeriod = rf16_1;
    }
    /*lint -restore*/
    tSchedulingInfoList.elem[wCurrentN].si_Periodicity = (SchedulingInfo_si_Periodicity) ucSavedSiPeriod;
    (tSchedulingInfoList.elem[wCurrentN].sib_MappingInfo.n)++;
    (tSchedulingInfoList.n)++;
    /* ���µ�����Ϣ */
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

    /*ֻ��sib12���������� �ȸ��µ����б�*/
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
* ��������: CimToMacAlignment
* ��������: 4�ֽڶ���
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimEncodeAlignment(WORD16 wLen)
{
    WORD16 wModResult;    /* ģ��Ľ�� */

    /* ģ4 ���� */
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
* ��������: CimFillSib8No
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimFillSib8No(SchedulingInfoList *ptSchedulingInfoList, WORD16  *pwSib8SiNo)
{
    WORD32 dwSiLoop;    /* SI���� */
    WORD32 dwSibLoop;   /* ÿ��SI��SIB���� */
    WORD32 dwSibCount;  /* ÿ��SI��SIB���� */
    WORD32 dwSibName;  /* SIB�� */
    BYTE   ucSiNum;    /* ��ǰSI */

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
                ucSiNum = (BYTE)dwSiLoop;/*�û����0��λ�ö�Ӧ����sib1,����sib�Ǵ�1��λ�ÿ�ʼ*/
                *pwSib8SiNo = ucSiNum & 0xFF;    /* ��ʱ������ȱ���ͨ���������޸� */
                return ;
            }
        }
    }

    *pwSib8SiNo = 0xffff;

    return;
}

/*<FUNC>***********************************************************************
* ��������: CimGetSiMaxTransNum
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSiMaxTransNum(BYTE ucSIWindowLength)
{
    /* TS36.331v850:si-WindowLength ENUM{ms1,ms2,ms5,ms10,ms15,ms20,ms40} */
    BYTE aucSiWndLen[] = {1, 2, 5, 10, 15, 20, 40};

    /* �ж���εĺϷ��� */
    if (sizeof(aucSiWndLen) <= ucSIWindowLength)
    {
        CCM_LOG(RNLC_WARN_LEVEL, "\n SI: Received invalid SI-WndLenIndex[=%u]!\n",ucSIWindowLength);
        /* SI�ڴ����ڵ���������ȡΪ��Сֵ1 */
        return 1;
    }

    /* ��Ϊ����汾������Ϊ4����֡�����SI�ش���������=floor(SiWndLen/4),
       Ȼ���ټ����״δ��䣬��ΪSI�������������� */
    return (BYTE)(1 + (aucSiWndLen[ucSIWindowLength] / 4));
}

/*<FUNC>***********************************************************************
* ��������: CimDbgShowCurBroadcastInfo
* ��������: ��ʾ�·��ĵ�����Ϣ������
*           SI-1 = {SIB2}
*           SI-2 = {SIB3 SIB4 SIB5}
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
    BYTE   aucShowString[500];  /* ��ʾ�·��㲥���ݵ��ַ����������޶�Ϊ500�ֽ� */
    BYTE   aucTmpString[28];    /* ��ʽ���õ���ʱ�ַ��������Ȳ��ᳬ��28�ֽ� */
    BYTE   aucBroadcastInfo[14]; /* ��¼��SIB�Ƿ��·���0: ��; 1: �� */

    CCM_NULL_POINTER_CHECK_VOID(ptSchedulInfoList);
    /* ��ʼ����ز��� */
    dwSiNum = ptSchedulInfoList->n;
    memset((BYTE *)aucBroadcastInfo, 0, sizeof(aucBroadcastInfo));
    memset((BYTE *)aucShowString, 0, sizeof(aucShowString));

    /* ����SIB1�еĵ�����Ϣ����ȡ�·��Ĺ㲥���� */
    /*lint -save -e1773 */
    iRet = sprintf((char *)aucShowString, (char *)"The broadcasting system infomations are: \nMIB, SIB1\n");
    /*lint -restore*/
    for (dwSiLoop = 0; dwSiLoop < dwSiNum; ++dwSiLoop)
    {
        dwSibNumInSi = ptSchedulInfoList->elem[dwSiLoop].sib_MappingInfo.n;
        memset(aucBroadcastInfo, 0, sizeof(aucBroadcastInfo));

        /* ������ǰSI��ӳ����Ϣ�б�,�ҳ��������SIB */
        for (dwSibLoop = 0 ; dwSibLoop < dwSibNumInSi; ++dwSibLoop)
        {
            dwSibType = ptSchedulInfoList->elem[dwSiLoop].sib_MappingInfo.elem[dwSibLoop];
            aucBroadcastInfo[dwSibType + 3] = 1; /* ��3��ʾ���ǵ�MIB,SIB1��SIB2 */
        }

        /* ���ǵ�SIB2���ر��� */
        if (0 == dwSiLoop)
        {
            /* ����ǵ�һ��SI,��SIB2�ı�־λ��1 */
            aucBroadcastInfo[2] = 1;
        }
        /*lint -save -e1773 */
        sprintf((char *)aucTmpString, (char *)"SI-%lu = {", dwSiLoop + 1);
        strcat((char *)aucShowString, (char *)aucTmpString);
        /*lint -restore*/
        /* ��֯SI��Ϣ�ַ��� */
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
* ��������: CimDbgPrintMsg
* ��������:
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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
* ��������: CimDbgBroadcastStreamPrint
* ��������: ��ʾMIB,SIB1,SI����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
VOID CimDbgBroadcastStreamPrint(T_BroadcastStream *ptBroadcastStream,
                                T_AllSiEncodeInfo *ptAllSiEncodeInfo)
{
    WORD32 dwSiNumLoop;
    WORD16 wSiLen;

    /* MIB������ӡ */
    CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: MIB stream as follow. len=%d!\n",ptBroadcastStream->ucMibLen);
    CimDbgPrintMsg(ptBroadcastStream->aucMibBuf, ptBroadcastStream->ucMibLen);

    /* SIB1������ӡ */
    CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SIB1 stream as follow. len=%d!\n",ptBroadcastStream->ucSib1Len);
    CimDbgPrintMsg(ptBroadcastStream->aucSib1Buf, ptBroadcastStream->ucSib1Len);

    /* ����SI������ӡ */
    for (dwSiNumLoop = 0; dwSiNumLoop < ptAllSiEncodeInfo->dwSiNum; dwSiNumLoop++)
    {
        if (FALSE == ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].ucValid)
        {
            /* ��SI����ʧ�ܻ��߲��ܵ��ȣ����·� */
            CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SI-%lu not be send!\n",dwSiNumLoop + 1);
            continue;
        }

        wSiLen = ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].wMsgLen;
        CCM_LOG(RNLC_DEBUG_LEVEL, "\n SI: SI-%lu stream as follow. len=%d!\n",dwSiNumLoop + 1, wSiLen);
        CimDbgPrintMsg(ptAllSiEncodeInfo->atSiEcode[dwSiNumLoop].aucMsg, wSiLen);
    }
}
/*<FUNC>***********************************************************************
* ��������: CimDbgConfigSISch
* ��������: ������Ϣ���������ڵ�R_SISCHE��ΪB�������ز����ȴ�׮����
* �㷨����:
* ȫ�ֱ���: ��
* Input����:
* �� �� ֵ: VOID
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
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

    /* ��sib2��sib10��������SI�� */
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

    /* ��sib2��sib10��������SI�� */
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
    ASN1CTXT  tCtxt;          /* ����������Ľṹ */

    CCM_NULL_POINTER_CHECK(ptWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptCimData);
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);

    /* �ӵ�0���ظ����룬��ΪҪ��¼SIB���� */
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
    /*��¼sib11�ķ�Ƭ��Ŀ*/
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum = ptWarningMsgSegmentList->dwSib11SegNum;

    return TRUE;
}

WORD32 CimEnCodeSib12SegList(TCmasWarningMsgSegmentList *ptCmasWarningMsgSegmentList, T_CIMVar *ptCimData,T_CimSIBroadcastVar *ptCimSIBroadcastVar)
{
    WORD32       dwResult = FALSE;
    WORD32    dwLoop = 0;
    WORD16    wAllSegSize = 0;
    WORD16    wTempLen = 0;
    ASN1CTXT  tCtxt;          /* ����������Ľṹ */

    CCM_NULL_POINTER_CHECK(ptCmasWarningMsgSegmentList);
    CCM_NULL_POINTER_CHECK(ptCimData);
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);

    /* �ӵ�0���ظ����룬��ΪҪ��¼SIB���� */
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
    /*��¼sib12�ķ�Ƭ��Ŀ*/
    ptCimSIBroadcastVar->tCimWarningInfo.dwSib12SegStreamNum = ptCmasWarningMsgSegmentList->dwSib12SegNum;
    

    return TRUE;
}



/*<FUNC>***********************************************************************
* ��������: BcmiSetSib10SpecialTimer
* ��������: ����Sib10��ʱ��,����֮��������OMC Timer���п���
* �㷨����:
* ȫ�ֱ���:
* Input����: TBcmiCellInstData *ptBcmiCellInstData ---- [in]ָ��BCMIʵ����������ָ��
* �������: ��
* �� �� ֵ: BOOLEAN
**    
*    
* ��    ��: V1.0
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib10SpecialTimer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId = INVALID_TIMER_ID; /* ��ʱ��ID */
    WORD32  dwDuration = 0;
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    dwDuration = g_dwRnlcCcmDbgSib10TimerLen;
    CCM_SIOUT_LOG(RNLC_INFO_LEVEL, ptCimSIBroadcastVar->wCellId,"\n SI: (CBE OMC HYBRID MODE)CimSetSib10SpecialTimer Timer duration  =%d \n", dwDuration);

    /* ���ö�ʱ�� */
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
* ��������: BcmiSetSib11Timer
* ��������: ����Sib11��ʱ��
* �㷨����:
* ȫ�ֱ���:
* Input����: TBcmiCellInstData *ptBcmiCellInstData ---- [in]ָ��BCMIʵ����������ָ��
* �������: ��
* �� �� ֵ: BOOLEAN
**    
*    
* ��    ��: V1.0
* �޸ļ�¼:
*    
    
    *P�ķ�ʽ�ٷ�Ƭ����������һ�ο��ܷ��Ͳ�ȫ���޸�
                                                     Ϊ��֤���һ��Sib11��Ƭ���(��ʱ�п��ܻ����N*Pʱ��)
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib11Timer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId           = INVALID_TIMER_ID; /* ��ʱ��ID */
    WORD32  dwDuration          = 0;                /* ��ʱ����ʱʱ�� */
    WORD32  dwSingleSib11Period = 0;                /* һ������SIB11������ */
    WORD32  dwSiBlockSchTime    = 0;                /* SIB11�ĵ������� */

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    /* ����һ��Sib11������ */
    dwSiBlockSchTime    = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod);
    dwSingleSib11Period = dwSiBlockSchTime * ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum;

    /* ����NumberofBroadcastRequest*RepetitionPeriod�����Ӧ�÷��Ͷ���ms */
    dwDuration = 1000 * ptCpmCcmWarningMsgReq->wRepetitionPeriod * ptCpmCcmWarningMsgReq->wNumberofBroadcastRequest;
    if ((dwDuration % dwSingleSib11Period ) != 0)
    {
        /* ���������ڶ�Sib11ȡ������,���ü�����������Ϊ���ܱ�֤dwDuration����dwSingleSib11Period */
        dwDuration = ((dwDuration / dwSingleSib11Period) + 1) * dwSingleSib11Period;
    }

    /* ���ö�ʱ�� */
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
* ��������: CimSetSib12Timer
* ��������: ����Sib11��ʱ��
* �㷨����:
* ȫ�ֱ���:
* Input����: TBcmiCellInstData *ptBcmiCellInstData ---- [in]ָ��BCMIʵ����������ָ��
* �������: ��
* �� �� ֵ: BOOLEAN
**    
*    
* ��    ��: V1.0
* �޸ļ�¼:
*    
    
    *P�ķ�ʽ�ٷ�Ƭ����������һ�ο��ܷ��Ͳ�ȫ���޸�
                                                     Ϊ��֤���һ��Sib11��Ƭ���(��ʱ�п��ܻ����N*Pʱ��)
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib12Timer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId           = INVALID_TIMER_ID; /* ��ʱ��ID */
    WORD32  dwDuration          = 0;                /* ��ʱ����ʱʱ�� */
    WORD32  dwSingleSib12Period = 0;                /* һ������SIB11������ */
    WORD32  dwSiBlockSchTime    = 0;                /* SIB11�ĵ������� */

    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    /* ����һ��Sib12������ */
    dwSiBlockSchTime    = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB12SiPeriod);
    dwSingleSib12Period = dwSiBlockSchTime * ptCimSIBroadcastVar->tCimWarningInfo.dwSib12SegStreamNum;

    /* ����NumberofBroadcastRequest*RepetitionPeriod�����Ӧ�÷��Ͷ���ms */
    dwDuration = 1000 * ptCpmCcmWarningMsgReq->wRepetitionPeriod * ptCpmCcmWarningMsgReq->wNumberofBroadcastRequest;
    if ((dwDuration % dwSingleSib12Period ) != 0)
    {
        /* ���������ڶ�Sib11ȡ������,���ü�����������Ϊ���ܱ�֤dwDuration����dwSingleSib11Period */
        dwDuration = ((dwDuration / dwSingleSib12Period) + 1) * dwSingleSib12Period;
    }

   WORD32 dwCmasFlag = 0;
   /*�˴��ĸ��Ʋ����ǴӸ�λ��ʼ�ģ��м�*/
   memcpy(&dwCmasFlag,ptCpmCcmWarningMsgReq->tMessageIdentifier.data,2);
   /*��λ�洢����Ϣid�����Ƶ���λ*/
   dwCmasFlag = dwCmasFlag>>16;
   /*��msg id����������洢�ڸ�16λ*/
   memcpy(&dwCmasFlag,ptCpmCcmWarningMsgReq->tSerialNumber.data,2);

    /* ���ö�ʱ�� */
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
* ��������: CimSetSib11SpecialTimer
* ��������: ����Sib11��ʱ��,��Ҫ���������Ϊ0������Ϊ1���������
* �㷨����:
* ȫ�ֱ���:
* Input����: TBcmiCellInstData *ptBcmiCellInstData ---- [in]ָ��BCMIʵ����������ָ��
* �������: ��
* �� �� ֵ: BOOLEAN
**    
*    
* ��    ��:
* �޸ļ�¼:
*    
    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimSetSib11SpecialTimer(T_CimSIBroadcastVar *ptCimSIBroadcastVar, TCpmCcmWarnMsgReq* ptCpmCcmWarningMsgReq)
{
    WORD32  dwTimerId        = INVALID_TIMER_ID; /* ��ʱ��ID */
    WORD32  dwDuration       = 0;
    WORD32  dwSiBlockSchTime = 0;
    /* ��μ�� */
    CCM_NULL_POINTER_CHECK(ptCimSIBroadcastVar);
    CCM_NULL_POINTER_CHECK(ptCpmCcmWarningMsgReq);

    dwSiBlockSchTime = CimGetSiPeriodMs(ptCimSIBroadcastVar->tCimWarningInfo.ucSIB11SiPeriod);
    dwDuration =  dwSiBlockSchTime*(ptCimSIBroadcastVar->tCimWarningInfo.dwSib11SegStreamNum);

    /* ���ö�ʱ�� */
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
* ��������: BcmGetSiPeriodMs
* ��������: ���ݵ������ڵ�������ȡʵ�ʵĵ���ʱ��
* �㷨����: SchedulingInfo_si_Periodicity �������Ԫ�����仯����ô�˺�����Ҫ�޸�
* ȫ�ֱ���:
* Input����: BYTE byIndex ---- ����
* �� �� ֵ: WORD32      ʱ�䵥λΪms
**    
*    
* ��    ��: V1.0
* �޸ļ�¼:
*    
    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32 CimGetSiPeriodMs(WORD32 dwSiPeriodIndex)
{
    WORD32 dwReturnTime          = 80;                                    /* SIPERIOD_80 Ĭ��ȡֵ */
    WORD32 adwS1PeriodMsTable[7] = {80, 160, 320, 640, 1280, 2560, 5120}; /* according to 36.331 930 6.2.2 */

    if (dwSiPeriodIndex >= (sizeof(adwS1PeriodMsTable)/sizeof(WORD32)))
    {
        /* �ֶ�ȡֵ���󣬴�ӡ�澯 */
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                        RNLC_INVALID_DWORD, 
                        dwSiPeriodIndex,
                        RNLC_ERROR_LEVEL, 
                        "tSiPeriodIndex=%lu not enumerate(rf8��rf16��rf32��rf64, rf128, rf256, rf512)",
                        dwSiPeriodIndex);
    }
    else
    {
        dwReturnTime = adwS1PeriodMsTable[dwSiPeriodIndex];
    }
    return dwReturnTime;
}
