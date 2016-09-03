/****************************************************************************
* ��Ȩ���� (C)2013 ����������ͨ
*
* �ļ�����: ccm_cim_celldelcomponent.cpp
* �ļ���ʶ:
* ����ժҪ: С��ɾ������ʵ���ļ�
* ����˵��: 
* ��ǰ�汾: V3.0
**    
*    
*
* �޸ļ�¼1: 
*    
***************************************************************************/

/*****************************************************************************/
/*               #include������Ϊ��׼��ͷ�ļ����Ǳ�׼��ͷ�ļ���            */
/*****************************************************************************/
#include "rnlc_common.h"

#include "pub_lte_rnlc_rrm_interface.h"
#include "pub_lte_rnlc_oam_interface.h"
#include "pub_lte_rnlc_rnlu_interface.h"

#include "ccm_timer.h"
#include "ccm_common_struct.h"
#include "ccm_eventdef.h"
#include "ccm_debug.h"
#include "ccm_common.h"
#include "usf_bpl_pub.h"

#include "ccm_cim_common.h"
#include "ccm_cim_celldelcomponent.h"

#include "rnlc_ccm_dcm_interface.h"

/*****************************************************************************/
/*                                ��������                                   */
/*****************************************************************************/
/* С��ɾ�������ڲ���ʱ������ */
const WORD16 TIMER_RRMCELLDEL_DURATION = 65000;
const WORD16 TIMER_CELLDEL_DURATION = 10000;

/* С��ɾ�������Ƿ��յ��ⲿϵͳС��ɾ����Ӧ�ı�־ */
const BYTE NULL_REL_COMPLETE = 0;
const BYTE RRU_REL_COMPLETE  = 1;
const BYTE RNLU_REL_COMPLETE = 2;
const BYTE BB_REL_COMPLETE   = 4;
const BYTE BBALG_REL_COMPLETE = 8;
const BYTE ALL_REL_COMPLETE  = 15;


/* С��ɾ�������ڲ�״̬��־ */
const BYTE S_CIM_DELCOM_IDLE = 0;
const BYTE S_CIM_DELCOM_WAIT_RRMDELRSP = 1;
const BYTE S_CIM_DELCOM_WAIT_DELRSP = 2;
const BYTE S_CIM_DELCOM_WAIT_DELOK = 3;

/*****************************************************************************/
/*                            �ļ��ڲ�ʹ�õĺ�                               */
/*****************************************************************************/

/*****************************************************************************/
/*                         �ļ��ڲ�ʹ�õ���������                            */
/*****************************************************************************/


/*****************************************************************************/
/*                                 ȫ�ֱ���                                  */
/*****************************************************************************/
/* ��ȫ�ֱ������������Ƶ��Ե�ʱ���Ƿ�����ⲿϵͳ�ı�־ */
extern T_SystemDbgSwitch g_tSystemDbgSwitch;

/*****************************************************************************/
/*                        ���ر���������̬ȫ�ֱ�����                         */
/*****************************************************************************/


/*****************************************************************************/
/*                              �ֲ�����ԭ��                                 */
/*****************************************************************************/

/*****************************************************************************/
/*                                ���ʵ��                                   */
/*****************************************************************************/

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Idle
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellDelComponentFsm::Idle(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_REL_REQ:
        {
            Handle_CIMDelReq(pMsg);
            break;
        }

        case CMD_CIM_MASTER_TO_SLAVE:\
        {\
            HandleMasterToSlave(pMsg);\
            break;\
        }
        
        default:
        {
            Handle_ErrorMsg(pMsg);
            break;
        }
    }
    
    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::WaitRRMDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellDelComponentFsm::WaitRRMDelRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_RRM_RNLC_CELL_REL_RSP:
        {
            Handle_RRMDelRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_RRMCELLDEL_TIMEOUT:
        {
            Handle_RRMTimeOut(pMsg);           
            break;
        }        
        default:
        {
            Handle_ErrorMsg(pMsg);
            break;
        }
    }

    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::WaitDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellDelComponentFsm::WaitDelRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_RRU_RNLC_CELL_REL_RSP:
        {
            Handle_RRUDelRsp(pMsg);
            break;
        }
        case EV_RNLU_RNLC_CELL_REL_RSP:
        {
            Handle_RNLUDelRsp(pMsg);
            break;
        }
        case EV_BB_RNLC_CELL_REL_RSP:
        {
            Handle_BBDelRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_SYSCELLDEL_TIMEOUT:
        {
            Handle_OtherTimeOut(pMsg);
            break;
        }        
        case CMD_CIM_BBALG_DEL_RSP:
        {
            Handle_BBDelAlgRsp(pMsg);
            break;
        }
        default:
        {
            Handle_ErrorMsg(pMsg);
            break;
        }
    }

    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::DelOK
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellDelComponentFsm::DelOK(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
      case CMD_CIM_MASTER_TO_SLAVE:\
      {\
          HandleMasterToSlave(pMsg);\
          break;\
      }
      
      default:
      {
          Handle_ErrorMsg(pMsg);
          break;
      }
    }

    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Init
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellDelComponentFsm::Init(VOID)
{
    InitComponentContext();

    BYTE  ucBoardMSState = BOARD_MS_UNCERTAIN;
    
    ucBoardMSState = OSS_GetBoardMSState();
    
    if (BOARD_SLAVE == ucBoardMSState)
    {
            CimPowerOnInSlave();
            SetComponentSlave();
            return;
    }

    TranStateWithTag(CCM_CIM_CellDelComponentFsm, Idle, S_CIM_DELCOM_IDLE);
    CIMPrintTranState("S_CIM_DELCOM_IDLE", S_CIM_DELCOM_IDLE);


    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_CIMDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_CIMDelReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_CmdCIMCellDelReq));
    CIMPrintRecvMessage("CMD_CIM_CELL_REL_REQ", pMsg->m_dwSignal);
        
    const T_CmdCIMCellDelReq *ptCmdCIMCellDelReq = 
                            static_cast<T_CmdCIMCellDelReq*>(pMsg->m_pSignalPara);

    m_ucCellDelReason = (BYTE)ptCmdCIMCellDelReq->dwCause;
    
    WORD32 dwResult = SendToRRMDelReqOrNot(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRMDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRRMDelReqOrNot!");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRRMDelReqOrNot;
    }

    WORD32 dwRRMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_RRMCELLDEL,
                                                      TIMER_RRMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwRRMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellDelComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellDelVar.dwRRMCellDelTimerId = dwRRMCellDelTimerId;
  
    TranStateWithTag(CCM_CIM_CellDelComponentFsm, WaitRRMDelRsp, S_CIM_DELCOM_WAIT_DELRSP);
    CIMPrintTranState("S_CIM_DELCOM_WAIT_DELRSP", S_CIM_DELCOM_WAIT_DELRSP);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_PRMDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_PRMDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_RRMDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_RRMDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_RrmRnlcCellRelRsp));
    CIMPrintRecvMessage("EV_RRM_RNLC_CELL_REL_RSP", pMsg->m_dwSignal);

    const T_RrmRnlcCellRelRsp *ptRrmRnlcCellRelRsp = 
                     static_cast<T_RrmRnlcCellRelRsp*>(pMsg->m_pSignalPara);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RRM_RNLC_CELL_REL_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RrmRnlcCellRelRsp),
                     (const BYTE *)(ptRrmRnlcCellRelRsp));
    
    CCM_CIM_CELLID_CHECK(ptRrmRnlcCellRelRsp->wCellId, 
                     GetJobContext()->tCIMVar.wCellId);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                            tCIMCellDelVar.dwRRMCellDelTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellDelComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
        
    if (0 != ptRrmRnlcCellRelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RRMREL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRrmRnlcCellRelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm RRM Del Failed! ");

         /* RRM �ͷ�ʧ�ܣ�ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
            û�д������,����¼��RRM�ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
         GetComponentContext()->tCIMCellDelVar.dwRRMResult = ERR_CCM_CIM_RRMREL_FAIL;
    }
    else
    {
        GetComponentContext()->tCIMCellDelVar.dwRRMResult = CCM_CELLRRMDEL_OK;
    }

    dwResult = SendCellDelToBbAlg();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToBBDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendCellDelToBbAlg");
    }

    /* ���Ӹ�dcm�ͷ���Ϣ */
    dwResult = SendCellDelToDcm(ptRrmRnlcCellRelRsp->wCellId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendCellDelToDcm,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendCellDelToDcm");
        
        //return ERR_CCM_CIM_FunctionCallFail_SendCellDelToDcm;
    }

    dwResult = SendToRRUDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRRUDelReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReqOrNot;
    }

    dwResult = SendToRNLUDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRNLUDelReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReqOrNot;
    }

    dwResult = SendToBBDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToBBDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToBBDelReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToBBDelReqOrNot;
    }
    
    WORD32 dwCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_SYSCELLDEL, 
                                                   TIMER_CELLDEL_DURATION, 
                                                   PARAM_NULL);
    if (INVALID_TIMER_ID == dwCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellDelComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellDelVar.dwCellDelTimerId = dwCellDelTimerId;

    TranStateWithTag(CCM_CIM_CellDelComponentFsm, WaitDelRsp, S_CIM_DELCOM_WAIT_DELRSP);
    CIMPrintTranState("S_CIM_DELCOM_WAIT_DELRSP", S_CIM_DELCOM_WAIT_DELRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_RRUDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_RRUDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_RruRnlcCellRelRsp));
    CIMPrintRecvMessage("EV_RRU_RNLC_CELL_REL_RSP", pMsg->m_dwSignal);

    const T_RruRnlcCellRelRsp *ptRruRnlcCellRelRsp = 
                         static_cast<T_RruRnlcCellRelRsp*>(pMsg->m_pSignalPara);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RRU_RNLC_CELL_REL_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RruRnlcCellRelRsp),
                     (const BYTE *)(ptRruRnlcCellRelRsp));

    CCM_CIM_CELLID_CHECK(ptRruRnlcCellRelRsp->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    if (0 != ptRruRnlcCellRelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RRUREL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRruRnlcCellRelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm RRU Del Failed! ");

        /* RRU �ͷ�ʧ�ܣ�ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
        û�д������,����¼��RRU�ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
        GetComponentContext()->tCIMCellDelVar.dwRRUResult = ERR_CCM_CIM_RRUREL_FAIL;         
    }
    else
    {
        GetComponentContext()->tCIMCellDelVar.dwRRUResult = CCM_CELLRRUDEL_OK;
    }
    
    SetCompleteSystemFlg(RRU_REL_COMPLETE, 
                         &(GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg));    
    if (GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                             GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg) 
                             == ALL_REL_COMPLETE)
    {      
        WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext() \
                                             ->tCIMCellDelVar.dwCellDelTimerId);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_CellDelComponentFsm Kill Timer Failed!");
            
            return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
        }
        
        dwResult = SendToMainComDelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComDelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp;
        }

        TranStateWithTag(CCM_CIM_CellDelComponentFsm, 
                         DelOK, 
                         S_CIM_DELCOM_WAIT_DELOK);
        CIMPrintTranState("S_CIM_DELCOM_WAIT_DELOK", 
                           S_CIM_DELCOM_WAIT_DELOK);
              
        return RNLC_SUCC;
    }
   
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_RNLUDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_RNLUDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_RnluRnlcCellRelRsp));
    CIMPrintRecvMessage("EV_RNLU_RNLC_CELL_REL_RSP", pMsg->m_dwSignal);

    const T_RnluRnlcCellRelRsp *ptRnluRnlcCellRelRsp = 
                        static_cast<T_RnluRnlcCellRelRsp*>(pMsg->m_pSignalPara);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLU_RNLC_CELL_REL_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RnluRnlcCellRelRsp),
                     (const BYTE *)(ptRnluRnlcCellRelRsp));

    CCM_CIM_CELLID_CHECK(ptRnluRnlcCellRelRsp->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    if (0 != ptRnluRnlcCellRelRsp->dwResult)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RNLUREL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRnluRnlcCellRelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm RNLU Del Failed! ");

        /* RNLU �ͷ�ʧ�ܣ�ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
        û�д������,����¼��RNLU�ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
        GetComponentContext()->tCIMCellDelVar.dwRNLUResult = ERR_CCM_CIM_RNLUREL_FAIL;         
    }
    else
    {
        GetComponentContext()->tCIMCellDelVar.dwRNLUResult = CCM_CELLRNLUDEL_OK;
    }
    
    SetCompleteSystemFlg(RNLU_REL_COMPLETE, 
                        &(GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg));    
    if (GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                             GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg) 
                             == ALL_REL_COMPLETE)
    {
        WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()-> \
                                               tCIMCellDelVar.dwCellDelTimerId);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_CellDelComponentFsm Kill Timer Failed!");
            
            return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
        }
        
        dwResult = SendToMainComDelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComDelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp;
        }

        TranStateWithTag(CCM_CIM_CellDelComponentFsm, 
                         DelOK, 
                         S_CIM_DELCOM_WAIT_DELOK);
        CIMPrintTranState("S_CIM_DELCOM_WAIT_DELOK", 
                           S_CIM_DELCOM_WAIT_DELOK);
         
        return RNLC_SUCC;
    }
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_BBDelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_BBDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_BbRnlcCellRelRsp));
    CIMPrintRecvMessage("EV_BB_RNLC_CELL_REL_RSP", pMsg->m_dwSignal);

    const T_BbRnlcCellRelRsp *ptBbRnlcCellRelRsp = 
                          static_cast<T_BbRnlcCellRelRsp*>(pMsg->m_pSignalPara);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_BB_RNLC_CELL_REL_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_BbRnlcCellRelRsp),
                     (const BYTE *)(ptBbRnlcCellRelRsp));

    CCM_CIM_CELLID_CHECK(ptBbRnlcCellRelRsp->tMsgHeader.wL3CellId, 
                         GetJobContext()->tCIMVar.wCellId);

    if (0 != ptBbRnlcCellRelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_BBREL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptBbRnlcCellRelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm BB Del Failed! ");

        /* BB �ͷ�ʧ�ܣ�ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
        û�д������,����¼��BB�ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
        GetComponentContext()->tCIMCellDelVar.dwBBResult = ERR_CCM_CIM_BBREL_FAIL;                  
    }
    else
    {
        GetComponentContext()->tCIMCellDelVar.dwBBResult = CCM_CELLBBDEL_OK;
    }
    
    SetCompleteSystemFlg(BB_REL_COMPLETE, 
                         &(GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg));
    if (GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                            (GetComponentContext())->tCIMCellDelVar.ucCompleteSystemFlg) 
                            == ALL_REL_COMPLETE)
    {
        WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()-> \
                                               tCIMCellDelVar.dwCellDelTimerId);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_CellDelComponentFsm Kill Timer Failed!");
            
            return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
        }
        
        dwResult = SendToMainComDelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComDelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp;
        }

        TranStateWithTag(CCM_CIM_CellDelComponentFsm, 
                         DelOK, 
                         S_CIM_DELCOM_WAIT_DELOK);
        CIMPrintTranState("S_CIM_DELCOM_WAIT_DELOK", 
                           S_CIM_DELCOM_WAIT_DELOK);
        
        return RNLC_SUCC;
    }
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_RRMTimeOut
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_RRMTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    RnlcLogTrace(PRINT_RNLC_CCM,  //����ģ��� 
                 __FILE__,        //�ļ�����
                 __LINE__,        //�ļ��к�
                 GetJobContext()->tCIMVar.wCellId,  //С����ʶ 
                 RNLC_INVALID_WORD,  //UE GID 
                 ERR_CCM_CIM_RRMREL_TIMEOUT,  //�쳣̽������� 
                 GetJobContext()->tCIMVar.wInstanceNo,  //�쳣̽�븨����
                 pMsg->m_dwSignal,  //�쳣̽����չ������
                 RNLC_FATAL_LEVEL,  //��ӡ����
                 "\n CCM_CIM_CellDelComponentFsm RRM Del TimeOut! \n");
    CIMPrintRecvMessage("TimeOut Msg", pMsg->m_dwSignal);

    /* RRM �ͷų�ʱ��ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
    û�д������,����¼��RRM�ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
    GetComponentContext()->tCIMCellDelVar.dwRRMResult = ERR_CCM_CIM_RRMREL_TIMEOUT;

    WORD32 dwResult = SendToRRUDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRRUDelReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReqOrNot;
    }

    dwResult = SendToRNLUDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRNLUDelReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReqOrNot;
    }

    dwResult = SendToBBDelReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToBBDelReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToBBDelReq! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToBBDelReqOrNot;
    }
    
    WORD32 dwCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_SYSCELLDEL, 
                                                   TIMER_CELLDEL_DURATION, 
                                                   PARAM_NULL);
    if (INVALID_TIMER_ID == dwCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellDelComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellDelVar.dwCellDelTimerId = dwCellDelTimerId;

    TranStateWithTag(CCM_CIM_CellDelComponentFsm, WaitDelRsp, S_CIM_DELCOM_WAIT_DELRSP);
    CIMPrintTranState("S_CIM_DELCOM_WAIT_DELRSP", S_CIM_DELCOM_WAIT_DELRSP);

    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellDelComponentFsm::Handle_OtherTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CIMPrintRecvMessage("OtherTimeOut", pMsg->m_dwSignal);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    
    /* ĳһ��Χ�豸�ͷų�ʱ��ҲҪ������������ͷ����̣�Ŀǰ��ɾ��С��ʧ�ܵ����
    û�д������,����¼���ͷ�ʧ�ܣ���ɾ�����������򲻷��ء�*/
    if (0 == GetCompleteSystemFlg(RRU_REL_COMPLETE, 
                    GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg)) 
    {

        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RRUREL_TIMEOUT,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm RRU Del Failed! ");

        GetComponentContext()->tCIMCellDelVar.dwRRUResult = ERR_CCM_CIM_RRUREL_TIMEOUT;
    
    }
    
    if (0 == GetCompleteSystemFlg(BB_REL_COMPLETE, 
                    GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg)) 
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_BBREL_TIMEOUT,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm BB Del Failed! ");
     
        GetComponentContext()->tCIMCellDelVar.dwBBResult = ERR_CCM_CIM_BBREL_TIMEOUT;
    }
    
    if (0 == GetCompleteSystemFlg(RNLU_REL_COMPLETE, 
                    GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg)) 
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RNLUREL_TIMEOUT,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm RNLU Del Failed! ");

         GetComponentContext()->tCIMCellDelVar.dwRNLUResult = ERR_CCM_CIM_RNLUREL_TIMEOUT;
    }

    /* ������������ǣ��ܽ��˳�ʱ�����֧��ȴ���ֽ��յ���ÿ����Χϵͳ����Ӧ
        ��������������ǲ����ܷ����ģ�����д�������ӡ������һ�°�! */
    if (ALL_REL_COMPLETE == GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                     GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg)) 
    {
         CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLDEL_STATE_ERROR,
                                 GetJobContext()->tCIMVar.wInstanceNo,
                                 pMsg->m_dwSignal,
                                 RNLC_FATAL_LEVEL, 
                                 " CCM_CIM_CellDelComponentFsm STATE_ERROR! ");
    }

    /*��ʱ����ʱ�󣬸�CMM�ظ���Ӧ */
    WORD32 dwResult = SendToMainComDelRsp();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToMainComDelRsp! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp;
    }

    TranStateWithTag(CCM_CIM_CellDelComponentFsm, 
                     DelOK, 
                     S_CIM_DELCOM_WAIT_DELOK);
    CIMPrintTranState("S_CIM_DELCOM_WAIT_DELOK", 
                       S_CIM_DELCOM_WAIT_DELOK);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::Handle_ErrorMsg
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_ErrorMsg(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CIMPrintRecvMessage("ErrorMsg", pMsg->m_dwSignal);
    
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_ErrorMsg,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            pMsg->m_dwSignal,
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_CellDelComponentFsm Received Error Msg! ");
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::InitComponentContext
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::InitComponentContext(VOID)
{
    GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg = NULL_REL_COMPLETE;
    GetComponentContext()->tCIMCellDelVar.dwRRMCellDelTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellDelVar.dwCellDelTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellDelVar.dwRRMResult = CCM_CELLDEL_OK;
    GetComponentContext()->tCIMCellDelVar.dwRRUResult = CCM_CELLDEL_OK;
    GetComponentContext()->tCIMCellDelVar.dwBBResult = CCM_CELLDEL_OK;
    GetComponentContext()->tCIMCellDelVar.dwRNLUResult = CCM_CELLDEL_OK;
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRRUDelReqOrNot
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReqOrNot(VOID)
{
    WORD32 dwResult = RNLC_SUCC;
    
    // �������������RRUС��ɾ������; ֻ��TDD��ʽ������������Ϣ
    if (1 == g_tSystemDbgSwitch.ucRRUDbgSwitch &&(OPRREASON_DEL_ByTestRunMode != m_ucCellDelReason))
    {
        if (DBS_LTE_TDD == GetJobContext()->tCIMVar.ucRadioMode)
        {
            dwResult = SendToRRUDelReq_TddV20();
            if (RNLC_SUCC != dwResult)
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReq,
                                        GetJobContext()->tCIMVar.wInstanceNo,
                                        dwResult,
                                        RNLC_FATAL_LEVEL, 
                                            " FunctionCallFail_SendToRRUDelReq_TddV20! ");
                
                return ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReq;
            }
        }
        else if (DBS_LTE_FDD == GetJobContext()->tCIMVar.ucRadioMode)
        {
           /*dwResult = SendToRRUDelReq_FddV20();**/
           dwResult = SendToRRUDelReq_Fdd();
            if (RNLC_SUCC != dwResult)
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReq,
                                        GetJobContext()->tCIMVar.wInstanceNo,
                                        dwResult,
                                        RNLC_FATAL_LEVEL, 
                                            " FunctionCallFail_SendToRRUDelReq_FddV20! ");
                
                return ERR_CCM_CIM_FunctionCallFail_SendToRRUDelReq;
            }
        }
           
    }
    // ���������RRU������FDD��ʽ�����ø�RRU��С��ɾ������, ����Ҫ�Լ�������Ӧ
    else
    {
        dwResult = DbgSendToSelfRRURelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_CellDelComponentFsm FunctionCallFail_DbgSendToSelfRRMRelRsp!");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp;
        }
    }
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRNLUDelReqOrNot
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRNLUDelReqOrNot(VOID)
{
    //��������£������û���С��ɾ������
    if (1 == g_tSystemDbgSwitch.ucRNLUDbgSwitch)
    {
        WORD32 dwResult = SendToRNLUDelReq();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToRNLUDelReq! ");

            return ERR_CCM_CIM_FunctionCallFail_SendToRNLUDelReq;
        }
    }
    // ����������û��棬���ø��û��淢������Ϣ,����Ҫ������Ӧ���͸��Լ�
    else
    {
        WORD32 dwResult = DbgSendToSelfRNLURelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRNLURelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfRNLURelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRNLURelRsp;
        }
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToBBDelReqOrNot
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToBBDelReqOrNot(VOID)
{
    //��������£�����С���������������
    if (1 == g_tSystemDbgSwitch.ucBBDbgSwitch)
    {
        WORD32 dwResult = SendToBBDelReq();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToBBDelReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToBBDelReq! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToBBDelReq;
        }
        
        return RNLC_SUCC;
    }
    // ���������BB,���ø�BB�����������󣬵���Ҫ����BBС��������Ӧ�����Լ�
    else
    {
        WORD32 dwResult = DbgSendToSelfBBRelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBReCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfBBRelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBRelRsp;
        }
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRRMDelReqOrNot
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRMDelReqOrNot(BYTE byDelCause)
{
    // ��������£�����С�����������RRM
    if (1 == g_tSystemDbgSwitch.ucRRMDbgSwitch)
    {
        WORD32 dwResult = SendToRRMDelReq(byDelCause);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRMDelReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToRRMDelReq! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToRRMDelReq;
        }
    }
    // ���������RRM�����ø�RRM�����������󡣵���Ҫ����RRM������Ӧ�����Լ�
    else
    {
        WORD32 dwResult = DbgSendToSelfRRMRelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfRRMRelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp;
        }
    }
    
    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToMainComDelRsp
* ��������: ����С��������Ӧ��Ϣ��CIM������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToMainComDelRsp(VOID)
{
    T_CmdCIMCellDelRsp tCmdCIMCellDelRsp;
    memset(&tCmdCIMCellDelRsp, 0, sizeof(tCmdCIMCellDelRsp));

    if ((CCM_CELLDEL_OK == GetComponentContext()->tCIMCellDelVar.dwRRMResult) && 
        (CCM_CELLDEL_OK == GetComponentContext()->tCIMCellDelVar.dwRRUResult) && 
        (CCM_CELLDEL_OK == GetComponentContext()->tCIMCellDelVar.dwRNLUResult) && 
        (CCM_CELLDEL_OK == GetComponentContext()->tCIMCellDelVar.dwBBResult))
    {
        tCmdCIMCellDelRsp.dwResult =  CCM_CELLDEL_OK;
    }
    else
    {
        tCmdCIMCellDelRsp.dwResult = ERR_CCM_CIM_CELLDEL_FAIL; 
    }
    
    tCmdCIMCellDelRsp.dwRRMResult = GetComponentContext()->tCIMCellDelVar.dwRRMResult;
    tCmdCIMCellDelRsp.dwRRUResult = GetComponentContext()->tCIMCellDelVar.dwRRUResult;
    tCmdCIMCellDelRsp.dwRNLUResult = GetComponentContext()->tCIMCellDelVar.dwRNLUResult;
    tCmdCIMCellDelRsp.dwBBResult = GetComponentContext()->tCIMCellDelVar.dwBBResult;

    Message tCmdCIMCellDelRspMsg;
    tCmdCIMCellDelRspMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tCmdCIMCellDelRspMsg.m_dwSignal = CMD_CIM_CELL_REL_RSP;
    tCmdCIMCellDelRspMsg.m_wLen = sizeof(tCmdCIMCellDelRsp);
    tCmdCIMCellDelRspMsg.m_pSignalPara = (void*)(&tCmdCIMCellDelRsp);

    WORD32 dwResult = SendTo(ID_CCM_CIM_MainComponent, &tCmdCIMCellDelRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellBlockComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }
    
    CIMPrintSendMessage("CMD_CIM_CELL_REL_RSP", CMD_CIM_CELL_REL_RSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToPRMDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToPRMDelReq(VOID)
{
    /*nothing!*/
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRRUDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_TddV20(VOID)
{
    T_RnlcRruCellCfgReq            tRnlcRruCellCfgReq; 
    
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.wOperateType = 2; /*2 ��ʾ Del */

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;

         
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_REL_REQ, 
                                             &tRnlcRruCellCfgReq,
                                             sizeof(tRnlcRruCellCfgReq), 
                                             COMM_RELIABLE, 
                                             &tOAMPid);
    if (OSS_SUCCESS != dwOssStatus) 
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_REL_REQ", EV_RNLC_RRU_CELL_REL_REQ);

    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCelldelCommForSigTrace tRnlcRruCelldelCommForSigTrace;
    memset(&tRnlcRruCelldelCommForSigTrace,0,sizeof(T_RnlcRruCelldelCommForSigTrace));

    tRnlcRruCelldelCommForSigTrace.dwTDDPresent = 1;
    memcpy(&tRnlcRruCelldelCommForSigTrace.tRnlcRruCellRelReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_REL_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCelldelCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCelldelCommForSigTrace));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_FddV20
* ��������:
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID
* ����˵��:
**    
*    
* ��    ��: V3.1
* �޸ļ�¼:
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_FddV20(VOID)
{
    TRnlcRruCommResCfgReq tRnlcRruCommResCfgReq;

    memset(&tRnlcRruCommResCfgReq, 0x00, sizeof(tRnlcRruCommResCfgReq));

    T_DBS_GetRruInfoByCellId_REQ tDbsGetRruInfoReq;
    T_DBS_GetRruInfoByCellId_ACK tDbsGetRruInfoAck;

    memset(&tDbsGetRruInfoReq, 0x00, sizeof(tDbsGetRruInfoReq));
    memset(&tDbsGetRruInfoAck, 0x00, sizeof(tDbsGetRruInfoAck));

    tDbsGetRruInfoReq.wCallType = USF_MSG_CALL;
    tDbsGetRruInfoReq.wCellId  = GetJobContext()->tCIMVar.wCellId;
    BOOL bResult = UsfDbsAccess(EV_DBS_GetRruInfoByCellId_REQ, (VOID *)&tDbsGetRruInfoReq, (VOID *)&tDbsGetRruInfoAck);
    if ((FALSE == bResult) || (0 != tDbsGetRruInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetRruInfoByCellId, bResult,tDbsGetRruInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetRruInfoByCellId_REQ fail!\n");
        return FALSE;
    }
    
    //��ȡrru��Ϣ
    tRnlcRruCommResCfgReq.wOperateType = 2;//  2��ʾɾ��
    tRnlcRruCommResCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCommResCfgReq.ucRruNum = 1;//��ʱһ��С��ֻ֧��һ��rru
    tRnlcRruCommResCfgReq.tRRUInfo[0].byRack = tDbsGetRruInfoAck.ucRruRack;
    tRnlcRruCommResCfgReq.tRRUInfo[0].byShelf = tDbsGetRruInfoAck.ucRruShelf;
    tRnlcRruCommResCfgReq.tRRUInfo[0].bySlot = tDbsGetRruInfoAck.ucRruSlot;

    /* ����srvcp���еõ���������λͼ*/
    {
        T_DBS_GetSvrcpInfo_REQ   tDbsGetSrvcpInfoReq;
        T_DBS_GetSvrcpInfo_ACK   tDbsGetSrvcpInfoAck;

        memset(&tDbsGetSrvcpInfoReq, 0x00, sizeof(tDbsGetSrvcpInfoReq));
        memset(&tDbsGetSrvcpInfoAck, 0x00, sizeof(tDbsGetSrvcpInfoAck));

        tDbsGetSrvcpInfoReq.wCallType  = USF_MSG_CALL;
        tDbsGetSrvcpInfoReq.wCId    = GetJobContext()->tCIMVar.wCellId;
        tDbsGetSrvcpInfoReq.wBpType    = 0;/* v2������ */
        bResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tDbsGetSrvcpInfoReq, (VOID *)&tDbsGetSrvcpInfoAck);
        if ((FALSE == bResult) || (0 != tDbsGetSrvcpInfoAck.dwResult))
        {
            CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetRruInfoByCellId, bResult,tDbsGetSrvcpInfoAck.dwResult,\
                                                        RNLC_FATAL_LEVEL, "Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
            return FALSE;
        }
        //��������λͼ�õ�����ͨ����
        //Ŀǰ��Ϊfdd����֧��һ��cp��һ��rru
        for ( WORD32 dwLoop = 0; dwLoop < MAX_RFCHAN_NUM; dwLoop++)
        {
            if ( (1 << dwLoop) == (tDbsGetSrvcpInfoAck.atCpInfo[0].ucDLAntMap & (1 << dwLoop)))
            {
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].byChannelId
                    = (BYTE)dwLoop;
   #if 0
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].dwStatus
                    = 0;// 1 ʹ�� 0 ������
   #endif
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum++;
            }
        }

    }


#if 0
    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
                                             &tRnlcRruCommResCfgReq, 
                                             sizeof(tRnlcRruCommResCfgReq), 
                                             COMM_RELIABLE, 
                                             &tOAMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }
#endif
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_REL_REQ", EV_RNLC_RRU_CELL_REL_REQ);
    //�˴���Ϊ�¼�����ͬ�ṹ��ͬ��������Ҫ�����������

    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCelldelCommForSigTrace tRnlcRruCelldelCommForSigTrace;
    memset(&tRnlcRruCelldelCommForSigTrace,0,sizeof(T_RnlcRruCelldelCommForSigTrace));

    tRnlcRruCelldelCommForSigTrace.dwFDDPresent = 1;
    memcpy(&tRnlcRruCelldelCommForSigTrace.tRnlcRruCommResCfgReq,
           &tRnlcRruCommResCfgReq,
           sizeof(tRnlcRruCommResCfgReq));
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_REL_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCelldelCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCelldelCommForSigTrace));

    WORD32 dwResult = DbgSendToSelfRRURelRsp();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellDelComponentFsm FunctionCallFail_DbgSendToSelfRRMRelRsp!");
        
        return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMRelRsp;
    }
    return RNLC_SUCC;
}


/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRNLUDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRNLUDelReq(VOID)
{      
    T_RnlcRnluCellRelReq tRnlcRnluCellRelReq;
    memset(&tRnlcRnluCellRelReq, 0, sizeof(tRnlcRnluCellRelReq));

    /* С��ID */
    tRnlcRnluCellRelReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* С������������0~5 */
    tRnlcRnluCellRelReq.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    tRnlcRnluCellRelReq.wTimeStamp = 0;

    Message tRnlcRnluCellRelReqMsg;
    tRnlcRnluCellRelReqMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tRnlcRnluCellRelReqMsg.m_dwSignal = EV_RNLC_RNLU_CELL_REL_REQ;
    tRnlcRnluCellRelReqMsg.m_wLen = sizeof(tRnlcRnluCellRelReq);
    tRnlcRnluCellRelReqMsg.m_pSignalPara = static_cast<void*>(&tRnlcRnluCellRelReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_AdptRnluComponent, &tRnlcRnluCellRelReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellBlockComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("EV_RNLC_RNLU_CELL_REL_REQ", EV_RNLC_RNLU_CELL_REL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RNLU_CELL_REL_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRnluCellRelReq),
                     (const BYTE *)(&tRnlcRnluCellRelReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToBBDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToBBDelReq(VOID)
{    
    T_RnlcBbCellRelReq  tRnlcBbCellRelReq;
    memset(&tRnlcBbCellRelReq, 0, sizeof(tRnlcBbCellRelReq));

    /*BB����ͷ*/
    /* ��Ϣ���ȣ�ָ��Ϣ�ĳ��ȣ�����MsgLen����ֶα��� */
    tRnlcBbCellRelReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellRelReq);
    /* ��Ϣ���� */
    tRnlcBbCellRelReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_REL_REQ; 

    /* ��ˮ�� */
    tRnlcBbCellRelReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* С��ID */
    tRnlcBbCellRelReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
    /* С���������� */
    tRnlcBbCellRelReq.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* ʱ��� */
    tRnlcBbCellRelReq.tMsgHeader.wTimeStamp = 0;
    
    Message tRnlcBbCellRelReqMsg;
    tRnlcBbCellRelReqMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tRnlcBbCellRelReqMsg.m_dwSignal = EV_RNLC_BB_CELL_REL_REQ;
    tRnlcBbCellRelReqMsg.m_wLen = sizeof(tRnlcBbCellRelReq);
    tRnlcBbCellRelReqMsg.m_pSignalPara = static_cast<void*>(&tRnlcBbCellRelReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_AdptBBComponent, &tRnlcBbCellRelReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellBlockComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("EV_RNLC_BB_CELL_REL_REQ", EV_RNLC_BB_CELL_REL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_BB_CELL_REL_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcBbCellRelReq),
                     (const BYTE *)(&tRnlcBbCellRelReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::SendToRRMDelReq
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRMDelReq(BYTE byDelCause)
{
    T_RnlcRrmCellRelReq tRnlcRrmCellRelReq;
    memset(&tRnlcRrmCellRelReq, 0, sizeof(tRnlcRrmCellRelReq));
    
    tRnlcRrmCellRelReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRrmCellRelReq.wCellDelCause = byDelCause;
    
    PID tRRMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tRRMPid;    
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRM_CELL_REL_REQ, 
                                             &tRnlcRrmCellRelReq,
                                             sizeof(tRnlcRrmCellRelReq), 
                                             COMM_RELIABLE, 
                                             &tRRMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }

    CIMPrintSendMessage("EV_RNLC_RRM_CELL_REL_REQ", EV_RNLC_RRM_CELL_REL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRM_CELL_REL_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRrmCellRelReq),
                     (const BYTE *)(&tRnlcRrmCellRelReq));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::CIMPrintRecvMessage
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::
CIMPrintRecvMessage(const void *pSignal, WORD32 dwSignal)
{
    CCM_CIM_NULL_POINTER_CHECK(pSignal);
    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                "CIM InstanceNo: %d, CCM_CIM_CellDelComponentFsm CurrentState: %d, Receive Message %s, MessageID: %d",
                wInstanceNo, dwTag, pSignal, dwSignal);
                                 
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::CIMPrintSendMessage
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::
CIMPrintSendMessage(const void *pSignal, WORD32 dwSignal)
{
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo: %d, CCM_CIM_CellDelComponentFsm CurrentState: %d, Send Message %s, MessageID: %d ",
                wInstanceNo, dwTag, pSignal, dwSignal);
   
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::CIMPrintTranState
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::
CIMPrintTranState(const void  *pTargetState, BYTE ucTargetState)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo:%d, CCM_CIM_CellDelComponentFsm TranState To %s, StateID: %d ",
                wInstanceNo, pTargetState, ucTargetState);
    
    return RNLC_SUCC;
}


/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRURelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRURelRsp(VOID)
{
    T_RruRnlcCellRelRsp tRruRnlcCellRelRsp;
    memset(&tRruRnlcCellRelRsp, 0, sizeof(tRruRnlcCellRelRsp));
    
    /* С��ID */
    tRruRnlcCellRelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* С������������0~5 */
    tRruRnlcCellRelRsp.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    tRruRnlcCellRelRsp.dwResult = 0;

    Message tRruRnlcCellRelRspMsg;
    tRruRnlcCellRelRspMsg.m_wSourceId = ID_CCM_CIM_CellReCfgComponent;
    tRruRnlcCellRelRspMsg.m_dwSignal = EV_RRU_RNLC_CELL_REL_RSP;
    tRruRnlcCellRelRspMsg.m_wLen = sizeof(tRruRnlcCellRelRsp);
    tRruRnlcCellRelRspMsg.m_pSignalPara = static_cast<void*>(&tRruRnlcCellRelRsp);

    WORD32 dwResult = SendToSelf(&tRruRnlcCellRelRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }
    
    CIMPrintSendMessage("EV_RRU_RNLC_CELL_REL_RSP", EV_RRU_RNLC_CELL_REL_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RRU_RNLC_CELL_REL_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRruRnlcCellRelRsp),
                     (const BYTE *)(&tRruRnlcCellRelRsp));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRNLURelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRNLURelRsp(VOID)
{
    T_RnluRnlcCellRelRsp tRnluRnlcCellRelRsp;
    memset(&tRnluRnlcCellRelRsp, 0, sizeof(tRnluRnlcCellRelRsp));
    /* С��ID */
    tRnluRnlcCellRelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* С������������0~5 */
    tRnluRnlcCellRelRsp.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    tRnluRnlcCellRelRsp.wTimeStamp = 0;
    tRnluRnlcCellRelRsp.dwResult = 0;

    Message tRnluRnlcCellRelRspMsg;         
    tRnluRnlcCellRelRspMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tRnluRnlcCellRelRspMsg.m_dwSignal = EV_RNLU_RNLC_CELL_REL_RSP;
    tRnluRnlcCellRelRspMsg.m_wLen = sizeof(tRnluRnlcCellRelRsp);
    tRnluRnlcCellRelRspMsg.m_pSignalPara = static_cast<void*>(&tRnluRnlcCellRelRsp);

    WORD32 dwResult = SendToSelf(&tRnluRnlcCellRelRspMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CIMPrintSendMessage("EV_RNLU_RNLC_CELL_REL_RSP", EV_RNLU_RNLC_CELL_REL_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RNLU_RNLC_CELL_REL_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRnluRnlcCellRelRsp),
                     (const BYTE *)(&tRnluRnlcCellRelRsp));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::DbgSendToSelfBBRelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfBBRelRsp(VOID)
{
    T_BbRnlcCellRelRsp tBbRnlcCellRelRsp;
    memset(&tBbRnlcCellRelRsp, 0, sizeof(tBbRnlcCellRelRsp));

    /*BB����ͷ*/
    /* ��Ϣ���ȣ�ָ��Ϣ�ĳ��ȣ�����MsgLen����ֶα��� */
    tBbRnlcCellRelRsp.tMsgHeader.wMsgLen = sizeof(tBbRnlcCellRelRsp);
    /* ��Ϣ���� */
    tBbRnlcCellRelRsp.tMsgHeader.wMsgType = EV_BB_RNLC_CELL_REL_RSP;    

    /* ��ˮ�� */
    tBbRnlcCellRelRsp.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* С��ID */
    tBbRnlcCellRelRsp.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* С���������� */
    tBbRnlcCellRelRsp.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* ʱ��� */
    tBbRnlcCellRelRsp.tMsgHeader.wTimeStamp = 0;

    tBbRnlcCellRelRsp.dwResult = 0;
    
    Message tBbRnlcCellRelRspMsg;
    tBbRnlcCellRelRspMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tBbRnlcCellRelRspMsg.m_dwSignal = EV_BB_RNLC_CELL_REL_RSP;
    tBbRnlcCellRelRspMsg.m_wLen = sizeof(tBbRnlcCellRelRsp);
    tBbRnlcCellRelRspMsg.m_pSignalPara = static_cast<void*>(&tBbRnlcCellRelRsp);

    WORD32 dwResult = SendToSelf(&tBbRnlcCellRelRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }
    
    CIMPrintSendMessage("EV_BB_RNLC_CELL_REL_RSP", EV_BB_RNLC_CELL_REL_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_BB_RNLC_CELL_REL_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tBbRnlcCellRelRsp),
                     (const BYTE *)(&tBbRnlcCellRelRsp));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRMRelRsp
* ��������: ����RRMС��������Ӧ������������Ҫ��������RRM���е���
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRMRelRsp(VOID)
{
    T_RrmRnlcCellRelRsp tRrmRnlcCellRelRsp;
    memset(&tRrmRnlcCellRelRsp, 0, sizeof(tRrmRnlcCellRelRsp));
    /* С��ID */
    tRrmRnlcCellRelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRrmRnlcCellRelRsp.dwResult = 0;

    Message tRrmRnlcCellRelRspMsg;
    tRrmRnlcCellRelRspMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tRrmRnlcCellRelRspMsg.m_dwSignal = EV_RRM_RNLC_CELL_REL_RSP;
    tRrmRnlcCellRelRspMsg.m_wLen = sizeof(tRrmRnlcCellRelRsp);
    tRrmRnlcCellRelRspMsg.m_pSignalPara = static_cast<void*>(&tRrmRnlcCellRelRsp);

    WORD32 dwResult = SendToSelf(&tRrmRnlcCellRelRspMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CIMPrintSendMessage("EV_RRM_RNLC_CELL_RECFG_RSP", EV_RRM_RNLC_CELL_RECFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RRM_RNLC_CELL_REL_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRrmRnlcCellRelRsp),
                     (const BYTE *)(&tRrmRnlcCellRelRsp));
    
    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: SendCellDelToDcm
* ��������: ����С��ɾ����Ϣ��dcm
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendCellDelToDcm(WORD16 wCellId)
{
    T_CcmDcmCellRelInd tCcmDcmCellRelInd;
    PID tDcmMainPid;
    WORD32  dwFuncRet = RNLC_FAIL;

    memset(&tCcmDcmCellRelInd, 0x00, sizeof(tCcmDcmCellRelInd));
    memset(&tDcmMainPid, 0x00, sizeof(tDcmMainPid));

    tCcmDcmCellRelInd.wCellId = wCellId;
    tCcmDcmCellRelInd.wCellRelCause = E_CCM_DCM_CELL_REL_CAUSE_CELL_DEL;

    dwFuncRet = CCM_OSS_GetDcmMainPid(&tDcmMainPid);

    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CCM_DCM_CELL_REL_IND, 
                                             &tCcmDcmCellRelInd,
                                             sizeof(tCcmDcmCellRelInd), 
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
    
    CIMPrintSendMessage("EV_CCM_DCM_CELL_REL_IND", EV_CCM_DCM_CELL_REL_IND);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_CCM_DCM_CELL_REL_IND,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tCcmDcmCellRelInd),
                     (const BYTE *)(&tCcmDcmCellRelInd));
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: SendCellDelToBbAlg
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
* �������: 
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendCellDelToBbAlg(VOID)
{
    WORD16 wBplBoardType;
    
    wBplBoardType = GetJobContext()->tCIMVar.wBoardType;
    CCM_LOG(INFO_LEVEL,"Send To BBALG Del REQ boardType %d.\n",wBplBoardType);
    if(USF_BPLBOARDTYPE_BPL0 == wBplBoardType)
    {
         CCM_LOG(INFO_LEVEL,"Send BB Alg Del Req Board Type is USF_BPLBOARDTYPE_BPL0");
         if (1 == g_tSystemDbgSwitch.ucBBDbgSwitch)
         {
              T_RnlcBbCellRelReq  tRnlcBbCellRelReq;
              memset(&tRnlcBbCellRelReq, 0, sizeof(tRnlcBbCellRelReq));
          
              /*BB����ͷ*/
              /* ��Ϣ���ȣ�ָ��Ϣ�ĳ��ȣ�����MsgLen����ֶα��� */
              tRnlcBbCellRelReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellRelReq);
              /* ��Ϣ���� */
              tRnlcBbCellRelReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_ALGPARA_CFG_REQ; 
          
              /* ��ˮ�� */
              tRnlcBbCellRelReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
              
              /* С��ID */
              tRnlcBbCellRelReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
              /* С���������� */
              tRnlcBbCellRelReq.tMsgHeader.ucCellIdInBoard = 
                                                 GetJobContext()->tCIMVar.ucCellIdInBoard;
              /* ʱ��� */
              tRnlcBbCellRelReq.tMsgHeader.wTimeStamp = 0;


        
               Message tRnlcBbCellRelReqMsg;
               tRnlcBbCellRelReqMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
               tRnlcBbCellRelReqMsg.m_dwSignal = CMD_CIM_BBALG_DEL_REQ;
               tRnlcBbCellRelReqMsg.m_wLen = sizeof(T_RnlcBbCellRelReq);
               tRnlcBbCellRelReqMsg.m_pSignalPara = static_cast <void*>(&tRnlcBbCellRelReq);
           
               WORD32 dwResult = SendTo(ID_CCM_CIM_BBAlgParaCfgComponent, &tRnlcBbCellRelReqMsg); 
               if (SSL_OK != dwResult)
               {
                   CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                           GetJobContext()->tCIMVar.wInstanceNo,
                                           dwResult,
                                           RNLC_FATAL_LEVEL, 
                                           " CCM_CIM_CellBlockComponentFsm FunctionCallFail_SendTo! ");
                   
                   return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
               }
          }
          // ���������BB,���ø�BB�����������󣬵���Ҫ����BBС��������Ӧ�����Լ�
          else
          {
              WORD32 dwResult = DbgSendToSelfBBAlgRelRsp();
              if (RNLC_SUCC != dwResult)
              {
                  CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBReCfgRsp,
                                          GetJobContext()->tCIMVar.wInstanceNo,
                                          dwResult,
                                          RNLC_FATAL_LEVEL, 
                                          " FunctionCallFail_DbgSendToSelfBBAlgRelRsp! ");
                  
                  return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBRelRsp;
              }
          }
    }
    else
    {
        WORD32 dwResult = DbgSendToSelfBBAlgRelRsp();
        if (RNLC_SUCC != dwResult)
              {
                  CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBReCfgRsp,
                                          GetJobContext()->tCIMVar.wInstanceNo,
                                          dwResult,
                                          RNLC_FATAL_LEVEL, 
                                          " FunctionCallFail_DbgSendToSelfBBAlgRelRsp! ");
                  
                  return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBRelRsp;
              }
    }
    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: Handle_BBDelAlgRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
* �������: 
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_BBDelAlgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(pMsg->m_wLen, sizeof(T_BbRnlcCellAlgParaCfgRsp));
    CIMPrintRecvMessage("CMD_CIM_BBALG_DEL_RSP", pMsg->m_dwSignal);

    const T_BbRnlcCellAlgParaCfgRsp *ptBbAlgCellRelRsp = 
                          static_cast<T_BbRnlcCellAlgParaCfgRsp*>(pMsg->m_pSignalPara);

    CCM_CIM_CELLID_CHECK(ptBbAlgCellRelRsp->tMsgHeader.wL3CellId, 
                         GetJobContext()->tCIMVar.wCellId);

    if (0 != ptBbAlgCellRelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_BBREL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptBbAlgCellRelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm BBAlg Del Failed! ");             
    }

    
    SetCompleteSystemFlg(BBALG_REL_COMPLETE, 
                         &(GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg));
    if (GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                            (GetComponentContext())->tCIMCellDelVar.ucCompleteSystemFlg) 
                            == ALL_REL_COMPLETE)
    {
        WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()-> \
                                               tCIMCellDelVar.dwCellDelTimerId);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_CellDelComponentFsm Kill Timer Failed!");
            
            return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
        }

        dwResult = SendToMainComDelRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComDelRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComDelRsp;
        }
        
        TranStateWithTag(CCM_CIM_CellDelComponentFsm, 
                         DelOK, 
                         S_CIM_DELCOM_WAIT_DELOK);
        CIMPrintTranState("S_CIM_DELCOM_WAIT_DELOK", 
                           S_CIM_DELCOM_WAIT_DELOK);
        
    }
    
    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: DbgSendToSelfBBAlgRelRsp
* ��������: 
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
* �������: 
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfBBAlgRelRsp(VOID)
{
    T_BbRnlcCellAlgParaCfgRsp tBbAlgCellRelRsp;
    memset(&tBbAlgCellRelRsp, 0, sizeof(tBbAlgCellRelRsp));

    tBbAlgCellRelRsp.dwResult = 0;
    tBbAlgCellRelRsp.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
    
    Message tBbRnlcCellRelRspMsg;
    tBbRnlcCellRelRspMsg.m_wSourceId = ID_CCM_CIM_CellDelComponent;
    tBbRnlcCellRelRspMsg.m_dwSignal = CMD_CIM_BBALG_DEL_RSP;
    tBbRnlcCellRelRspMsg.m_wLen = sizeof(T_BbRnlcCellAlgParaCfgRsp);
    tBbRnlcCellRelRspMsg.m_pSignalPara = static_cast<void*>(&tBbAlgCellRelRsp);

    WORD32 dwResult = SendToSelf(&tBbRnlcCellRelRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellDelComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }
    
    CIMPrintSendMessage("CMD_CIM_BBALG_DEL_RSP", EV_BB_RNLC_CELL_REL_RSP);

    CCM_LOG(INFO_LEVEL,"CMD_CIM_BBALG_DEL_RSP sent to ID_CCM_CIM_CellDelComponent");
    
    return RNLC_SUCC;
}


WORD32 CCM_CIM_CellDelComponentFsm::CimPowerOnInSlave()
{
   return RNLC_SUCC;
}

VOID CCM_CIM_CellDelComponentFsm::SetComponentSlave()
{
    TranStateWithTag(CCM_CIM_CellDelComponentFsm, Handle_InSlaveState,CCM_ALLCOMPONET_SLAVE_STATE);
}

VOID CCM_CIM_CellDelComponentFsm::Handle_InSlaveState(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg)
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_SYNC_COMPONENT:
        {
            /*�����幹����״̬��������*/
         TCimComponentInfo   *ptCimComponentInfo = \
                                       (TCimComponentInfo *)pMsg->m_pSignalPara;

         ucMasterSateCpy = ptCimComponentInfo->ucState;
         CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellDelComponentFsm get State %d.\n",ucMasterSateCpy);            
         break;
        }

        case CMD_CIM_SLAVE_TO_MASTER:
     {
        CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellDelComponentFsm SLAVE_TO_MASTER.\n");
        HandleSlaveToMaster(pMsg);
        break;
     }
        
        default:
        {
            Handle_ErrorMsg(pMsg);
            break;
        }
    }

    return ;
}

WORD32 CCM_CIM_CellDelComponentFsm::HandleMasterToSlave(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellDelComponentFsm HandleMasterToSlave\n");
    SetComponentSlave();
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellDelComponentFsm::HandleSlaveToMaster(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellDelComponentFsm HandleSlaveToMaster State is %d.\n",ucMasterSateCpy);
    /*lint -save -e30 */
    switch(ucMasterSateCpy)
    {
        case S_CIM_DELCOM_IDLE:
         TranStateWithTag(CCM_CIM_CellDelComponentFsm, Idle, S_CIM_DELCOM_IDLE);
        break;
        
        default:
            TranStateWithTag(CCM_CIM_CellDelComponentFsm, Idle, S_CIM_DELCOM_IDLE);
            break;
    }
    /*lint -restore*/
    return RNLC_SUCC;
}
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_Fdd()
{
    WORD32 dwResult = 0;
    WORD16 wBoardType = 0 ;
    
    /* �������ݿ��ж�С�������Ļ�����״̬�Ƿ����� */
     dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);

     if(wBoardType == USF_BPLBOARDTYPE_BPL1)
     {
         dwResult = SendToRRUDelReq_FddBpl1V25();
     }
     else
     {
         dwResult = SendToRRUDelReq_FddBpl0V20();
     }

    return RNLC_SUCC;
}
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_FddBpl1V25()
{
    T_RnlcRruCellCfgReq            tRnlcRruCellCfgReq; 
    T_DBS_GetSuperCpInfo_REQ tDbsGetSuperCpInfoReq;
    T_DBS_GetSuperCpInfo_ACK tDbsGetSuperCpInfoAck;
    T_RnlcCpCellCfgInfo            *ptRnlcCpCellCfgInfo = NULL;
    TSuperCPInfo                     *pSuperCpInfo = NULL;
    BOOL                                  bResult = FALSE;

    memset(&tDbsGetSuperCpInfoReq, 0x00, sizeof(tDbsGetSuperCpInfoReq));
    memset(&tDbsGetSuperCpInfoAck, 0x00, sizeof(tDbsGetSuperCpInfoAck));
    memset(&tRnlcRruCellCfgReq, 0x00, sizeof(tRnlcRruCellCfgReq));
    
    tDbsGetSuperCpInfoReq.wCallType = USF_MSG_CALL;
    tDbsGetSuperCpInfoReq.wCId = GetJobContext()->tCIMVar.wCellId;

    bResult = UsfDbsAccess(EV_DBS_GetSuperCpInfo_REQ, (VOID *)&tDbsGetSuperCpInfoReq, (VOID *)&tDbsGetSuperCpInfoAck);
    if ((FALSE == bResult) || (0 != tDbsGetSuperCpInfoAck.dwResult))
    {
         /*��ӡ����*/
        return FALSE;
    }

    /***************4T8R*****************/
    if(GetJobContext()->tCIMVar.ucCpMergeType)
    {
        CcmCommonCpMerge(GetJobContext()->tCIMVar.wCellId,(VOID *)&tDbsGetSuperCpInfoAck);
    }
    /************************************/

    T_DBS_GetRruInfoByCellId_REQ tDbsGetRruInfoReq;
    T_DBS_GetRruInfoByCellId_ACK tDbsGetRruInfoAck;

    memset(&tDbsGetRruInfoReq, 0x00, sizeof(tDbsGetRruInfoReq));
    memset(&tDbsGetRruInfoAck, 0x00, sizeof(tDbsGetRruInfoAck));

    tDbsGetRruInfoReq.wCallType = USF_MSG_CALL;
    tDbsGetRruInfoReq.wCellId  = GetJobContext()->tCIMVar.wCellId;
    bResult = UsfDbsAccess(EV_DBS_GetRruInfoByCellId_REQ, (VOID *)&tDbsGetRruInfoReq, (VOID *)&tDbsGetRruInfoAck);
    if ((FALSE == bResult) || (0 != tDbsGetRruInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetSvrcpTddInfo, bResult,tDbsGetRruInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetRruInfoByCellId_REQ fail!\n");
        return FALSE;
    }

    tRnlcRruCellCfgReq.wOperateType = 2; /* 2 ��ʾ Del*/
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tDbsGetSuperCpInfoAck.ucCPNum;

    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    pSuperCpInfo = tDbsGetSuperCpInfoAck.atCpInfo;

    for(BYTE ucCpLoop = 0; ucCpLoop <tRnlcRruCellCfgReq.ucCpNum && ucCpLoop<MAX_CP_OAM_PER_CELL;ucCpLoop++ )
    {
         ptRnlcCpCellCfgInfo->byRruRack = pSuperCpInfo->ucRruRackNo;
         ptRnlcCpCellCfgInfo->byRruShelf = pSuperCpInfo->ucRruShelfNo;
         ptRnlcCpCellCfgInfo->byRruSlot = pSuperCpInfo->ucRruSlotNo;

        for ( WORD32 dwLoop = 0; dwLoop < MAX_RFCHAN_NUM; dwLoop++)
        {
            if ( (1 << dwLoop) == (pSuperCpInfo->ucDLAntMap & (1 << dwLoop)))
            {
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byChannelId
                    = (BYTE)dwLoop;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byNum = 1;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucCarNo  
                    = pSuperCpInfo->ucRRUCarrierNo;// 1 ʹ�� 0 ������
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucStatus = 0;
                ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum++;
            }
        }

        ptRnlcCpCellCfgInfo++;
        pSuperCpInfo++;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_REL_REQ, 
                                             &tRnlcRruCellCfgReq, 
                                             sizeof(tRnlcRruCellCfgReq), 
                                             COMM_RELIABLE, 
                                             &tOAMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }

        //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCelldelCommForSigTrace tRnlcRruCelldelCommForSigTrace;
    memset(&tRnlcRruCelldelCommForSigTrace,0,sizeof(T_RnlcRruCelldelCommForSigTrace));

    tRnlcRruCelldelCommForSigTrace.dwFDDPresent = 1;
    memcpy(&tRnlcRruCelldelCommForSigTrace.tRnlcRruCellRelReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                 EV_RNLC_RRU_CELL_REL_REQ,
                 GetJobContext()->tCIMVar.wCellId,
                 RNLC_INVALID_WORD,
                 RNLC_INVALID_WORD,
                 RNLC_ENB_TRACE_RNLC_SENT, 
                 sizeof(tRnlcRruCelldelCommForSigTrace),
                 (const BYTE *)(&tRnlcRruCelldelCommForSigTrace));
    

    return RNLC_SUCC;
}
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_FddBpl0V20()
{
    T_RnlcRruCellCfgReq            tRnlcRruCellCfgReq; 
    T_DBS_GetSvrcpInfo_REQ   tDbsGetSrvcpInfoReq;
    T_DBS_GetSvrcpInfo_ACK   tDbsGetSrvcpInfoAck;
    T_RnlcCpCellCfgInfo            *ptRnlcCpCellCfgInfo = NULL;
    TCPInfo                             *pCpInfo = NULL;
    BOOL                                  bResult = FALSE;

    memset(&tDbsGetSrvcpInfoReq, 0x00, sizeof(tDbsGetSrvcpInfoReq));
    memset(&tDbsGetSrvcpInfoAck, 0x00, sizeof(tDbsGetSrvcpInfoAck));
    memset(&tRnlcRruCellCfgReq,0x00,sizeof(tRnlcRruCellCfgReq));

    tDbsGetSrvcpInfoReq.wCallType  = USF_MSG_CALL;
    tDbsGetSrvcpInfoReq.wCId    = GetJobContext()->tCIMVar.wCellId;
    tDbsGetSrvcpInfoReq.wBpType    = 0;/* v2������ */
    bResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tDbsGetSrvcpInfoReq, (VOID *)&tDbsGetSrvcpInfoAck);
    if ((FALSE == bResult) || (0 != tDbsGetSrvcpInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetRruInfoByCellId, bResult,tDbsGetSrvcpInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return FALSE;
    }

    T_DBS_GetRruInfoByCellId_REQ tDbsGetRruInfoReq;
    T_DBS_GetRruInfoByCellId_ACK tDbsGetRruInfoAck;

    memset(&tDbsGetRruInfoReq, 0x00, sizeof(tDbsGetRruInfoReq));
    memset(&tDbsGetRruInfoAck, 0x00, sizeof(tDbsGetRruInfoAck));

    tDbsGetRruInfoReq.wCallType = USF_MSG_CALL;
    tDbsGetRruInfoReq.wCellId  = GetJobContext()->tCIMVar.wCellId;
    bResult = UsfDbsAccess(EV_DBS_GetRruInfoByCellId_REQ, (VOID *)&tDbsGetRruInfoReq, (VOID *)&tDbsGetRruInfoAck);
    if ((FALSE == bResult) || (0 != tDbsGetRruInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetSvrcpTddInfo, bResult,tDbsGetRruInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetRruInfoByCellId_REQ fail!\n");
        return FALSE;
    }

    tRnlcRruCellCfgReq.wOperateType = 2; /* 2 ��ʾɾ��  */
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tDbsGetSrvcpInfoAck.ucCPNum;

    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    pCpInfo = tDbsGetSrvcpInfoAck.atCpInfo;

    for(BYTE ucCpLoop = 0; ucCpLoop <tRnlcRruCellCfgReq.ucCpNum && ucCpLoop<MAX_CP_OAM_PER_CELL;ucCpLoop++ )
    {
         ptRnlcCpCellCfgInfo->byRruRack = tDbsGetRruInfoAck.ucRruRack;
         ptRnlcCpCellCfgInfo->byRruShelf = tDbsGetRruInfoAck.ucRruShelf;
         ptRnlcCpCellCfgInfo->byRruSlot = tDbsGetRruInfoAck.ucRruSlot;

        for ( WORD32 dwLoop = 0; dwLoop < MAX_RFCHAN_NUM; dwLoop++)
        {
            if ( (1 << dwLoop) == (pCpInfo->ucDLAntMap & (1 << dwLoop)))
            {
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byChannelId
                    = (BYTE)dwLoop;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byNum = 1;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucCarNo  
                    = pCpInfo->ucRRUCarrierNo;// 1 ʹ�� 0 ������
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucStatus = 0;
                ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum++;
            }
        }

        pCpInfo++;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_REL_REQ, 
                                             &tRnlcRruCellCfgReq, 
                                             sizeof(tRnlcRruCellCfgReq), 
                                             COMM_RELIABLE, 
                                             &tOAMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }

        //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCelldelCommForSigTrace tRnlcRruCelldelCommForSigTrace;
    memset(&tRnlcRruCelldelCommForSigTrace,0,sizeof(T_RnlcRruCelldelCommForSigTrace));

    tRnlcRruCelldelCommForSigTrace.dwFDDPresent = 1;
    memcpy(&tRnlcRruCelldelCommForSigTrace.tRnlcRruCellRelReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                 EV_RNLC_RRU_CELL_REL_REQ,
                 GetJobContext()->tCIMVar.wCellId,
                 RNLC_INVALID_WORD,
                 RNLC_INVALID_WORD,
                 RNLC_ENB_TRACE_RNLC_SENT, 
                 sizeof(tRnlcRruCelldelCommForSigTrace),
                 (const BYTE *)(&tRnlcRruCelldelCommForSigTrace));

    return RNLC_SUCC;
}

/*****************************************************************************/
/*                                ȫ�ֺ���                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                                �ֲ�����                                   */
/*****************************************************************************/
