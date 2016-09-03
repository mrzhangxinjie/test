/****************************************************************************
* ��Ȩ���� (C)2013 ����������ͨ
*
* �ļ�����: ccm_cim_cellsetupcomponent.cpp
* �ļ���ʶ:
* ����ժҪ: С����������ʵ���ļ�����Ҫʵ��С������������CIMģ�����ⲿ��ϵͳ�Ľ���
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
#include "pub_usf_oss.h"

#include "pub_lte_dbs.h"

#include "pub_lte_rnlc_oam_interface.h"
#include "pub_lte_rnlc_bb_interface.h"
#include "pub_lte_rnlc_rnlu_interface.h"
#include "pub_lte_rnlc_rrm_interface.h"

#include "rnlc_common.h"
#include "ccm_common.h"
#include "ccm_common_struct.h"
#include "ccm_eventdef.h"
#include "ccm_timer.h"
#include "ccm_error.h"
#include "ccm_debug.h"
#include "usf_bpl_pub.h"

#include "ccm_cim_common.h"
#include "ccm_cim_cellsetupcomponent.h"

/*****************************************************************************/
/*                                ��������                                   */
/*****************************************************************************/
/* С�����������ڲ���ʱ������ */
const WORD16 TIMER_PRMCELLCFG_DURATION  = 10000;
const WORD16 TIMER_CIMBBPARACFG_DURATION = 10000;
const WORD16 TIMER_RRUCELLCFG_DURATION = 25000;
const WORD16 TIMER_RNLUCELLCFG_DURATION = 1000000;
const WORD16 TIMER_BBCELLCFG_DURATION = 10000;
const WORD16 TIMER_RRMCELLCFG_DURATION = 10000;
const WORD16 TIMER_SIUPD_DURATION = 10000;

/* С�����������ڲ�״̬����*/
const BYTE S_CIM_SETUPCOM_IDLE = 0;
const BYTE S_CIM_SETUPCOM_WAIT_PRMCFGRSP = 1;
const BYTE S_CIM_SETUPCOM_WAIT_BBPARACFGRSP = 2;
const BYTE S_CIM_SETUPCOM_WAIT_RRUCFGRSP = 3;
const BYTE S_CIM_SETUPCOM_WAIT_RNLUCFGRSP = 4;
const BYTE S_CIM_SETUPCOM_WAIT_BBCFGRSP = 5;
const BYTE S_CIM_SETUPCOM_WAIT_RRMCFGRSP = 6;
const BYTE S_CIM_SETUPCOM_WAIT_SIUPDRSP = 7;
const BYTE S_CIM_SETUPCOM_SETUPOK = 8;
const BYTE S_CIM_SETUPCOM_SETUPERROR = 9;
const BYTE S_CIM_SETUPCOM_SETUPCANCEL = 10;
/*****************************************************************************/
/*                            �ļ��ڲ�ʹ�õĺ�                               */
/*****************************************************************************/ 


/*****************************************************************************/
/*                         �ļ��ڲ�ʹ�õ���������                            */
/*****************************************************************************/


/*****************************************************************************/
/*                                 ȫ�ֱ���                                  */
/*****************************************************************************/
/* ���ȫ�ֱ��������������ⲿ��ϵͳ�Ŀ��أ�����RRM, RNLU, RRU, BB
   Ĭ������Ϊȫ1���������ⲿϵͳ������ */
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

/*****************************************************************************/
/*                                ״̬����                                   */
/*****************************************************************************/

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Init
* ��������: ÿ����������ʱ���еĳ�ʼ������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: SSLҪ���д�ĺ�����
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::Init(void)
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

    ucOcfOperType = 0xFF;

    CIMPrintTranState("S_CIM_SETUPCOM_IDLE", S_CIM_SETUPCOM_IDLE);
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, Idle, S_CIM_SETUPCOM_IDLE);
    
    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Idle
* ��������: С��CIM���ع���Idle״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::Idle(Message *pMsg)
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
        
        case CMD_CIM_CELL_CFG_REQ:
        {
            Handle_CIMCfgReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitBBParaCfgRsp
* ��������: С��CIM���ع���WaitBBParaCfgRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitBBParaCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_BBPARA_CFG_RSP:
        {
            Handle_BBParaCfgRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_BBPARACFG_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitRRUCfgRsp
* ��������: С��CIM���ع���WaitRRUCfgRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitRRUCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_RRU_RNLC_CELL_CFG_RSP:
        {
            Handle_RRUCfgRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_RRUCELLCFG_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitRNLUCfgRsp
* ��������: С��CIM���ع���WaitRNLUCfgRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitRNLUCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_RNLU_RNLC_CELL_CFG_RSP:
        {
            Handle_RNLUCfgRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_RNLUCELLCFG_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitBBCfgRsp
* ��������: С��CIM���ع���WaitBBCfgRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitBBCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_BB_RNLC_CELL_CFG_RSP:
        {
            Handle_BBCfgRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_BBCELLCFG_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitRRMCfgRsp
* ��������: С��CIM���ع���WaitRRMCfgRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitRRMCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_RRM_RNLC_CELL_CFG_RSP:
        {
            Handle_RRMCfgRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_RRMCELLCFG_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::WaitSIUpdRsp
* ��������: С��CIM���ع���WaitSIUpdRsp״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: �ޡ�
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::WaitSIUpdRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_SYSINFO_UPDATE_RSP:
        {
            Handle_SIUpdRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CFGSIUPD_TIMEOUT:
        {
            Handle_TimeOut(pMsg);
            break;
        }
        case CMD_CIM_CELL_CFG_CANCEL_REQ:
        {
            Handle_CfgCancelReq(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::SetupOK
* ��������: С��CIM���ع���SetupOK״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: ���״̬�����ӣ���Ϊ�˱�ʾ��С��ֻ�ܽ���һ�飬�����ɹ���ù�������
*           ������������Ϣ��
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::SetupOK(Message *pMsg)
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
        case EV_CMM_CIM_OCF_CELL_PAOPR_REQ:
        {
            HandleOcfPaOper(pMsg);
            break;
        }

        case EV_CMM_CIM_OAM_CELL_PAOPR_RSP:
        {
            HandleOamPaOperResult(pMsg);
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
* ��������: CCM_CIM_CellSetupComponentFsm::SetupError
* ��������: С��CIM���ع���SetupError״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: ���״̬�����ӣ���Ϊ�˱�ʾ��С��ֻ�ܽ���һ�飬�����ɹ���ù�������
*           ������������Ϣ��
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::SetupError(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    Handle_ErrorMsg(pMsg);

    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SetupCancel
* ��������: С��CIM���ع���SetupCancel״̬������
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: ���״̬�����ӣ���Ϊ�˱�ʾ��С��ֻ�ܽ���һ�飬����û����ɱ�ȡ����
            �ù������ٽ�����������Ϣ��
**    
*    
* ��    ��: V3.1
* �޸ļ�¼: 
*    
****************************************************************************/
void CCM_CIM_CellSetupComponentFsm::SetupCancel(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    Handle_ErrorMsg(pMsg);

    return;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_CIMCfgReq
* ��������: С��������������CMD_CIM_CELL_CFG_REQ��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_CIMCfgReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellCfgReq), pMsg->m_wLen);
       
    const T_CmdCIMCellCfgReq *ptCmmCimCellCellCfgReq = 
                          static_cast<T_CmdCIMCellCfgReq*>(pMsg->m_pSignalPara);

    m_ucCellSetupReason = ptCmmCimCellCellCfgReq->ucCellOprReason;
    
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_REQ", pMsg->m_dwSignal);
    /*��ʼ��С������ʵʱ��Ϣ*/
    CimClearCellSetupInfo((T_CIMVar *)&(GetJobContext()->tCIMVar));
    
    WORD32 dwResult = SendBBParaCfgReq();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendBBParaCfgReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendBBParaCfgReq! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendBBParaCfgReq;
    }

    WORD32 dwCIMBBParaCfgTimerId = 
                       USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_BBPARACFG, 
                                                TIMER_CIMBBPARACFG_DURATION, 
                                                PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMBBParaCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                " SetRelativeTimer Failed! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwCIMBBParaCfgTimerId = 
                                                          dwCIMBBParaCfgTimerId;
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitBBParaCfgRsp, 
                     S_CIM_SETUPCOM_WAIT_BBPARACFGRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_BBPARACFGRSP", 
                       S_CIM_SETUPCOM_WAIT_BBPARACFGRSP);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_BBParaCfgRsp
* ��������: С��������������CMD_CIM_CELL_BBPARA_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_BBParaCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);    
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCellBBParaCfgRsp), pMsg->m_wLen);

    T_CmdCellBBParaCfgRsp *ptCmdCellBBParaCfgRsp = 
                       static_cast<T_CmdCellBBParaCfgRsp*>(pMsg->m_pSignalPara);

    CIMPrintRecvMessage("CMD_CIM_CELL_BBPARA_CFG_RSP", pMsg->m_dwSignal);
        
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                        tCIMCellSetupVar.dwCIMBBParaCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
 
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_BBALG,ptCmdCellBBParaCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
#ifdef VS2008
    ptCmdCellBBParaCfgRsp->dwResult = 0;
#endif
    if (0 != ptCmdCellBBParaCfgRsp->dwResult)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_BBALGPARACFG_INSETUP_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCellBBParaCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " BBAlgPara Cfg Fail! ");
        
        dwResult = SendCellCfgRsp(ptCmdCellBBParaCfgRsp->dwResult);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    ptCmdCellBBParaCfgRsp->dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");

            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }
                          
         /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
            �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
            �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
             ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                         SetupError, 
                         S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);
      
        return ERR_CCM_CIM_BBALGPARACFG_INSETUP_FAIL;
    }

    dwResult = SendToRRUCfgReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRRUCfgReqOrNot! ");

        return ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReqOrNot;
    }

    WORD32 dwRRUCellCfgTimerId = 
                         USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_RRUCELLCFG, 
                                                  TIMER_RRUCELLCFG_DURATION, 
                                                  PARAM_NULL);
    if (INVALID_TIMER_ID == dwRRUCellCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwRRUCellCfgTimerId = 
                                                            dwRRUCellCfgTimerId;
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitRRUCfgRsp, 
                     S_CIM_SETUPCOM_WAIT_RRUCFGRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_RRUCFGRSP", 
                       S_CIM_SETUPCOM_WAIT_RRUCFGRSP);
   
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_BBParaCfgRsp
* ��������: С��������������EV_RRU_RNLC_CELL_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_RRUCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_RruRnlcCellCfgRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_RRU_RNLC_CELL_CFG_RSP", pMsg->m_dwSignal);
    
    const T_RruRnlcCellCfgRsp *ptRruRnlcCellCfgRsp = 
                         static_cast<T_RruRnlcCellCfgRsp*>(pMsg->m_pSignalPara);

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RRU_RNLC_CELL_CFG_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RruRnlcCellCfgRsp),
                     (const BYTE *)(ptRruRnlcCellCfgRsp));

    CCM_CIM_CELLID_CHECK(ptRruRnlcCellCfgRsp->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);
    
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                          tCIMCellSetupVar.dwRRUCellCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
  
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_OAM,ptRruRnlcCellCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
    if (0 != ptRruRnlcCellCfgRsp->dwResult)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RRUCELLCFG_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRruRnlcCellCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                   " CCM_CIM_CellSetupComponentFsm RRU Cfg Fail! ");
        
        dwResult = SendCellCfgRsp(ERR_CCM_CIM_RRUCELLCFG_FAIL);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");

            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }
         
         /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
            �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
            �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
             ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                         SetupError, 
                         S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);
      
        return ERR_CCM_CIM_RRUCELLCFG_FAIL;
    }

    dwResult = SendToRNLUCfgReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRNLUCfgReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRNLUCfgReqOrNot! ");

        return ERR_CCM_CIM_FunctionCallFail_SendToRNLUCfgReqOrNot;
    }

    WORD32 dwRNLUCellCfgTimerId = 
                           USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_RNLUCELLCFG, 
                                                    TIMER_RNLUCELLCFG_DURATION, 
                                                    PARAM_NULL);
    if (INVALID_TIMER_ID == dwRNLUCellCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwRNLUCellCfgTimerId = 
                                                           dwRNLUCellCfgTimerId;
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitRNLUCfgRsp, 
                     S_CIM_SETUPCOM_WAIT_RNLUCFGRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_RNLUCFGRSP", 
                       S_CIM_SETUPCOM_WAIT_RNLUCFGRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_RNLUCfgRsp
* ��������: С��������������EV_RNLU_RNLC_CELL_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_RNLUCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_dwSignal);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_RnluRnlcCellCfgRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_RNLU_RNLC_CELL_CFG_RSP", pMsg->m_dwSignal);
    
    const T_RnluRnlcCellCfgRsp *ptRnluRnlcCellCfgRsp = 
                    static_cast<T_RnluRnlcCellCfgRsp*>(pMsg->m_pSignalPara);
    
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLU_RNLC_CELL_CFG_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RnluRnlcCellCfgRsp),
                     (const BYTE *)(ptRnluRnlcCellCfgRsp));
 
    CCM_CIM_CELLID_CHECK(ptRnluRnlcCellCfgRsp->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                         tCIMCellSetupVar.dwRNLUCellCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
   
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_RNLU,ptRnluRnlcCellCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
    
    if (0 != ptRnluRnlcCellCfgRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RNLUCELLCFG_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRnluRnlcCellCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm RNLU Cfg Fail! ");
     
        dwResult = SendCellCfgRsp(ERR_CCM_CIM_RNLUCELLCFG_FAIL);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }
         
         /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
            �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
            �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
             ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                         SetupError, 
                         S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);

         return ERR_CCM_CIM_RNLUCELLCFG_FAIL;
    }

    dwResult = SendCfgReqToBBOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendCfgReqToBBOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SSendCfgReqToBBOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendCfgReqToBBOrNot;
    }

    WORD32 dwBBCellCfgTimerId = 
                             USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_BBCELLCFG, 
                                                      TIMER_BBCELLCFG_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwBBCellCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwBBCellCfgTimerId = 
                                                             dwBBCellCfgTimerId;

    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitBBCfgRsp, 
                     S_CIM_SETUPCOM_WAIT_BBCFGRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_BBCFGRSP", 
                       S_CIM_SETUPCOM_WAIT_BBCFGRSP);
 
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_BBCfgRsp
* ��������: С��������������EV_BB_RNLC_CELL_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_BBCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);    
    CCM_CIM_MSGLEN_CHECK(sizeof(T_BbRnlcCellCfgRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_BB_RNLC_CELL_CFG_RSP", pMsg->m_dwSignal);

    T_BbRnlcCellCfgRsp *ptBbRnlcCellCfgRsp = 
                          static_cast<T_BbRnlcCellCfgRsp*>(pMsg->m_pSignalPara);
    
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_BB_RNLC_CELL_CFG_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_BbRnlcCellCfgRsp),
                     (const BYTE *)(ptBbRnlcCellCfgRsp));

    CCM_CIM_CELLID_CHECK(ptBbRnlcCellCfgRsp->tMsgHeader.wL3CellId, 
                         GetJobContext()->tCIMVar.wCellId);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                       tCIMCellSetupVar.dwBBCellCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_BBCFG,ptBbRnlcCellCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
#ifdef VS2008
    ptBbRnlcCellCfgRsp->dwResult = 0;
#endif
    if (0 != ptBbRnlcCellCfgRsp->dwResult)
    {
        RnlcLogTrace(PRINT_RNLC_CCM,  //����ģ��� 
                     __FILE__,        //�ļ�����
                     __LINE__,        //�ļ��к�
                     GetJobContext()->tCIMVar.wCellId,  //С����ʶ 
                     RNLC_INVALID_WORD,  //UE GID 
                     ERR_CCM_CIM_RNLUCELLCFG_FAIL,  //�쳣̽������� 
                     GetJobContext()->tCIMVar.wInstanceNo,  //�쳣̽�븨����
                     ptBbRnlcCellCfgRsp->dwResult,  //�쳣̽����չ������
                     RNLC_FATAL_LEVEL,  //��ӡ����
                     "\n CCM_CIM_CellSetupComponentFsm BB Cfg Fail! \n");

        dwResult = SendCellCfgRsp(ERR_CCM_CIM_BBCELLCFG_FAIL);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }
        
         /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
            �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
            �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
             ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                          SetupError, 
                          S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);
    
        return ERR_CCM_CIM_RNLUCELLCFG_FAIL;
    }

    SaveBBCfgRspPara(ptBbRnlcCellCfgRsp);
    
    dwResult = SendToRRMCfgReqOrNot();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRMCfgReqOrNot,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToRRMCfgReqOrNot! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToRRMCfgReqOrNot;
    }
    
    WORD32 dwRRMCellCfgTimerId = 
                             USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_RRMCELLCFG, 
                                                      TIMER_RRMCELLCFG_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwRRMCellCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwRRMCellCfgTimerId = 
                                                            dwRRMCellCfgTimerId;

    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitRRMCfgRsp, 
                     S_CIM_SETUPCOM_WAIT_RRMCFGRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_RRMCFGRSP", 
                       S_CIM_SETUPCOM_WAIT_RRMCFGRSP); 
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_RRMCfgRsp
* ��������: С��������������EV_RRM_RNLC_CELL_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_RRMCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_RrmRnlcCellCfgRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_RRM_RNLC_CELL_CFG_RSP", pMsg->m_dwSignal);
    
    const T_RrmRnlcCellCfgRsp *ptRrmRnlcCellCfgRsp = 
                         static_cast<T_RrmRnlcCellCfgRsp*>(pMsg->m_pSignalPara);
    
    CCM_CIM_CELLID_CHECK(ptRrmRnlcCellCfgRsp->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);
 
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                          tCIMCellSetupVar.dwRRMCellCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RRM_RNLC_CELL_CFG_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_RECE, 
                     sizeof(T_RrmRnlcCellCfgRsp),
                     (const BYTE *)(ptRrmRnlcCellCfgRsp));
  
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_RRM,ptRrmRnlcCellCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
    if (0 != ptRrmRnlcCellCfgRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_RRMCELLCFG_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptRrmRnlcCellCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm RRM Cfg Fail! ");

        dwResult = SendCellCfgRsp(ERR_CCM_CIM_RRMCELLCFG_FAIL);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }
             
        /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
        �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
        �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
         ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                         SetupError, 
                         S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);

        return ERR_CCM_CIM_RRMCELLCFG_FAIL;
    }

    dwResult = SendSIUpdReq();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendSIUpdReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendSIUpdReq! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendSIUpdReq;
    }
    
    WORD32 dwCIMSIBBroadcastTimerId = 
                          USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CFGSIUPD, 
                                                   TIMER_SIUPD_DURATION, 
                                                   PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMSIBBroadcastTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SetRelativeTimer;
    }
    GetComponentContext()->tCIMCellSetupVar.dwCIMSIBBroadcastTimerId = 
                                                       dwCIMSIBBroadcastTimerId;
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     WaitSIUpdRsp, 
                     S_CIM_SETUPCOM_WAIT_SIUPDRSP);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SYSINFORSP", 
                       S_CIM_SETUPCOM_WAIT_SIUPDRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_SIUpdRsp
* ��������: С��������������EV_RRM_RNLC_CELL_CFG_RSP��Ϣ�ĺ���
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_SIUpdRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);    
    CCM_CIM_MSGLEN_CHECK(sizeof(T_SysInfoUpdRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("CMD_CIM_CELL_SYSINFO_UPDATE_RSP", pMsg->m_dwSignal);
      
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()-> \
                                     tCIMCellSetupVar.dwCIMSIBBroadcastTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_CellSetupComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }

    T_SysInfoUpdRsp *ptSysInfoUpdRsp = 
                             static_cast<T_SysInfoUpdRsp*>(pMsg->m_pSignalPara);
#ifdef VS2008
    ptSysInfoUpdRsp->dwResult = 0;
#endif
   
    /*����С������״̬����*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_SIUPD,ptSysInfoUpdRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
    if (0 != ptSysInfoUpdRsp->dwResult)
    {          
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_SIUPD_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptSysInfoUpdRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm SI Update Fail! ");

        dwResult = SendCellCfgRsp(ERR_CCM_CIM_SIUPD_FAIL);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToMainComCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
        }

        /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPERROR
        �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
        �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
         ����ͬ*/
        TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                         SetupError, 
                         S_CIM_SETUPCOM_SETUPERROR);
        CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                           S_CIM_SETUPCOM_SETUPERROR);
      
        return ERR_CCM_CIM_SIUPD_FAIL;
    }

    dwResult = SendCellCfgRsp(ptSysInfoUpdRsp->dwResult);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToMainComCfgRsp! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
    }

    /* ����ط������ٹ������ǽ���״̬ת��ΪS_CIMCELLSETUP_SETUPOK
    �ҵĳ���������ΪС������������������һ�Σ�������гɹ��ˣ�ok.
    �������ʧ���ˣ�ҲҪ�ȴ�С��ɾ�����������������̺��������´�
     ����ͬ*/
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     SetupOK, 
                     S_CIM_SETUPCOM_SETUPOK);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPOK", 
                       S_CIM_SETUPCOM_SETUPOK);

 
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_CfgCancelReq
* ��������: С��������������CMD_CIM_CELL_CFG_CANCEL_REQ��Ϣ�ĺ���
* �㷨����: ��
* ȫ�ֱ���: ��
* Input����: VOID
* �������: ��
* �� �� ֵ: VOID 
* ����˵��: 
**    
* �������: 2011��11��
* ��    ��: V3
* �޸ļ�¼: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_CfgCancelReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_CANCEL_REQ", pMsg->m_dwSignal);

    WORD32 dwResult = SendCfgCancelRspToCcmMain();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendCfgCancelRspToCcmMain,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendCfgCancelRspToCcmMain! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendCfgCancelRspToCcmMain;
    }
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     SetupCancel,
                     S_CIM_SETUPCOM_SETUPCANCEL);
    CIMPrintTranState("S_CIM_SETUPCOM_SETUPCANCEL", 
                       S_CIM_SETUPCOM_SETUPCANCEL);
    
    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_TimeOut
* ��������: С���������������ⲿϵͳ��Ӧ��ʱ�ĺ�������Ϊÿ��ϵͳ��ʱ�Ĵ���ʽһ����
*           ���Խ���һ������ʱ�ĺ�����
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_TimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLSETUP_TIMEOUT,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            GetTag(),
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_CellSetupComponentFsm CellSetup Timeout! ");

    CIMPrintRecvMessage("TimeOut Msg", pMsg->m_dwSignal);

    WORD32 dwResult = SendCellCfgRsp(pMsg->m_dwSignal);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToMainComCfgRsp! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToMainComCfgRsp;
    }
    
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     SetupError, 
                     S_CIM_SETUPCOM_SETUPERROR);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPERROR", 
                       S_CIM_SETUPCOM_SETUPERROR); 

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::Handle_ErrorMsg
* ��������: С����ĳһ״̬���ܵ��������Ϣ�����ڵĴ���ʽ�ǽ��ϱ��쳣��
*           ������Ĵ���
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
WORD32 CCM_CIM_CellSetupComponentFsm ::Handle_ErrorMsg(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CIMPrintRecvMessage("ErrorMsg", pMsg->m_dwSignal);
    
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_ErrorMsg,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            pMsg->m_dwSignal,
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_CellSetupComponentFsm Received Error Msg! ");
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::InitComponentContext
* ��������: ��ʼ��С������������������
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
WORD32 CCM_CIM_CellSetupComponentFsm::InitComponentContext(VOID)
{
    GetComponentContext()->tCIMCellSetupVar.dwPRMCellCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwCIMBBParaCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwRRUCellCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwRNLUCellCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwBBCellCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwRRMCellCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMCellSetupVar.dwCIMSIBBroadcastTimerId = INVALID_TIMER_ID;
     
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot
* ��������: �ж��Ƿ���С������������Ϣ��RRU����-OAM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot(VOID)
{
    WORD32  dwResult = RNLC_FAIL;
    // �������������RRUС����������; ֻ��TDD��ʽ������������Ϣ
    if (1 == g_tSystemDbgSwitch.ucRRUDbgSwitch) 
    {
        if (DBS_LTE_TDD == GetJobContext()->tCIMVar.ucRadioMode)
        {
            /*dwResult = SendToRRUCfgReq_TddV20();*/
            dwResult = SendToRRUCfgReq_Tdd();
            if (RNLC_SUCC != dwResult)
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReq,
                                        GetJobContext()->tCIMVar.wInstanceNo,
                                        dwResult,
                                        RNLC_FATAL_LEVEL, 
                                        " FunctionCallFail_SendToRRUCfgReq_TddV20! ");

                return ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReq;
            }
            
        }
        else if ( DBS_LTE_FDD == GetJobContext()->tCIMVar.ucRadioMode )
        {
            /*dwResult = SendToRRUCfgReq_FddV20();*/
            dwResult = SendToRRUCfgReq_Fdd();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                        " FunctionCallFail_SendToRRUCfgReq_FddV20! ");

            return ERR_CCM_CIM_FunctionCallFail_SendToRRUCfgReq;
        }
        }
        
    }
    else
    {
        dwResult = DbgSendToSelfRRUCfgRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRUCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfRRUCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRUCfgRsp;
        }
    }
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReqOrNot
* ��������: �����Ƿ���С������������Ϣ��RNLU
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReqOrNot(VOID)
{
    //��������£������û���С����������
    if (1 == g_tSystemDbgSwitch.ucRNLUDbgSwitch)
    {
        WORD32 dwResult = SendToRNLUCfgReq();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRNLUCfgReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToRNLUCfgReq! ");

            return ERR_CCM_CIM_FunctionCallFail_SendToRNLUCfgReq;
        }
    }
    else
    {
        WORD32 dwResult = DbgSendToSelfRNLUCfgRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRNLUCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfRNLUCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRNLUCfgRsp;
        }
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendCfgReqToBBOrNot
* ��������: �����Ƿ���С������������Ϣ��BB
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCfgReqToBBOrNot(VOID)
{
    //��������£�����С���������������
    if (1 == g_tSystemDbgSwitch.ucBBDbgSwitch)
    {
        WORD32 dwResult = SendCfgReqToBB();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendCfgReqToBB,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendCfgReqToBB! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendCfgReqToBB;
        }
        
        return RNLC_SUCC;
    }
    // ���������BB,���ø�BB�����������󣬵���Ҫ����BBС��������Ӧ�����Լ�
    else
    {
        WORD32 dwResult = DbgSendToSelfBBCfgRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfBBCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfBBCfgRsp;
        }
    }

    return RNLC_SUCC;
}
/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReqOrNot
* ��������: �����Ƿ���С������������Ϣ��RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReqOrNot(VOID)
{
    // ��������£�����С�����������RRM
    if (1 == g_tSystemDbgSwitch.ucRRMDbgSwitch)
    {
        WORD32 dwResult = SendToRRMCfgReq();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToRRMCfgReq,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_SendToRRMCfgReq! ");
            
            return ERR_CCM_CIM_FunctionCallFail_SendToRRMCfgReq;
        }
    }
    // ���������RRM�����ø�RRM�����������󡣵���Ҫ����RRM������Ӧ�����Լ�
    else
    {
        WORD32 dwResult = DbgSendToSelfRRMCfgRsp();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMCfgRsp,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_DbgSendToSelfRRMCfgRsp! ");
            
            return ERR_CCM_CIM_FunctionCallFail_DbgSendToSelfRRMCfgRsp;
        }
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToPRMCfgReq
* ��������: ����С������������Ϣ��PRMģ�飬���ڲ�ʵ�֡�
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToPRMCfgReq(VOID)
{
    /*nothing!*/
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendBBParaCfgReq
* ��������: ����С������������Ϣ�������㷨�������ù�����
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendBBParaCfgReq(VOID)
{    
    T_CmdCellBBParaCfgReq tCmdCellBBParaCfgReq;
    memset(&tCmdCellBBParaCfgReq, 0, sizeof(tCmdCellBBParaCfgReq));

    Message tBBParaCfgReqMsg;
    tBBParaCfgReqMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tBBParaCfgReqMsg.m_dwSignal = CMD_CIM_CELL_BBPARA_CFG_REQ;
    tBBParaCfgReqMsg.m_wLen = sizeof(tCmdCellBBParaCfgReq);
    tBBParaCfgReqMsg.m_pSignalPara = static_cast<void*>(&tCmdCellBBParaCfgReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_BBAlgParaCfgComponent, &tBBParaCfgReqMsg);
    if (SSL_OK != dwResult)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("CMD_CIM_CELL_BBPARA_CFG_REQ", CMD_CIM_CELL_BBPARA_CFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_BBPARA_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCellBBParaCfgReq),
                     (const BYTE *)(&tCmdCellBBParaCfgReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot
* ��������: �ж��Ƿ���С������������Ϣ��RRU����-OAM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_TddV20(VOID)
{    
    T_RnlcRruCellCfgReq tRnlcRruCellCfgReq;
    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));   

    memset(&tRnlcRruCellCfgReq, 0, sizeof(tRnlcRruCellCfgReq));
#if 0
    /* ��ȡ���ݿ���Ϣ*/
    tGetSvrcpTddInfoReq.wCallType = USF_MSG_CALL;
    tGetSvrcpTddInfoReq.wCId   = GetJobContext()->tCIMVar.wCellId;
    BYTE ucResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tGetSvrcpTddInfoReq, (VOID *)&tGetSvrcpTddInfoAck);
    if ((FALSE == ucResult) || (0 != tGetSvrcpTddInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetSvrcpTddInfo, ucResult,tGetSvrcpTddInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return FALSE;
    }

    tRnlcRruCellCfgReq.ucRruNum = tGetSvrcpTddInfoAck.ucCPNum;  /*    */
    for ( BYTE ucCpLoop=0;ucCpLoop<tGetSvrcpTddInfoAck.ucCPNum && ucCpLoop < MAX_RPO_RRU_NUM_PER_CELL;ucCpLoop++)
    {
        T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
        memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
        tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
        tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
        
        T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
        memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
        
        BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                         static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                         static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
        if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
        {        
            const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                    tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                    bDbResult,
                                    RNLC_FATAL_LEVEL, 
                                    " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                    wInstanceNo);
            
            return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
        }

        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].wCellId =  GetJobContext()->tCIMVar.wCellId;

        /*�����ӿڣ��ز�ѹ������*/
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;
        /* С������������0~5 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
         /* 0:FDD��1:TDD */ 
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucDuplexMode =  GetJobContext()->tCIMVar.ucRadioMode;     
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].wCellPower = 
                                                            tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellTransPwr;
        /* С�������� */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucAntGroup = tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucAntSetNo;
        /* С���ز���/Ƶ������Ŀǰ��֧�ֵ��ز���Ŀǰ�̶�Ϊ1 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucCarrNum = 1;
        /* �ز��� */  /*OAM����Ҫ����ʱ���޸ģ������׮ */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].ucCarrNo = 1;
        /* ������ƵEARFCN ����rru��Ƶ����Ҫת��֮ǰ��ֵ*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*��ȡƵ��ָʾ*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* �ز�����ö��ֵ��{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* ��������֡���ã�ö��ֵ��{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* ������֡���ã�ö��ֵ��{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* ���õ���Чϵͳ��֡�ţ�Ϊ10ms֡��0~1023 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwCfgValidSFN = 0;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);


    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));

    tRnlcRruCellCfgCommForSigTrace.dwTDDPresent = 1;
    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCellCfgReq,
           &tRnlcRruCellCfgReq,
           sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
#endif
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: SendToRRUCfgReq_FddV20
* ��������: ����С��������Ϣ��-OAM----FDD
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_FddV20(VOID)
{
    TRnlcRruCommResCfgReq tRnlcRruCommResCfgReq;


    memset(&tRnlcRruCommResCfgReq, 0x00, sizeof(tRnlcRruCommResCfgReq));
#if 0
    /* ����rru��Ϣ���ܿ�� */
    {
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
        
        //��ȡrru��Ϣ
        tRnlcRruCommResCfgReq.wOperateType = 0;//0 ��ʾ����
        tRnlcRruCommResCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
        tRnlcRruCommResCfgReq.ucRruNum = 1;//��ʱһ��С��ֻ֧��һ��rru
        tRnlcRruCommResCfgReq.tRRUInfo[0].byRack = tDbsGetRruInfoAck.ucRruRack;
        tRnlcRruCommResCfgReq.tRRUInfo[0].byShelf = tDbsGetRruInfoAck.ucRruShelf;
        tRnlcRruCommResCfgReq.tRRUInfo[0].bySlot = tDbsGetRruInfoAck.ucRruSlot;
    }

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
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].dwStatus
                    = 1;// 1 ʹ�� 0 ������
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum++;
            }
        }

    }
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);
    //�˴���Ϊ�¼�����ͬ�ṹ��ͬ��������Ҫ�����������


    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));

    tRnlcRruCellCfgCommForSigTrace.dwFDDPresent = 1;
    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCommResCfgReq,
           &tRnlcRruCommResCfgReq,
           sizeof(tRnlcRruCommResCfgReq));
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
#endif    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReq
* ��������: ����С������������Ϣ��RNLU
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReq(VOID)
{    
    T_RnlcRnluCellCfgReq tRnlcRnluCellCfgReq;
    memset(&tRnlcRnluCellCfgReq, 0, sizeof(tRnlcRnluCellCfgReq));

    tRnlcRnluCellCfgReq.m.dwAddOrRecfgBcchInfoPresent = 1;
    tRnlcRnluCellCfgReq.m.dwAddOrRecfgPcchInfoPresent = 1;
    tRnlcRnluCellCfgReq.m.dwAddOrRecfgUlCcchInfoPresent = 1;
    tRnlcRnluCellCfgReq.m.dwAddOrRecfgDlCcchInfoPresent = 1;

    tRnlcRnluCellCfgReq.m.dwDelBcchGidPresent = 0;
    tRnlcRnluCellCfgReq.m.dwDelPcchGidPresent = 0;
    tRnlcRnluCellCfgReq.m.dwDelUlCcchGidPresent = 0;
    tRnlcRnluCellCfgReq.m.dwDelDlCcchGidPresent = 0;

    /*    */
    tRnlcRnluCellCfgReq.ucCmacSUnit = GetJobContext()->tCIMVar.tCIMPIDinfo.tBBPid.ucSUnit;
  
    /* С��ID */
    tRnlcRnluCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;   
    /* С������������0~5 */
    tRnlcRnluCellCfgReq.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* ��������֡���ã�ö��ֵ��{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
    tRnlcRnluCellCfgReq.ucUlDlSlotAlloc = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlDlSlotAlloc;
    tRnlcRnluCellCfgReq.wTimeStamp = 0;

    /* ����ʹ����v2.5�Ľӿڣ�����ֱ�Ӵ����ݿ��ȡplmn��Ϣ */
    {
        T_DBS_GetCellInfoByCellId_REQ tGetCellCfgInfoReq;
        T_DBS_GetCellInfoByCellId_ACK tGetCellCfgInfoAck;
        T_PlmnID tPlmnId;
        BOOL bResult = FALSE;
        

        memset(&tGetCellCfgInfoReq, 0x00, sizeof(tGetCellCfgInfoReq));
        memset(&tGetCellCfgInfoAck, 0x00, sizeof(tGetCellCfgInfoAck));
        memset(&tPlmnId, 0x00, sizeof(tPlmnId));

        tGetCellCfgInfoReq.wCallType = USF_MSG_CALL;
        tGetCellCfgInfoReq.wCellId = tRnlcRnluCellCfgReq.wCellId;
        bResult = UsfDbsAccess(EV_DBS_GetCellInfoByCellId_REQ, (VOID*)(&tGetCellCfgInfoReq), (VOID*)(&tGetCellCfgInfoAck));
        if (!bResult || DBS_SUCCESS != tGetCellCfgInfoAck.dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                                tGetCellCfgInfoAck.dwResult, 
                                bResult,
                                RNLC_ERROR_LEVEL, 
                                "UsfDbsAccess EV_DBS_GetCellCfgInfo4FDD_REQ failed.");
            tRnlcRnluCellCfgReq.m.dwAddOrRecfgPlmnInfoPresent = 0;//�����ȡʧ�ܣ�����plmn��Ӱ��kpi
        }
        else
        {
            tRnlcRnluCellCfgReq.m.dwAddOrRecfgPlmnInfoPresent = 1;
            tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.ucAddOrRecfgPlmnNum =
                tGetCellCfgInfoAck.tCellAttr.tCellInfo.ucPlmnNum;
            for( WORD32 dwMccMncLoop = 0; 
                 (dwMccMncLoop < tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.ucAddOrRecfgPlmnNum) 
                 &&(dwMccMncLoop < MAX_NUM_PLMN_PRE_CELL); 
                 dwMccMncLoop++ )
            {
            #if 0
                CcmCimConvertArrayToMccMnc(&tGetCellCfgInfoAck.tCellAttr.tCellInfo.aucMCC[dwMccMncLoop*3], 
                    &tGetCellCfgInfoAck.tCellAttr.tCellInfo.aucMNC[dwMccMncLoop*3], 
                    tPlmnId);
                RnlcGetMccMncByPlmn(tPlmnId, &wMcc,&wMnc);
                tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.atAddOrRecfgMccMncInfo[dwMccMncLoop].dwMcc = wMcc;
                tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.atAddOrRecfgMccMncInfo[dwMccMncLoop].dwMnc = wMnc;
            #endif

                WORD32 dwMcc = tGetCellCfgInfoAck.tCellAttr.tCellInfo.awMCC[dwMccMncLoop];
                WORD32 dwMnc = tGetCellCfgInfoAck.tCellAttr.tCellInfo.awMNC[dwMccMncLoop];
                RnlcKpiMccMncTrans(&dwMcc,&dwMnc);

                tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.atAddOrRecfgMccMncInfo[dwMccMncLoop].dwMcc = dwMcc;
                tRnlcRnluCellCfgReq.tAddOrRecfgPlmnInfo.atAddOrRecfgMccMncInfo[dwMccMncLoop].dwMnc = dwMnc;
            }
            
        }
        
    }

    /* BCCH������Ϣ */
    tRnlcRnluCellCfgReq.tAddOrRecfgBcchInfo.wGid = 
                               GetJobContext()->tCIMVar.tChannelGidInfo.wBcchId;   

    /* PCCH������Ϣ */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.wGid = 
                               GetJobContext()->tCIMVar.tChannelGidInfo.wPcchId;   
    /* ����������ݿ��ж�ȡѰ���ط����� */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.wPagingRepeatTime = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucPagRepeatTime;   
    /* ����������ݿ��ж�ȡѰ���ط����� */
    /* ȡֵ���´�0��ʼ������ fourT, twoT, oneT, halfT, quarterT, oneEightT, 
                                               onSixteenthT, oneThirtySecondT */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.dwDrxLength = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucPagDrxCyc;    
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.dwNb = 
                 GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucnB;

    /* ULCCCH������Ϣ */
    tRnlcRnluCellCfgReq.tAddOrRecfgUlCcchInfo.wGid = 
                             GetJobContext()->tCIMVar.tChannelGidInfo.wUlCcchId;
    /* DLCCCH������Ϣ */
    tRnlcRnluCellCfgReq.tAddOrRecfgDlCcchInfo.wGid = 
                             GetJobContext()->tCIMVar.tChannelGidInfo.wDlCcchId;

    /*����dwSwchUserInac�ֶδ����ݿ��л�ȡ*/
    /* ENodeB�������ö�ȡ */
    T_DBS_GetENodeBGlobalPara_REQ tGetENodeBGlobalParaReq;
    memset(&tGetENodeBGlobalParaReq, 0, sizeof(tGetENodeBGlobalParaReq));
    tGetENodeBGlobalParaReq.wCallType = USF_MSG_CALL;
    
    T_DBS_GetENodeBGlobalPara_ACK tGetENodeBGlobalParaAck;
    memset(&tGetENodeBGlobalParaAck, 0, sizeof(tGetENodeBGlobalParaAck));

    BOOLEAN bResult = UsfDbsAccess(EV_DBS_GetENodeBGlobalPara_REQ, 
                                   (VOID*)(&tGetENodeBGlobalParaReq), 
                                   (VOID*)(&tGetENodeBGlobalParaAck));

    if (!bResult || DBS_SUCCESS != tGetENodeBGlobalParaAck.dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetENodeBGlobalParaReq,
                                bResult,
                                tGetENodeBGlobalParaAck.dwResult,
                                ERROR_LEVEL, 
                                "UsfDbsAccess EV_DBS_GetENodeBGlobalPara_REQ failed.");

        tRnlcRnluCellCfgReq.ucSwitchUserInac = 1; /* �Ѹ�����ȷ�ϣ���ȡʧ��ֱ�Ӹ�ֵΪ1 */
    }
    else
    {
        tRnlcRnluCellCfgReq.ucSwitchUserInac = tGetENodeBGlobalParaAck.ucSwchUserInac;
    }
    tRnlcRnluCellCfgReq.dwEnbId = tGetENodeBGlobalParaAck.dwENodeBId;
    tRnlcRnluCellCfgReq.ucUlPingOptimizeType = tGetENodeBGlobalParaAck.ucUlPingOptimizeType;
    tRnlcRnluCellCfgReq.dwULDLFtpSwitch = tGetENodeBGlobalParaAck.ucULDLFtpSwitch;

    /*    */
    T_DBS_GetGlobalT3N3PathInfo_REQ tGetGlobalT3N3PathInfoReq;
    T_DBS_GetGlobalT3N3PathInfo_ACK tGetGlobalT3N3PathInfoAck;
    
    memset((VOID*)&tGetGlobalT3N3PathInfoReq, 0, sizeof(T_DBS_GetGlobalT3N3PathInfo_REQ));
    memset((VOID*)&tGetGlobalT3N3PathInfoAck, 0, sizeof(T_DBS_GetGlobalT3N3PathInfo_ACK));
    
    /* ��ȡ��Ϣ */
    tGetGlobalT3N3PathInfoReq.wCallType = USF_MSG_CALL;
    
    bResult = UsfDbsAccess(EV_DBS_GetGlobalT3N3PathInfo_REQ, 
                        (VOID *)&tGetGlobalT3N3PathInfoReq, 
                        (VOID *)&tGetGlobalT3N3PathInfoAck);
    if (!bResult || DBS_SUCCESS != tGetGlobalT3N3PathInfoAck.dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetENodeBGlobalParaReq,
                                bResult,
                                tGetGlobalT3N3PathInfoAck.dwResult,
                                ERROR_LEVEL, 
                                "UsfDbsAccess EV_DBS_GetENodeBGlobalPara_REQ failed.");

        tRnlcRnluCellCfgReq.ucTcpOrderEnable = 0;   /*0��ʾ���عر�*/
        tRnlcRnluCellCfgReq.ucTcpOrderTimerLen = 2;   /*Ĭ�ϳ�ʼֵΪ2*/
    }
    else
    {
        tRnlcRnluCellCfgReq.ucTcpOrderEnable = tGetGlobalT3N3PathInfoAck.ucTcpOrderEnable;
        tRnlcRnluCellCfgReq.ucTcpOrderTimerLen = tGetGlobalT3N3PathInfoAck.ucTcpOrderTimerLen;
    }
    /*    */

    /* 0618 ECN begin */
    T_DBS_GetECNPara_REQ tGetECNParaReq;
    memset(&tGetECNParaReq, 0, sizeof(tGetECNParaReq));
    tGetECNParaReq.wCallType = USF_MSG_CALL;

    T_DBS_GetECNPara_ACK tGetECNParaAck;
    memset(&tGetECNParaAck, 0, sizeof(tGetECNParaAck));

    bResult = UsfDbsAccess(EV_DBS_GetECNPara_REQ, 
                           (VOID*)(&tGetECNParaReq), 
                           (VOID*)(&tGetECNParaAck));
                           
    if (!bResult || DBS_SUCCESS != tGetECNParaAck.dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetECNParaReq,
                                bResult,
                                tGetECNParaAck.dwResult,
                                ERROR_LEVEL, 
                                "UsfDbsAccess EV_DBS_GetECNPara_REQ failed.");

        tRnlcRnluCellCfgReq.m.dwECNCfgInfoPresent = 0; 
    }
    else
    {
        tRnlcRnluCellCfgReq.m.dwECNCfgInfoPresent = 1;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucEcnEnable         = tGetECNParaAck.ucEcnEnable;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucEcnPeriod         = tGetECNParaAck.ucEcnPeriod;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucDlQueThreshold    = tGetECNParaAck.ucDlQueThreshold;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucDlRetQueThreshold = tGetECNParaAck.ucDlRetQueThreshold;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucUlMemThreshold    = tGetECNParaAck.ucUlMemThreshold;
        tRnlcRnluCellCfgReq.tECNCfgInfo.ucUlRecQueThreshold = tGetECNParaAck.ucUlRecQueThreshold;
    }
    /* 0618 ECN end */

    Message tRnlcRnluCellCfgReqMsg;
    tRnlcRnluCellCfgReqMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tRnlcRnluCellCfgReqMsg.m_dwSignal = EV_RNLC_RNLU_CELL_CFG_REQ;
    tRnlcRnluCellCfgReqMsg.m_wLen = sizeof(tRnlcRnluCellCfgReq);
    tRnlcRnluCellCfgReqMsg.m_pSignalPara = static_cast<void*>(&tRnlcRnluCellCfgReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_AdptRnluComponent, &tRnlcRnluCellCfgReqMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("EV_RNLC_RNLU_CELL_CFG_REQ", EV_RNLC_RNLU_CELL_CFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RNLU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRnluCellCfgReq),
                     (const BYTE *)(&tRnlcRnluCellCfgReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendCfgReqToBB
* ��������: ����С������������Ϣ��BB
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCfgReqToBB(VOID)
{     
    T_RnlcBbCellCfgReq tRnlcBbCellCfgReq;
    memset(&tRnlcBbCellCfgReq, 0, sizeof(tRnlcBbCellCfgReq));

    /*****************************BB����ͷ*************************************/
    /* ��Ϣ���ȣ�ָ��Ϣ�ĳ��ȣ�����MsgLen����ֶα��� */
    tRnlcBbCellCfgReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellCfgReq);
    /* ��Ϣ���� */
    tRnlcBbCellCfgReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_CFG_REQ;  
    /* ��ˮ�� */
    tRnlcBbCellCfgReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    /* С��ID */
    tRnlcBbCellCfgReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* С���������� */
    tRnlcBbCellCfgReq.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* ʱ��� */
    tRnlcBbCellCfgReq.tMsgHeader.wTimeStamp = 0;

    /****************************BB��������************************************/
    /* ����С��ID */
    tRnlcBbCellCfgReq.wPhyCellId = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.wPhyCellId;

    /* System Frequency Info */
    tRnlcBbCellCfgReq.dwSysFreqInfoPresent = 1;
    tRnlcBbCellCfgReq.tSysFreqInfo.wUlCarrierFreq = 
                              (WORD16)(GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.dwUlCenterFreq);
    tRnlcBbCellCfgReq.tSysFreqInfo.wDlCarrierFreq =          
                              (WORD16)(GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.dwDlCenterFreq);

    /* Cell Info */
    tRnlcBbCellCfgReq.dwCellInfoPresent = 1;
    /* CP�ĳ��� */
    tRnlcBbCellCfgReq.tCellInfo.ucCPType = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucPhyChCPSel;

    /*****************4T8R Begin*******************/
    if(GetJobContext()->tCIMVar.ucCpMergeType)
    {
        tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = GetJobContext()->tCIMVar.ucPhyUlAntNum;
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntPortNum = GetJobContext()->tCIMVar.ucPhyDlAntNum;
    }
    else
    {
#ifndef VS2008
        /*������������*/
        /*tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 
                                  GetJobContext()->tCIMVar.tRRUInfo.ucUlAntCapacity;*/ 
        tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 1;
        /* �������߶˿��� */
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntPortNum = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucCellRSPortNum;
#else
        tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 1;  
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntPortNum = 1;
#endif

      }
    /*****************4T8R End*******************/


    /* ���д���TDD����һ�� */
    tRnlcBbCellCfgReq.tCellInfo.ucUlBandwidth = 
                         GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlSysBandWidth;
    /* ���д���TDD����һ�� */
    tRnlcBbCellCfgReq.tCellInfo.ucDlBandwidth = 
                          GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucDlSysBandWidth;


    /* 0:FDD��1:TDD */
#ifdef VS2008
    GetJobContext()->tCIMVar.ucRadioMode = FDD_RADIOMODE;
#endif
    if (TDD_RADIOMODE == GetJobContext()->tCIMVar.ucRadioMode)
    {
        tRnlcBbCellCfgReq.tCellInfo.ucDuplexMode = 1;
    }
    else if (FDD_RADIOMODE ==GetJobContext()->tCIMVar.ucRadioMode)
    {
        tRnlcBbCellCfgReq.tCellInfo.ucDuplexMode = 0;
    }
    else
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_InValidPara, 
                                GetJobContext()->tCIMVar.ucRadioMode,
                                0,
                                RNLC_FATAL_LEVEL, 
                                " ERR_CCM_CIM_InValidPara! ");

        return ERR_CCM_CIM_InValidPara;
    }
    
    /* ������ʱ϶���� */
    tRnlcBbCellCfgReq.tCellInfo.ucUlDlSwitchCfg =         
                               GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlDlSlotAlloc; 
    /* ������֡���ã�ö��ֵ��{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,ssp5,ssp6,ssp7,ssp8} */
    tRnlcBbCellCfgReq.tCellInfo.ucSpecialSubrameCfg =     
                      GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucSpecSubFramePat;     
    /* С���뾶 ��λ��10m  */
    tRnlcBbCellCfgReq.tCellInfo.wCellRadius = 
                                   GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.wCellRadius;
    /* C-RNTI��Temp C-RNTI�ķֽ�㣬MACʹ�÷ֽ�ǰ�ģ��������ֽ�� */
    tRnlcBbCellCfgReq.tCellInfo.wEdgeofCrnti = 
                        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgGlobalInfoForV25.wCrntiTCrntiEdge;
    /* С��MBMSFN���� */
    tRnlcBbCellCfgReq.tCellInfo.ucCellMbmsAttri =
                              GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucCellMbsfnAtt; 

    tRnlcBbCellCfgReq.tCellInfo.ucOnlySendUCINum = 
                      GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucOnlySendUCINum; 
    /* ��������ʹ�ܸ��� */
    /*֧�ֳ���С��*/
    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));   

    /*��ȡʵ��������*/
    T_CIMVar *ptCIMVar  = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* ��ȡ���ݿ���Ϣ*/
    tGetSvrcpTddInfoReq.wCallType = USF_MSG_CALL;
    tGetSvrcpTddInfoReq.wCId   = ptCIMVar->wCellId;
    tGetSvrcpTddInfoReq.wBpType =1;/*��ʾBPL1*/
#if 0 /* shijunqiang: ��Щ��Ϣľ���ã���� */
    BYTE ucResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tGetSvrcpTddInfoReq, (VOID *)&tGetSvrcpTddInfoAck);
    if ((FALSE == ucResult) || (0 != tGetSvrcpTddInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSvrcpInfoReq, ucResult,tGetSvrcpTddInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "[BBALG]Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return FALSE;
    }
#endif
#if 0
    /* ��֧�ֳ���С�������6��CP������ͨС����ȡ��һ������Ԫ�� */
    BYTE ucDlAntennaEnableNum_temp = 0;
    BYTE ucDlAntennaEnableNum_temp_index = 0;
    BYTE ucPortLoop = 0;
    
    for (BYTE ucCpLoop=0;ucCpLoop<tGetSvrcpTddInfoAck.ucCPNum;ucCpLoop++)
    {
        ucDlAntennaEnableNum_temp = 0;
        ucDlAntennaEnableNum_temp_index = 0;
        
        for(ucPortLoop = 0;ucPortLoop<8;ucPortLoop++)
        {
            if(15 != tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].aucAnttoPortMapV25[ucPortLoop])
            {
                ucDlAntennaEnableNum_temp ++;
            }
        }
        /* ת��Ϊ����ֵ */
        if (1 == ucDlAntennaEnableNum_temp)
        {
            ucDlAntennaEnableNum_temp_index = 0;
        }
        else if(2 == ucDlAntennaEnableNum_temp)
        {
            ucDlAntennaEnableNum_temp_index = 1;
        }
        else if(4 == ucDlAntennaEnableNum_temp)
        {
            ucDlAntennaEnableNum_temp_index = 2;
        }
        else if(8 == ucDlAntennaEnableNum_temp)
        {
            ucDlAntennaEnableNum_temp_index = 3;
        }
        else
        {
            ucDlAntennaEnableNum_temp_index = 255;
        }
       
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntennaEnableNum[ucCpLoop]= ucDlAntennaEnableNum_temp_index;
    }
#endif
    /* С���Զ��û�MIMO��֧������ */
    tRnlcBbCellCfgReq.tCellInfo.ucMUMIMOSupport = 
                        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucMUMIMOSupport;
    /* �Ƿ�֧��64QAM��� */
    tRnlcBbCellCfgReq.tCellInfo.ucUlSupport64QAM =         
                     GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUl64QamDemSpInd;

    /* 0��������Ӧ��CMAC�ϱ�TM����ֵ; 1��tm1,2��tm2,3��tm3,4��
                          tm4,5��tm5,6��tm6,7��tm7��8��tm8����ʱCMAC���ϱ�TM */
    tRnlcBbCellCfgReq.tCellInfo.ucTmAutoSwitch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucTDDUeTransMode;
    
    /* ʱ����붨ʱ������λ��10ms */
    tRnlcBbCellCfgReq.tCellInfo.ucTATimer =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucTimeAlignTimer;
    
    /*���߸�������*/
    tRnlcBbCellCfgReq.tCellInfo.ucSceneCfg =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucSceneCfg;

     /* Channel GID Info */
    tRnlcBbCellCfgReq.dwChannelGidInfoPresent = 1;
    tRnlcBbCellCfgReq.tChannelGidInfo = GetJobContext()->tCIMVar.tChannelGidInfo;

    /* Active Ue Info */
    tRnlcBbCellCfgReq.dwActiveUeInfoPresent = 1;
    /* ��ֵΪ�����ܹ�����ļ����û����澯���� */
    tRnlcBbCellCfgReq.tActiveUeInfo.wActiveUEThreshold = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgGlobalInfoForV25.wActiveUEThreshold; 
    /* PHICH Info */
    tRnlcBbCellCfgReq.dwPHICHInfoPresent = 1;
    /* PHICH��Դ������ */
    tRnlcBbCellCfgReq.tPhichInfo.ucNg = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPdchInfoForV25.ucNg;
    /* ָʾPHICHռ��OFDM������ */
    tRnlcBbCellCfgReq.tPhichInfo.ucPhichDuration = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPdchInfoForV25.ucPhichDuration; 

    /* PUCCH Info */
    tRnlcBbCellCfgReq.dwPUCCHInfoPresent = 1;
    /* PUCCѭ����λ */
    tRnlcBbCellCfgReq.tPucchInfo.ucDeltaPucchShift = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.ucPucchDeltaShf;
    /* 0~98 */
    tRnlcBbCellCfgReq.tPucchInfo.ucN_RB2 = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.ucNumPucchCqiRB;
     /* 0~7 */
    tRnlcBbCellCfgReq.tPucchInfo.ucN_cs1 = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.ucPucchNcsAn; 
#ifdef VS2008
    tRnlcBbCellCfgReq.tPucchInfo.ucN_cs1 = 3;
#endif
    /* ��̬���õ�PUCCH Format1���ŵ�����������SR���뾲̬A/N��A/N Repetiton�ŵ�����֮�ͣ��������������ɺ�̨���� */
    tRnlcBbCellCfgReq.tPucchInfo.wN_PUCCH1 = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchSemiAn +
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchSr + 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchAckRep;
    /* PUSCH Info */
    tRnlcBbCellCfgReq.dwPUSCHInfoPresent = 1;
    /* PUCCH��PUSCH����ο��źŵ�����Ƶʹ��ָʾ */  
    tRnlcBbCellCfgReq.tPuschInfo.ucGroupHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucGrpHopEnableInd;                        
    /* Sequence-hoppingʹ��ָʾ */
    tRnlcBbCellCfgReq.tPuschInfo.ucSeqHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucSeqHopEnableInd;
    /* PUSCH��Ƶʱϵͳ������Ҫ���ֵ��Ӵ���Ŀ */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucNsb = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschNsb;                        
     /* PUSCH����Ƶ��Χָʾ */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucHopMode = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschFHpMode;
    /* ��Ƶ��Ч���� */
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschRB = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschRB;
    /* ��Ƶ�����RB���� */
    tRnlcBbCellCfgReq.tPuschInfo.ucUlHopRBLimt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucUlHopRBLimt;
    /* Group HoppingʱPUSCH��PUCCH������ƫ�Ʋ0~29 */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschGroupAssig = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschGroupAssig;

    /*PUCCH��PUSCH����ο��źŵ�������ʹ��ָʾ��Ƶ������Ƶ�Ƿ�ʹ��ָʾ�� 
    ��������Hopping�����ֶΡ������ֶ�Ϊtrueʱ����������hopping�ֶ���Ч*/
    tRnlcBbCellCfgReq.tPuschInfo.ucPilotHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPilotHopEnable;
    
    /* ��Ƶ������Ϣ */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucHoppingBitsInfo = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucHoppingBitsInfo;
    /* PUSCH��Ƶ��ʼλ�ã�0~98 */
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschHopOffset = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschhopOfst;
    /*С�������вο��źŵ���Чƫ����Ŀ PUSCH����ο��ź�ѭ��ƫ��ֵ��0~7 */
    tRnlcBbCellCfgReq.tPuschInfo.ucCyclicShift = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucnDMRS1;

    /* RACH Info */
    tRnlcBbCellCfgReq.dwRACHInfoPresent = 1;
    /* ��������ǰ������ */   
    tRnlcBbCellCfgReq.tRachInfo.ucNumberOfRaPreambles = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucNumRAPreambles;
    /* GroupAǰ������ */
    tRnlcBbCellCfgReq.tRachInfo.ucSizeOfRaPreamblesGroupA = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucSizeRAGroupA;      
    /* ��������� */
    tRnlcBbCellCfgReq.tRachInfo.ucMaxPreambleTranCt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPreambleTxMax;          
    /* RACH��Ӧ�Ĵ��ڴ�С����λ��TTI��ms��*/
    tRnlcBbCellCfgReq.tRachInfo.ucRachRspWndSize = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucRARspWinSize;
    /* ���������ʱ��ʱ������λ��ms */
    tRnlcBbCellCfgReq.tRachInfo.ucMacConTentionTimer = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucMACContResTimer;
    /* MSG3������������1~8 */      
    tRnlcBbCellCfgReq.tRachInfo.ucMaxMsg3TranCt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucMaxHARQMsg3Tx;             
     /* ��Ӧǰ׺�����͵�����֡�ź���֡�ţ�0~63��ȥ30��46��60-62 */
    tRnlcBbCellCfgReq.tRachInfo.ucPrachCfgIndex = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPrachConfig;             
    /* С�������ƶ����� */ 
    tRnlcBbCellCfgReq.tRachInfo.uchighSpeedFlag = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucCellHighSpdAtt;                                                
    /* ����64��ǰ׺���е��߼������е���ʼ�����ţ�0~837 */
    tRnlcBbCellCfgReq.tRachInfo.wRootSeqIndex =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.wLogRtSeqStNum;               
    /* ѭ����λ������0~15 */  
    tRnlcBbCellCfgReq.tRachInfo.ucNcsCfg =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucNcs;                     
    /* ��һ�����õ�������Դ��������0~104 */
    tRnlcBbCellCfgReq.tRachInfo.ucPrachFreqOffset = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPrachFreqOffset;           
    tRnlcBbCellCfgReq.tRachInfo.ucMsg3CrcErrCnt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucMsg3CrcErrCnt;
    tRnlcBbCellCfgReq.tRachInfo.ucRLFSwitch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucRLFSwitch;
    tRnlcBbCellCfgReq.tRachInfo.ucSelPreGrpThresh = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucSelPreGrpThresh;

    /* SRS Info */
    tRnlcBbCellCfgReq.dwSRSInfoPresent = 1;
    /* SRS����ʹ��ָʾ */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsEnable;                
    /* SRS��֡���� */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsSubframeCfg = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsSubFrameCfg;            
    /* SRS�������� */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsBwCfg =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsBWCfg;
    /* SRS��Ack/Nack�Ƿ����ͬʱ���� */
    tRnlcBbCellCfgReq.tSrsInfo.ucSimAN_SRS = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucAckSrsTraSptInd;
    /* SRS UpPts */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsMaxUpPts = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsMaxUpPts;

    tRnlcBbCellCfgReq.tSrsInfo.ucSRSFrmPwTrsPrtct = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSRSFrmPwTrsPrtct;
    tRnlcBbCellCfgReq.tSrsInfo.ucSRSNFrmPwTrsPrtct = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSRSNFrmPwTrsPrtct;
    tRnlcBbCellCfgReq.tSrsInfo.ucCellSRSFrmPwTrsPrtct = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucCellSRSFrmPwTrsPrtct;
    tRnlcBbCellCfgReq.tSrsInfo.ucCellSRSNFrmPwTrsPrtct = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucCellSRSNFrmPwTrsPrtct;
    tRnlcBbCellCfgReq.tSrsInfo.ucDisconFrmPwTrsPrtct = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucDisconFrmPwTrsPrtct;
    tRnlcBbCellCfgReq.tSrsInfo.ucPowerTransientSwitch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucPowerTransientSwitch;
        
    /* SR Info */
    tRnlcBbCellCfgReq.dwSRInfoPresent = 1;
    /* SR��������,��λ��TTI��ms��*/
    tRnlcBbCellCfgReq.tSRInfo.ucSRTransPeriod =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucSRITransPrd;
    /* SR�������� */
    tRnlcBbCellCfgReq.tSRInfo.ucSRMaxTransNum = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucDsrTransMax;
#if 0 
    /* UL Info */
    tRnlcBbCellCfgReq.dwUlAntennaInfoPresent = 1;
    for (BYTE ucCpLoop=0;ucCpLoop<tGetSvrcpTddInfoAck.ucCPNum;ucCpLoop++)
    {
        //---ת��ʵ�ʼ���--begin
        
        BYTE byLoop_temp1 = 0;
        BYTE byBitMap_temp1 = 0;
        BYTE byAntNum_temp1 = 0;

        byBitMap_temp1 = tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucUpActAntBitmap;
        
        for(byLoop_temp1 = 0;byLoop_temp1 < 8;byLoop_temp1++)
        {
            if(0 != (byBitMap_temp1 & (1<<byLoop_temp1)))
            {
                byAntNum_temp1++;
            }
        }
        /* ת��Ϊö�� */
        if (1 == byAntNum_temp1)
        {
            tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 0;
        }
        else if (2 == byAntNum_temp1)
        {
            tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 1;
        }
        else if (4 == byAntNum_temp1)
        {
            tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 2;
        }
        else if(8 == byAntNum_temp1)
        {
            tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 3;
        }
        else
        {
            tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 255;
        }
        //---ת��ʵ�ʼ���--end
    
        //tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 
                            //tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucUpActAntBitmap;
                            //tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucUlEnabledAntNum;

        /* ÿ���ֽ�ָʾ��Ч���ߺ� */
        BYTE ucValidAntennaIndex = 0;
        for (BYTE ucAntLoop = 0; ucAntLoop < 8; ucAntLoop++)
        {
            if (1 == ((tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucULLogicAntMap >> ucAntLoop) & 0x01))
            {
                tRnlcBbCellCfgReq.tUlAntennaInfo.aucValidAntennaIndex[ucCpLoop][ucValidAntennaIndex] = ucAntLoop;
                ucValidAntennaIndex++;
            }
        }
    }
#endif   
    /* UL PC Info */
    tRnlcBbCellCfgReq.dwUlPCInfoPresent = 1;
    /* Msg3 �ջ����ز���, ��Χ0~14����λ���ޣ�����2��Ĭ��ȡֵΪ6,ʵ��ֵ=�ڴ�ֵ-2 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucDeltaPreambleMsg3 = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPrmbMsg3;
    /* ��ʼpreamble���书�ʣ�0~210����λ��dB������2��Ĭ��ȡֵΪ12,ʵ��ֵ=�ڴ�ֵ-120 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucInitPreRcvPw = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPreInitPwr;
    /* ȡֵ��Χ:enum{0,2 4,6},��λ��dBm,Ĭ��ֵ��2dBm */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPwRampingStep = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPwRampingStep;
    /* ȡֵ��Χ: enum {3,4,5,6,7,8,10,20,50,100,200},Ĭ��ֵ��10 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPreambleTransMax = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPreambleTxMax;
    /* ��̬����P0NominalPusch��Χ��0~150,ʵ��ֵ=�ڴ�ֵ-126,Ĭ��ֵ46 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPuschDynamic =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPusch1;              
    /* PUSCH�ڰ뾲̬������Ȩ��ʽ�·��͵���������Ҫ��С�����幦�ʣ���������Ϊ����
    PUSCH���书�ʵ�һ���֣��������ֲ�ͬС���Ĺ��ʲ��졣ȡֵ��Χ��0~150��ʵ��ֵ=�ڴ�ֵ-126 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPuschPersistent = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPusch0;              
    /* С����·�𲹳�ϵ��,Enumerate{0 ,0.4, 0.5,0.6,0.7,0.8,0.9,1}����Χ������ֵ0~7����λ���ޣ�Ĭ��ȡֵΪ0.8��Ӧ������5 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucAlpha4PL = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucAlfa;              
    /*0~150��Ĭ��46, P0NominalPucch��ʵ��ֵ=�ڴ�ֵ-126*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPucch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPucch;              
    /*0~63����λdbm��Ĭ��ֵ53, �����ue����书��, ��ʵ��ֵ=�ڴ�ֵ-30*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPMax = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucUePMax;
    /* 0-- PUSCH����DCI Format 3��ʽ���й���;1-- PUSCH����DCI Format 3A��ʽ���й��� */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPuschDCIType = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPuschDCIType;
    /* ��PUCCH��ʽ1a��Ϊ�ο���ʽ��ÿ��PUCCH��ʽ��Ӧ�Ĵ����ʽ��������С���ض���enumerate{enumerate{-2,0,2},
    enumerate{1,3,5},enumerate{-2,0,1,2},enumerate{-2,0,2},enumerate{-2,0,2}},ȱʡֵ{0,3,0,0,2} */
    tRnlcBbCellCfgReq.tUlPCInfo.aucDeltaFPUCCH[0] = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPoPucchF1;              
    tRnlcBbCellCfgReq.tUlPCInfo.aucDeltaFPUCCH[1] = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPoPucchF1b;              
    tRnlcBbCellCfgReq.tUlPCInfo.aucDeltaFPUCCH[2] = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPoPucchF2;              
    tRnlcBbCellCfgReq.tUlPCInfo.aucDeltaFPUCCH[3] = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPoPucchF2a;              
    tRnlcBbCellCfgReq.tUlPCInfo.aucDeltaFPUCCH[4] = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPoPucchF2b; 
    /*0-- PUCCH����DCI Format 3��ʽ���й���,1-- PUCCH����DCI Format 3A��ʽ���й���*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPucchDCIType = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPucchDCIType;
    
    /* DL PC Info */
    tRnlcBbCellCfgReq.dwDlPCInfoPresent = 1;

    /* ����RS��PDSCH EPRE�벻����RS��PDSCH EPRE�ı�ֵ������0~3 */ 
    tRnlcBbCellCfgReq.tDlPCInfo.ucPB = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.ucPB;
    
#if 0    
    /* С������书�ʣ���λ��dBm��0~50 */ 
    tRnlcBbCellCfgReq.tDlPCInfo.wMaxCellTransPwr = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.wMaxCellTransPwr;          
    /* С�����书�ʣ���λ��dBm��0~50 */
    tRnlcBbCellCfgReq.tDlPCInfo.wCellTransPwr = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.wCellTransPwr;

    /* С���ο��źŹ��ʣ���λ��dBm��-60~50 */ 
    for (BYTE ucCpLoop=0;ucCpLoop<tGetSvrcpTddInfoAck.ucCPNum;ucCpLoop++)
    {
        tRnlcBbCellCfgReq.tDlPCInfo.awReferSignalPower[ucCpLoop] = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.wCellSpeRefSigPwr;
    }
#endif
    Message tRnlcBbCellCfgReqMsg;
    tRnlcBbCellCfgReqMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tRnlcBbCellCfgReqMsg.m_dwSignal = EV_RNLC_BB_CELL_CFG_REQ;
    tRnlcBbCellCfgReqMsg.m_wLen = sizeof(tRnlcBbCellCfgReq);
    tRnlcBbCellCfgReqMsg.m_pSignalPara = static_cast<void*>(&tRnlcBbCellCfgReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_AdptBBComponent, &tRnlcBbCellCfgReqMsg); 
    if (SSL_OK != dwResult)
    {
        RnlcLogTrace(PRINT_RNLC_CCM,  //����ģ��� 
                     __FILE__,        //�ļ�����
                     __LINE__,        //�ļ��к�
                     GetJobContext()->tCIMVar.wCellId,  //С����ʶ 
                     RNLC_INVALID_WORD,  //UE GID 
                     ERR_CCM_CIM_FunctionCallFail_USF_SendTo,  //�쳣̽������� 
                     GetJobContext()->tCIMVar.wInstanceNo,  //�쳣̽�븨����
                     dwResult,  //�쳣̽����չ������
                     RNLC_FATAL_LEVEL,  //��ӡ����
                     "\n CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! \n");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("EV_RNLC_BB_CELL_CFG_REQ", EV_RNLC_BB_CELL_CFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_BB_CELL_CFG_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcBbCellCfgReq),
                     (const BYTE *)(&tRnlcBbCellCfgReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReq
* ��������: ����С������������Ϣ��RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReq(VOID)
{
    T_RnlcRrmCellCfgReq tRnlcRrmCellCfgReq;
    memset(&tRnlcRrmCellCfgReq, 0, sizeof(tRnlcRrmCellCfgReq));
    
    /* С��ID */
    tRnlcRrmCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRrmCellCfgReq.tCellMacRbInfo = GetJobContext()->tCIMVar.tCellMacRbInfo;

    PID tRRMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tRRMPid;       
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRM_CELL_CFG_REQ, 
                                             &tRnlcRrmCellCfgReq, 
                                             sizeof(tRnlcRrmCellCfgReq), 
                                             COMM_RELIABLE, 
                                             &tRRMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }

    CIMPrintSendMessage("EV_RNLC_RRM_CELL_CFG_REQ", EV_RNLC_RRM_CELL_CFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRM_CELL_CFG_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRrmCellCfgReq),
                     (const BYTE *)(&tRnlcRrmCellCfgReq));
                     
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendSIUpdReq
* ��������: ����С��ϵͳ�㲥������Ϣ��ϵͳ�㲥����
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendSIUpdReq(VOID)
{    
    T_SysInfoUpdReq tSysInfoUpdReq;
    memset(&tSysInfoUpdReq, 0, sizeof(tSysInfoUpdReq));
    
    tSysInfoUpdReq.wCellId = GetJobContext()->tCIMVar.wCellId;

    if (OPRREASON_ADD_ByDelAdd == m_ucCellSetupReason)
    {
        tSysInfoUpdReq.ucCellSIUpdCause = SI_UPD_CELL_DELADD;
    }
    else if (OPRREASON_ADD_ByUnBlock == m_ucCellSetupReason)
    {
        tSysInfoUpdReq.ucCellSIUpdCause = SI_UPD_CELL_ADD_byUNBLOCK;
    }
    else
    {
        tSysInfoUpdReq.ucCellSIUpdCause = SI_UPD_CELL_SETUP;
    }

    Message tCmdCellSysInfoUpdMsg;
    tCmdCellSysInfoUpdMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tCmdCellSysInfoUpdMsg.m_dwSignal = CMD_CIM_CELL_SYSINFO_UPDATE_REQ;
    tCmdCellSysInfoUpdMsg.m_wLen = sizeof(tSysInfoUpdReq);
    tCmdCellSysInfoUpdMsg.m_pSignalPara = static_cast<void*>(&tSysInfoUpdReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_SIBroadcastComponent, &tCmdCellSysInfoUpdMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("CMD_CIM_CELL_SYSINFO_UPDATE_REQ", 
                         CMD_CIM_CELL_SYSINFO_UPDATE_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_SYSINFO_UPDATE_REQ,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tSysInfoUpdReq),
                     (const BYTE *)(&tSysInfoUpdReq));
                     
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendCellCfgRsp
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCellCfgRsp(WORD32 dwRspResult)
{
    T_CmdCIMCellCfgRsp tCmdCIMCellCfgRsp;
    memset(&tCmdCIMCellCfgRsp, 0, sizeof(tCmdCIMCellCfgRsp));
    
    tCmdCIMCellCfgRsp.dwResult = dwRspResult;
    
    Message tCmdCIMCellCfgRspMsg;
    tCmdCIMCellCfgRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tCmdCIMCellCfgRspMsg.m_dwSignal = CMD_CIM_CELL_CFG_RSP;
    tCmdCIMCellCfgRspMsg.m_wLen = sizeof(tCmdCIMCellCfgRsp);
    tCmdCIMCellCfgRspMsg.m_pSignalPara = static_cast<void*>(&tCmdCIMCellCfgRsp);

    WORD32 dwResult = SendTo(ID_CCM_CIM_MainComponent, &tCmdCIMCellCfgRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("CMD_CIM_CELL_CFG_RSP", CMD_CIM_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_CFG_RSP,
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCIMCellCfgRsp),
                     (const BYTE *)(&tCmdCIMCellCfgRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SendCfgCancelRspToCcmMain
* ��������: ����С������ȡ����Ӧ��Ϣ��CIM������
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCfgCancelRspToCcmMain(VOID)
{
    T_CmdCIMCellCfgCancelRsp tCmdCIMCellCfgCancelRsp;
    memset(&tCmdCIMCellCfgCancelRsp, 0, sizeof(tCmdCIMCellCfgCancelRsp));

    Message tCmdCIMCellCfgCancelRspMsg;
    tCmdCIMCellCfgCancelRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tCmdCIMCellCfgCancelRspMsg.m_dwSignal = CMD_CIM_CELL_CFG_CANCEL_RSP;
    tCmdCIMCellCfgCancelRspMsg.m_wLen = sizeof(tCmdCIMCellCfgCancelRsp);
    tCmdCIMCellCfgCancelRspMsg.m_pSignalPara = 
                                   static_cast<void*>(&tCmdCIMCellCfgCancelRsp);

    WORD32 dwResult = SendTo(ID_CCM_CIM_MainComponent, &tCmdCIMCellCfgCancelRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("CMD_CIM_CELL_CFG_CANCEL_RSP", CMD_CIM_CELL_CFG_CANCEL_RSP);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::SaveBBCfgRspPara
* ��������: ����������͵�С��������Ӧ��Я������Ϣ��֮����Ҫ��С�����������д���
*           RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::
SaveBBCfgRspPara(const T_BbRnlcCellCfgRsp *ptBbRnlcCellCfgRsp)
{
    CCM_NULL_POINTER_CHECK(ptBbRnlcCellCfgRsp);

    /* С����ʼ����ʱ��Ч��ȡֵΪ����Ч��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlModulationEfficiency =
        ptBbRnlcCellCfgRsp->wUlModulationEfficiency;      
    /* PUCCH + PRACHʹ�õ�PRB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlCommonUsedPRB =
        ptBbRnlcCellCfgRsp->wUlCommonUsedPRB;          
    /* �����ܵĿ���RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlTotalAvailRB = 
        ptBbRnlcCellCfgRsp->dwUlTotalAvailRB;          
    /* ����GBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlGbrUsedPRB =
        ptBbRnlcCellCfgRsp->wUlGbrUsedPRB;             
    /* ����NGBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlNgbrUsedPRB =
        ptBbRnlcCellCfgRsp->wUlNgbrUsedPRB;           
    /* ����NGBR�ϱ������ڵ�PBR���ۼ�ֵ����λ��kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlNgbrTotalPBR =
        ptBbRnlcCellCfgRsp->dwUlNgbrTotalPBR;          
    /* ����NGBR�ϱ������ڵ�����������λ��kBps */ 
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlNgbrThroghput =
        ptBbRnlcCellCfgRsp->dwUlNgbrThroghput;         
    /* С����ʼ����ʱ��Ч��ȡֵΪ���Ʊ���Ч��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlModulationEfficiency =
        ptBbRnlcCellCfgRsp->wDlModulationEfficiency;  
    /* BCCH+PCCH+Msg2+Msg4ʹ�õ�RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlCommonUsedPRB =
        ptBbRnlcCellCfgRsp->wDlCommonUsedPRB;          
    /* �����ܵĿ���RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlTotalAvailRB =
        ptBbRnlcCellCfgRsp->dwDlTotalAvailRB;          
    /* ����GBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlGbrUsedPRB =
        ptBbRnlcCellCfgRsp->wDlGbrUsedPRB;             
    /* ����NGBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlNgbrUsedPRB =
        ptBbRnlcCellCfgRsp->wDlNgbrUsedPRB;            
    /* ����NGBR�ϱ������ڵ�PBR���ۼ�ֵ����λ��kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrTotalPBR =
        ptBbRnlcCellCfgRsp->dwDlNgbrTotalPBR;          
    /* ����NGBR�ϱ������ڵ�����������λ��kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrThroghput =
        ptBbRnlcCellCfgRsp->dwDlNgbrThroghput;         
    /* ����ר��Gbrҵ��ռ�õĹ���*100 ��λ��mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlGbrUsedPower =
        ptBbRnlcCellCfgRsp->dwDlGbrUsedPower;          
    /* ����ר��Ngbrҵ��ռ�õĹ���*100 ��λ��mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrUsedPower =
        ptBbRnlcCellCfgRsp->dwDlNgbrUsedPower;         
    /* �����ŵ�ר�õĹ���*100 ��λ��mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlCommonUsedPower =
        ptBbRnlcCellCfgRsp->dwDlCommonUsedPower;       
    /* ���п�����ҵ����ȵ�ʣ�๦��ֵ*100����λ��mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlRestAvailPower =
        ptBbRnlcCellCfgRsp->dwDlRestAvailPower;

    return RNLC_SUCC;
}



/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::CIMPrintTranState
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
WORD32 CCM_CIM_CellSetupComponentFsm::CIMPrintTranState(const void  *pTargetState, BYTE ucTargetState)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo:%d, CCM_CIM_CellSetupComponentFsm TranState To %s, StateID: %d ",
                wInstanceNo, pTargetState, ucTargetState);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::CIMPrintRecvMessage
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
WORD32 CCM_CIM_CellSetupComponentFsm::
CIMPrintRecvMessage(const void *pSignal, WORD32 dwSignal)
{    
    CCM_CIM_NULL_POINTER_CHECK(pSignal);
    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                "CIM InstanceNo: %d, CCM_CIM_CellSetupComponentFsm CurrentState: %d, Receive Message %s, MessageID: %d",
                wInstanceNo, dwTag, pSignal, dwSignal);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::CIMPrintSendMessage
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
WORD32 CCM_CIM_CellSetupComponentFsm::
CIMPrintSendMessage(const void *pSignal, WORD32 dwSignal)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo: %d, CCM_CIM_CellSetupComponentFsm CurrentState: %d, Send Message %s, MessageID: %d ",
                wInstanceNo, dwTag, pSignal, dwSignal);
   
    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRUCfgRsp
* ��������: ����RRUС��������Ӧ������������Ҫ��������RRU���е���
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRUCfgRsp(VOID)
{
    T_RruRnlcCellCfgRsp tRruRnlcCellCfgRsp;
    memset(&tRruRnlcCellCfgRsp, 0, sizeof(tRruRnlcCellCfgRsp));
    
    /* С��ID */
    tRruRnlcCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* С������������0~5 */
    tRruRnlcCellCfgRsp.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    tRruRnlcCellCfgRsp.dwResult = 0;

    Message tRruRnlcCellCfgRspMsg;
    tRruRnlcCellCfgRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tRruRnlcCellCfgRspMsg.m_dwSignal = EV_RRU_RNLC_CELL_CFG_RSP;
    tRruRnlcCellCfgRspMsg.m_wLen = sizeof(tRruRnlcCellCfgRsp);
    tRruRnlcCellCfgRspMsg.m_pSignalPara = static_cast<void*>(&tRruRnlcCellCfgRsp);

    WORD32 dwResult = SendToSelf(&tRruRnlcCellCfgRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }
    
    CIMPrintSendMessage("EV_RRU_RNLC_CELL_CFG_RSP", EV_RRU_RNLC_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RRU_RNLC_CELL_CFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRruRnlcCellCfgRsp),
                     (const BYTE *)(&tRruRnlcCellCfgRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRNLUCfgRsp
* ��������: ����RNLUС��������Ӧ������������Ҫ��������RNLU���е���
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRNLUCfgRsp(VOID)
{
    T_RnluRnlcCellCfgRsp tRnluRnlcCellCfgRsp;
    memset(&tRnluRnlcCellCfgRsp, 0, sizeof(tRnluRnlcCellCfgRsp));
    
    /* С��ID */
    tRnluRnlcCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* С������������0~5 */
    tRnluRnlcCellCfgRsp.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    tRnluRnlcCellCfgRsp.wTimeStamp = 0;
    tRnluRnlcCellCfgRsp.dwResult = 0;

    Message tRnluRnlcCellCfgRspMsg;
    tRnluRnlcCellCfgRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tRnluRnlcCellCfgRspMsg.m_dwSignal = EV_RNLU_RNLC_CELL_CFG_RSP;
    tRnluRnlcCellCfgRspMsg.m_wLen = sizeof(tRnluRnlcCellCfgRsp);
    tRnluRnlcCellCfgRspMsg.m_pSignalPara = static_cast<void*>(&tRnluRnlcCellCfgRsp);

    WORD32 dwResult = SendToSelf(&tRnluRnlcCellCfgRspMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CIMPrintSendMessage("EV_RNLU_RNLC_CELL_CFG_RSP", EV_RNLU_RNLC_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RNLU_RNLC_CELL_CFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRnluRnlcCellCfgRsp),
                     (const BYTE *)(&tRnluRnlcCellCfgRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfBBCfgRsp
* ��������: ����BBС��������Ӧ������������Ҫ��������BB���е���
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfBBCfgRsp(VOID)
{
    T_BbRnlcCellCfgRsp tBbRnlcCellCfgRsp;
    memset(&tBbRnlcCellCfgRsp, 0, sizeof(tBbRnlcCellCfgRsp));

    /*BB����ͷ*/
    /* ��Ϣ���ȣ�ָ��Ϣ�ĳ��ȣ�����MsgLen����ֶα��� */
    tBbRnlcCellCfgRsp.tMsgHeader.wMsgLen = sizeof(tBbRnlcCellCfgRsp);
    /* ��Ϣ���� */
    tBbRnlcCellCfgRsp.tMsgHeader.wMsgType = EV_BB_RNLC_CELL_CFG_RSP;    

    /* ��ˮ�� */
    tBbRnlcCellCfgRsp.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* С��ID */
    tBbRnlcCellCfgRsp.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* С���������� */
    tBbRnlcCellCfgRsp.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* ʱ��� */
    tBbRnlcCellCfgRsp.tMsgHeader.wTimeStamp = 0;

    /* С����ʼ����ʱ��Ч��ȡֵΪ����Ч��*100 */
    tBbRnlcCellCfgRsp.wUlModulationEfficiency = I_DONT_KNOW;      
    /* PUCCH + PRACHʹ�õ�PRB��*100 */
    tBbRnlcCellCfgRsp.wUlCommonUsedPRB = I_DONT_KNOW;          
    /* �����ܵĿ���RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.dwUlTotalAvailRB = I_DONT_KNOW;          
    /* ����GBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.wUlGbrUsedPRB = I_DONT_KNOW;             
    /* ����NGBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.wUlNgbrUsedPRB = I_DONT_KNOW;           
    /* ����NGBR�ϱ������ڵ�PBR���ۼ�ֵ����λ��kBps */
    tBbRnlcCellCfgRsp.dwUlNgbrTotalPBR = I_DONT_KNOW;          
    /* ����NGBR�ϱ������ڵ�����������λ��kBps */ 
    tBbRnlcCellCfgRsp.dwUlNgbrThroghput = I_DONT_KNOW;         
    /* С����ʼ����ʱ��Ч��ȡֵΪ���Ʊ���Ч��*100 */
    tBbRnlcCellCfgRsp.wDlModulationEfficiency = I_DONT_KNOW;  
    /* BCCH+PCCH+Msg2+Msg4ʹ�õ�RB��*100 */
    tBbRnlcCellCfgRsp.wDlCommonUsedPRB = I_DONT_KNOW;          
    /* �����ܵĿ���RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.dwDlTotalAvailRB = I_DONT_KNOW;          
    /* ����GBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.wDlGbrUsedPRB = I_DONT_KNOW;             
    /* ����NGBRҵ��ʹ�õ�RB����ȡֵΪͳ��RB��*100 */
    tBbRnlcCellCfgRsp.wDlNgbrUsedPRB = I_DONT_KNOW;            
    /* ����NGBR�ϱ������ڵ�PBR���ۼ�ֵ����λ��kBps */
    tBbRnlcCellCfgRsp.dwDlNgbrTotalPBR = I_DONT_KNOW;          
    /* ����NGBR�ϱ������ڵ�����������λ��kBps */
    tBbRnlcCellCfgRsp.dwDlNgbrThroghput = I_DONT_KNOW;         
    /* ����ר��Gbrҵ��ռ�õĹ���*100 ��λ��mw */
    tBbRnlcCellCfgRsp.dwDlGbrUsedPower = I_DONT_KNOW;          
    /* ����ר��Ngbrҵ��ռ�õĹ���*100 ��λ��mw */
    tBbRnlcCellCfgRsp.dwDlNgbrUsedPower = I_DONT_KNOW;         
    /* �����ŵ�ר�õĹ���*100 ��λ��mw */
    tBbRnlcCellCfgRsp.dwDlCommonUsedPower = I_DONT_KNOW;       
    /* ���п�����ҵ����ȵ�ʣ�๦��ֵ*100����λ��mw */
    tBbRnlcCellCfgRsp.dwDlRestAvailPower = I_DONT_KNOW;         

    Message tBbRnlcCellCfgRspMsg;
    tBbRnlcCellCfgRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tBbRnlcCellCfgRspMsg.m_dwSignal = EV_BB_RNLC_CELL_CFG_RSP;
    tBbRnlcCellCfgRspMsg.m_wLen = sizeof(tBbRnlcCellCfgRsp);
    tBbRnlcCellCfgRspMsg.m_pSignalPara = static_cast<void*>(&tBbRnlcCellCfgRsp);

    WORD32 dwResult = SendToSelf(&tBbRnlcCellCfgRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CIMPrintSendMessage("EV_BB_RNLC_CELL_CFG_RSP", EV_BB_RNLC_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_BB_RNLC_CELL_CFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tBbRnlcCellCfgRsp),
                     (const BYTE *)(&tBbRnlcCellCfgRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* ��������: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRMCfgRsp
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRMCfgRsp(VOID)
{
    T_RrmRnlcCellCfgRsp tRrmRnlcCellCfgRsp;
    memset(&tRrmRnlcCellCfgRsp, 0, sizeof(tRrmRnlcCellCfgRsp));
    
    /* С��ID */
    tRrmRnlcCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRrmRnlcCellCfgRsp.dwResult = 0;

    Message tRrmRnlcCellCfgRspMsg;
    tRrmRnlcCellCfgRspMsg.m_wSourceId = ID_CCM_CIM_CellSetupComponent;
    tRrmRnlcCellCfgRspMsg.m_dwSignal = EV_RRM_RNLC_CELL_CFG_RSP; 
    tRrmRnlcCellCfgRspMsg.m_wLen = sizeof(tRrmRnlcCellCfgRsp);
    tRrmRnlcCellCfgRspMsg.m_pSignalPara = static_cast<void*>(&tRrmRnlcCellCfgRsp);

    WORD32 dwResult = SendToSelf(&tRrmRnlcCellCfgRspMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_CellSetupComponentFsm FunctionCallFail_SDF_SendToSelf! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendToSelf;
    }

    CIMPrintSendMessage("EV_RRM_RNLC_CELL_CFG_RSP", EV_RRM_RNLC_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_RRM_RNLC_CELL_CFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tRrmRnlcCellCfgRsp),
                     (const BYTE *)(&tRrmRnlcCellCfgRsp));

    return RNLC_SUCC;
}


WORD32 CCM_CIM_CellSetupComponentFsm::CimPowerOnInSlave()
{
   return RNLC_SUCC;
}

VOID CCM_CIM_CellSetupComponentFsm::SetComponentSlave()
{
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, Handle_InSlaveState,CCM_ALLCOMPONET_SLAVE_STATE);
}

VOID CCM_CIM_CellSetupComponentFsm::Handle_InSlaveState(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg)
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_SYNC_COMPONENT:
        {
            /*�����幹����״̬��������*/
            TCimComponentInfo    *ptCimComponentInfo = \
                                       (TCimComponentInfo *)pMsg->m_pSignalPara;
            
            ucMasterSateCpy = ptCimComponentInfo->ucState;
            CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellSetupComponentFsm get State %d.\n",ucMasterSateCpy);
            break;
        }
        
        case CMD_CIM_SLAVE_TO_MASTER:
        {
            CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellSetupComponentFsm SLAVE_TO_MASTER.\n");
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

WORD32 CCM_CIM_CellSetupComponentFsm::HandleOcfPaOper(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara)

    T_RnlcOamCellOperationReq tRnlcOamCellOperReq;
    T_CmmCimPaOper              *ptCmmCimPaOper = NULL;

    ptCmmCimPaOper = (T_CmmCimPaOper *)(pMsg->m_pSignalPara);

    ucOcfOperType = ptCmmCimPaOper->ucOperType;

    memset((VOID *)&tRnlcOamCellOperReq,0,sizeof(T_RnlcOamCellOperationReq));

    tRnlcOamCellOperReq.wSrvCellId = GetJobContext()->tCIMVar.wCellId;    
    tRnlcOamCellOperReq.wActionInd = ptCmmCimPaOper->ucOperType;/*0 ��ʾ�ر� 1 ��ʾ��*/
      
    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_OAM_CELL_OPERATION_REQ, 
                                         &tRnlcOamCellOperReq, 
                                         sizeof(T_RnlcOamCellOperationReq), 
                                         COMM_RELIABLE, 
                                         &tOAMPid);

    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_LOG(ERROR_LEVEL,"Send Son Msg %d Fail in HandleOcfPaOper.\n",EV_RNLC_OAM_CELL_OPERATION_REQ);
    }
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::HandleOamPaOperResult(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara)
    CCM_LOG(RNLC_INFO_LEVEL,"recive EV_CMM_CIM_OAM_CELL_PAOPR_RSP  sucess!");

    T_RnlcOcfCellOpenRsp        tRnlcOcfCellOpenRsp;
    T_RnlcOcfCellCloseRsp        tRnlcOcfCellCloseRsp;
    T_CmmCimOamPaOperRsp *pCmmCimOamPaOperRsp = NULL;
    WORD16   wEvent;
    VOID        *pData = NULL;
    WORD16   wLen = 0;

    pCmmCimOamPaOperRsp = (T_CmmCimOamPaOperRsp *)(pMsg->m_pSignalPara);

    switch(ucOcfOperType)
    {
        case 0:
        {
            wEvent = EV_RNLC_OCF_CELL_CLOSE_RSP;
            wLen = sizeof(T_RnlcOcfCellOpenRsp);
            tRnlcOcfCellOpenRsp.wSrvCellId = pCmmCimOamPaOperRsp->wCellId;
            tRnlcOcfCellOpenRsp.wResult = pCmmCimOamPaOperRsp->wResult;

            pData = (VOID *)&tRnlcOcfCellOpenRsp;
            break;
        }

        case 1:
        {
            wEvent = EV_RNLC_OCF_CELL_OPEN_RSP;
            wLen = sizeof(T_RnlcOcfCellCloseRsp);
            tRnlcOcfCellCloseRsp.wSrvCellId = pCmmCimOamPaOperRsp->wCellId;
            tRnlcOcfCellCloseRsp.wResult = pCmmCimOamPaOperRsp->wResult;

            pData = (VOID *)&tRnlcOcfCellCloseRsp;
            break;
        }
        default:
        {
            CCM_LOG(ERROR_LEVEL,"ucOcfOperType is Error.\n");
            return RNLC_SUCC;
        }

    }
    ucOcfOperType = 0xFF;
#ifndef WIN32
    PID tSonPid = {};
#else
    PID tSonPid = {0};
#endif

    WORD32 dwInstanceNo = OSS_ConstructPNO(P_S_LTE_SON_MAIN, 1);
    WORD32 dwOssRet = OSS_GetPIDByPNO(dwInstanceNo, &tSonPid);
    if (OSS_SUCCESS != dwOssRet)
    {
        CCM_LOG(ERROR_LEVEL,"Get Son PID Fail %d in HandleOamPaOperResult.\n",dwOssRet);
        return dwOssRet;
    }

    dwOssRet = OSS_SendAsynMsg(wEvent,pData,wLen,COMM_RELIABLE,&tSonPid);

    if (OSS_SUCCESS != dwOssRet)
    {
        CCM_LOG(ERROR_LEVEL,"Send Son Msg %d Fail in HandleOamPaOperResult.\n",wEvent);
        return dwOssRet;
    }
    
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::HandleMasterToSlave(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellSetupComponentFsm HandleMasterToSlave\n");
    SetComponentSlave();
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::HandleSlaveToMaster(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_CellSetupComponentFsm HandleSlaveToMaster State is %d.\n",ucMasterSateCpy);
    /*lint -save -e30 -e142*/
    switch(ucMasterSateCpy)
    {
        case S_CIM_SETUPCOM_IDLE:
               TranStateWithTag(CCM_CIM_CellSetupComponentFsm, Idle, S_CIM_SETUPCOM_IDLE);
               break;
        
        case S_CIM_SETUPCOM_SETUPOK:
               TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                                             SetupOK, 
                                             S_CIM_SETUPCOM_SETUPOK);
               break;
        
        case S_CIM_SETUPCOM_SETUPERROR:
               TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                                             SetupError, 
                                             S_CIM_SETUPCOM_SETUPERROR);
               break;
        
        case S_CIM_SETUPCOM_SETUPCANCEL:
              TranStateWithTag(CCM_CIM_CellSetupComponentFsm, Idle, S_CIM_SETUPCOM_IDLE);
              break;
        
        default:
            TranStateWithTag(CCM_CIM_CellSetupComponentFsm, Idle, S_CIM_SETUPCOM_IDLE);
            break;
    }
    /*lint -restore*/
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_Tdd(VOID)
{
    WORD32 dwResult = 0;
    WORD16 wBoardType = 0 ;
    
    /* �������ݿ��ж�С�������Ļ�����״̬�Ƿ����� */
     dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);

    if(wBoardType == USF_BPLBOARDTYPE_BPL1)
    {
        /*BPL1�Ľӿ�����*/
        dwResult = SendToRruCfgBplV25_Tdd();
    }
    else
    {
        /*BPL0�Ľӿ�����*/
        dwResult = SendToRruCfgBplV20_Tdd();
    }
    
    return RNLC_SUCC;
}
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_Fdd(VOID)
{
    WORD32 dwResult = 0;
    WORD16 wBoardType = 0 ;
    
    /* �������ݿ��ж�С�������Ļ�����״̬�Ƿ����� */
     dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);

    if(wBoardType == USF_BPLBOARDTYPE_BPL1)
    {
        /*BPL1�Ľӿ�����*/
        dwResult = SendToRruCfgBplV25_Fdd();
    }
    else
    {
        /*BPL0�Ľӿ�����*/
        dwResult = SendToRruCfgBplV20_Fdd();
    }
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::SendToRruCfgBplV25_Tdd()
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

    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
    
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }

    tRnlcRruCellCfgReq.wOperateType = 0; /*Setup*/
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tDbsGetSuperCpInfoAck.ucCPNum;
    tRnlcRruCellCfgReq.ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;

    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    pSuperCpInfo          = tDbsGetSuperCpInfoAck.atCpInfo;
    for(BYTE ucCpLoop = 0; ucCpLoop < tDbsGetSuperCpInfoAck.ucCPNum && ucCpLoop < MAX_CP_NUM_6;ucCpLoop++)
    {
        ptRnlcCpCellCfgInfo->wCpId = (WORD16)pSuperCpInfo->ucCPId;
        ptRnlcCpCellCfgInfo->ucCellIdInBoard = pSuperCpInfo->ucBufferIdInBoard;

        ptRnlcCpCellCfgInfo->wCellPower = pSuperCpInfo->wCPTransPwr + GetCpPwrByStdWgt(GetJobContext()->tCIMVar.wCellId,
                                                                                                                                   pSuperCpInfo->ucCPId,
                                                                                                                                   pSuperCpInfo->ucDlEnabledAntNum);   /*tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellTransPwr;*/


        /* С���ز���/Ƶ������Ŀǰ��֧�ֵ��ز���Ŀǰ�̶�Ϊ1 */
        ptRnlcCpCellCfgInfo->ucCarrNum = 1;
        /* �ز��� */  /*OAM����Ҫ����ʱ���޸ģ������׮ */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].ucCarrNo = 1;
        /* ������ƵEARFCN ����rru��Ƶ����Ҫת��֮ǰ��ֵ*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*��ȡƵ��ָʾ*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* �ز�����ö��ֵ��{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* ��������֡���ã�ö��ֵ��{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* ������֡���ã�ö��ֵ��{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* ���õ���Чϵͳ��֡�ţ�Ϊ10ms֡��0~1023 */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCfgValidSFN = 0;

        ptRnlcCpCellCfgInfo++;
        pSuperCpInfo++;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);


    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));
    
    /*tRnlcRruCellCfgCommForSigTrace.dwTDDPresent = 1;*/
    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCellCfgReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
    
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::SendToRruCfgBplV20_Tdd()
{
    T_RnlcRruCellCfgReq            tRnlcRruCellCfgReq; 
    T_RnlcCpCellCfgInfo            *ptRnlcCpCellCfgInfo = NULL;
    
    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;
    BOOL  bResult = FALSE;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));   
    memset(&tRnlcRruCellCfgReq, 0, sizeof(tRnlcRruCellCfgReq));

    /* ��ȡ���ݿ���Ϣ*/
    tGetSvrcpTddInfoReq.wCallType = USF_MSG_CALL;
    tGetSvrcpTddInfoReq.wCId   = GetJobContext()->tCIMVar.wCellId;
    BYTE ucResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tGetSvrcpTddInfoReq, (VOID *)&tGetSvrcpTddInfoAck);
    if ((FALSE == ucResult) || (0 != tGetSvrcpTddInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetSvrcpTddInfo, ucResult,tGetSvrcpTddInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return FALSE;
    }

    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
    
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }

    T_DBS_GetBpBoardInfoByCellId_REQ    tDBSGetBpBoardInfoByCellIdREQ;
    T_DBS_GetBpBoardInfoByCellId_ACK    tDBSGetBpBoardInfoByCellIdACK;
    
    memset(&tDBSGetBpBoardInfoByCellIdREQ, 0x00, sizeof(T_DBS_GetBpBoardInfoByCellId_REQ));
    memset(&tDBSGetBpBoardInfoByCellIdACK, 0x00, sizeof(T_DBS_GetBpBoardInfoByCellId_ACK));
    
        
    tDBSGetBpBoardInfoByCellIdREQ.wCallType = USF_MSG_CALL;
    tDBSGetBpBoardInfoByCellIdREQ.wCellId   = GetJobContext()->tCIMVar.wCellId;
    
    bResult = UsfDbsAccess(EV_DBS_GetBpBoardInfoByCellId_REQ, &tDBSGetBpBoardInfoByCellIdREQ, &tDBSGetBpBoardInfoByCellIdACK);
    
    if ((TRUE != bResult) || (0 != tDBSGetBpBoardInfoByCellIdACK.dwResult))
    {
        /* �쳣�ϱ� */
        
        return ((tDBSGetBpBoardInfoByCellIdACK.dwResult << 16));
    }

    tRnlcRruCellCfgReq.wOperateType = 0; /*Setup*/
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tGetSvrcpTddInfoAck.ucCPNum;
    tRnlcRruCellCfgReq.ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;
    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    
    for(BYTE ucCpLoop = 0; ucCpLoop < tGetSvrcpTddInfoAck.ucCPNum && ucCpLoop < MAX_CP_NUM_6;ucCpLoop++)
    {
        ptRnlcCpCellCfgInfo->wCellPower = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellTransPwr;
        ptRnlcCpCellCfgInfo->wCpId = tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucCPId;
        ptRnlcCpCellCfgInfo->ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
        ptRnlcCpCellCfgInfo->byRruRack = tDBSGetBpBoardInfoByCellIdACK.ucBpRack;
        ptRnlcCpCellCfgInfo->byRruShelf = tDBSGetBpBoardInfoByCellIdACK.ucBpShelf;
        ptRnlcCpCellCfgInfo->byRruSlot = tDBSGetBpBoardInfoByCellIdACK.ucBpSlotNo;

        /* С���ز���/Ƶ������Ŀǰ��֧�ֵ��ز���Ŀǰ�̶�Ϊ1 */
        ptRnlcCpCellCfgInfo->ucCarrNum = 1;
        /* �ز��� */  /*OAM����Ҫ����ʱ���޸ģ������׮ */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].ucCarrNo = 1;
        /* ������ƵEARFCN ����rru��Ƶ����Ҫת��֮ǰ��ֵ*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*��ȡƵ��ָʾ*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* �ز�����ö��ֵ��{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* ��������֡���ã�ö��ֵ��{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* ������֡���ã�ö��ֵ��{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* ���õ���Чϵͳ��֡�ţ�Ϊ10ms֡��0~1023 */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCfgValidSFN = 0;
        ptRnlcCpCellCfgInfo++;
    }

    WORD32 dwGetPwr = 0;

    dwGetPwr = GetCpPwrFunction_tdd(&tRnlcRruCellCfgReq);
    if(RNLC_SUCC != dwGetPwr)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                        GetJobContext()->tCIMVar.wInstanceNo,
                        dwGetPwr,
                        RNLC_FATAL_LEVEL, 
                        " CCM_CIM_CellSetupComponentFsm FunctionCallFail_GetCpPwrFunction_tdd! ");
    }
    

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);


    //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));
    
    /*tRnlcRruCellCfgCommForSigTrace.dwTDDPresent = 1;*/
    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCellCfgReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
    
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::SendToRruCfgBplV25_Fdd()
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

    /*************************************************/
    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
    
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }
    /*************************************************/    

    tRnlcRruCellCfgReq.wOperateType = 0;
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tDbsGetSuperCpInfoAck.ucCPNum;
    tRnlcRruCellCfgReq.ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;

    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    pSuperCpInfo = tDbsGetSuperCpInfoAck.atCpInfo;

    for(BYTE ucCpLoop = 0; ucCpLoop <tRnlcRruCellCfgReq.ucCpNum && ucCpLoop<MAX_CP_OAM_PER_CELL;ucCpLoop++ )
    {
         ptRnlcCpCellCfgInfo->wCpId = pSuperCpInfo->ucCPId;
         ptRnlcCpCellCfgInfo->ucCellIdInBoard = pSuperCpInfo->ucBufferIdInBoard;
         ptRnlcCpCellCfgInfo->byRruRack = pSuperCpInfo->ucRruRackNo;
         ptRnlcCpCellCfgInfo->byRruShelf = pSuperCpInfo->ucRruShelfNo;
         ptRnlcCpCellCfgInfo->byRruSlot =  pSuperCpInfo->ucRruSlotNo;

        for ( WORD32 dwLoop = 0; dwLoop < MAX_RFCHAN_NUM; dwLoop++)
        {
            if ( (1 << dwLoop) == (pSuperCpInfo->ucDLAntMap & (1 << dwLoop)))
            {
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byChannelId
                    = (BYTE)dwLoop;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].byNum = 1;
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucCarNo  
                    = pSuperCpInfo->ucRRUCarrierNo;// 1 ʹ�� 0 ������
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucStatus = 1;
                ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum++;
            }
        }

        ptRnlcCpCellCfgInfo++;
        pSuperCpInfo++;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
                                             &tRnlcRruCellCfgReq, 
                                             sizeof(T_RnlcRruCellCfgReq), 
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);
    //�˴���Ϊ�¼�����ͬ�ṹ��ͬ��������Ҫ�����������
        //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));

    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCellCfgReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::SendToRruCfgBplV20_Fdd()
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

    
    /*************************************************/
    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
    
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }
    /*************************************************/    
    

    tRnlcRruCellCfgReq.wOperateType = 0;
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.ucCpNum = tDbsGetSrvcpInfoAck.ucCPNum;
    tRnlcRruCellCfgReq.ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;


    ptRnlcCpCellCfgInfo = tRnlcRruCellCfgReq.atRnlcCpCellCfgInfo;
    pCpInfo = tDbsGetSrvcpInfoAck.atCpInfo;

    for(BYTE ucCpLoop = 0; ucCpLoop <tRnlcRruCellCfgReq.ucCpNum && ucCpLoop<MAX_CP_OAM_PER_CELL;ucCpLoop++ )
    {
         ptRnlcCpCellCfgInfo->wCpId = pCpInfo->ucCPId;
         ptRnlcCpCellCfgInfo->ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
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
                ptRnlcCpCellCfgInfo->tChannelList.atChannelInfo[ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum].atCarStatus[0].ucStatus = 1;
                ptRnlcCpCellCfgInfo->tChannelList.dwChannelNum++;
            }
        }

        pCpInfo++;
        ptRnlcCpCellCfgInfo++;
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_RRU_CELL_CFG_REQ, 
                                             &tRnlcRruCellCfgReq, 
                                             sizeof(T_RnlcRruCellCfgReq), 
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
    
    CIMPrintSendMessage("EV_RNLC_RRU_CELL_CFG_REQ", EV_RNLC_RRU_CELL_CFG_REQ);
    //�˴���Ϊ�¼�����ͬ�ṹ��ͬ��������Ҫ�����������
            //ΪEV_RNLC_RRU_CELL_CFG_REQ �������FDD TDDͨ��
    T_RnlcRruCellCfgCommForSigTrace tRnlcRruCellCfgCommForSigTrace;
    memset(&tRnlcRruCellCfgCommForSigTrace,0,sizeof(T_RnlcRruCellCfgCommForSigTrace));
    
    memcpy(&tRnlcRruCellCfgCommForSigTrace.tRnlcRruCellCfgReq,
                &tRnlcRruCellCfgReq,
                sizeof(tRnlcRruCellCfgReq));

    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_RNLC_OUTER,
                     EV_RNLC_RRU_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_RNLC_SENT, 
                     sizeof(tRnlcRruCellCfgCommForSigTrace),
                     (const BYTE *)(&tRnlcRruCellCfgCommForSigTrace));
    return RNLC_SUCC;
}

WORD32 CCM_CIM_CellSetupComponentFsm::GetCpPwrFunction_tdd(T_RnlcRruCellCfgReq *ptRnlcRruCellCfgReq)
{
    /*lint -save -e653  -e834 -e734 -e747  -e732*/
    WORD16 awsubFreqNum[6] = {72,80,300,600,900,1200};
    FLOAT     alNumda[4][2] = {{1,5/4},{4/5,1},{3/5,3/4},{2/5,1/2}};
    FLOAT     alPAForDTCHPub[8] = {-6, -4.77, -3, -1.77, 0, 1, 2, 3};
    BOOL     bOamReturn = FALSE;
    WORD16  wPmaxCp = 0;
    BYTE     byCpPwr = 1;
    BOOLEAN bDbResult = FALSE;

    /*************************************************/
    T_DBS_GetSrvcelRecordByCellId_REQ tDBSGetSrvcelRecordByCellId_REQ;
    memset(&tDBSGetSrvcelRecordByCellId_REQ, 0, sizeof(tDBSGetSrvcelRecordByCellId_REQ));  
    tDBSGetSrvcelRecordByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetSrvcelRecordByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetSrvcelRecordByCellId_ACK tDBSGetSrvcelRecordByCellId_ACK;
    memset(&tDBSGetSrvcelRecordByCellId_ACK, 0, sizeof(tDBSGetSrvcelRecordByCellId_ACK));
    
    bDbResult = UsfDbsAccess(EV_DBS_GetSrvcelRecord_REQ,
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetSrvcelRecordByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetSrvcelRecordByCellId_ACK.dwResult))
    {        
        const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq,
                                tDBSGetSrvcelRecordByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }
    /*************************************************/

    /*************************************************/
    T_DBS_GetCellInfoByCellId_REQ tDBSGetCellInfoByCellId_REQ;
    memset(&tDBSGetCellInfoByCellId_REQ, 0, sizeof(tDBSGetCellInfoByCellId_REQ));  
    tDBSGetCellInfoByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetCellInfoByCellId_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetCellInfoByCellId_ACK tDBSGetCellInfoByCellId_ACK;
    memset(&tDBSGetCellInfoByCellId_ACK, 0, sizeof(tDBSGetCellInfoByCellId_ACK));
    
    bDbResult = UsfDbsAccess(EV_DBS_GetCellInfoByCellId_REQ,
                                     static_cast<VOID*>(&tDBSGetCellInfoByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetCellInfoByCellId_ACK));
    if ((!bDbResult) || (0 != tDBSGetCellInfoByCellId_ACK.dwResult))
    {        
        CCM_LOG(RNLC_ERROR_LEVEL, "[ULCOMP]CcmSetsSwitchForULCoMP  EV_DBS_GetCellInfoByCellId_REQ for ucULCOMPFlag fail.\n");
        return RNLC_FAIL;
    }
    /*************************************************/


    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));  
    
    /* ��ȡ���ݿ���Ϣ*/
    tGetSvrcpTddInfoReq.wCallType = USF_MSG_CALL;
    tGetSvrcpTddInfoReq.wCId   = GetJobContext()->tCIMVar.wCellId;
    bDbResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tGetSvrcpTddInfoReq, (VOID *)&tGetSvrcpTddInfoAck);
    if ((FALSE == bDbResult) || (0 != tGetSvrcpTddInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetSvrcpTddInfo, bDbResult,tGetSvrcpTddInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "[AdptBB]Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return RNLC_FAIL;
    }

    for(BYTE byCpLoop = 0; byCpLoop < tGetSvrcpTddInfoAck.ucCPNum; byCpLoop++)
    {
        if(tGetSvrcpTddInfoAck.atCpInfo[byCpLoop].ucCPId == 0)
        {
            continue;
        }

#if defined(_PRODUCT_TYPE) && (_PRODUCT_TYPE == _PRODUCT_LTE_TDD)
        bOamReturn = OamGetPmaxCp(GetJobContext()->tCIMVar.wCellId, tGetSvrcpTddInfoAck.atCpInfo[byCpLoop].ucCPId, &wPmaxCp);
#endif
       
        CCM_CIM_LOG(RNLC_INFO_LEVEL, "[CCM_CIM_TDD]GetCpPwrFunction_tdd call OamPmaxCp Ret %d,wPmaxCp %d,CPID %d! ", 
                                                           bOamReturn,
                                                           wPmaxCp,
                                                           tGetSvrcpTddInfoAck.atCpInfo[byCpLoop].ucCPId);
        
        if(FALSE == bOamReturn)
        {
            CCM_CIM_LOG(RNLC_FATAL_LEVEL, "[CCM_CIM_TDD]OamGetPmaxCp Fail %d! ", bOamReturn);
            return RNLC_FAIL;

        }
        else
        {
            /**************�������begin***************/
            if(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlSysBandWidth >5 ||
               tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB > 3 ||
               tDBSGetCellInfoByCellId_ACK.tCellAttr.tPowerInfo.ucPAForDTCHPub > 7)
            {
                CCM_CIM_LOG(RNLC_FATAL_LEVEL, "SysBandWith %d PB %d DTCHPub %d ", 
                                     tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlSysBandWidth,
                                     tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB,
                                     tDBSGetCellInfoByCellId_ACK.tCellAttr.tPowerInfo.ucPAForDTCHPub);

                return RNLC_FAIL;
            }
            /**************�������End***************/
        
            /*ptCmacParaDCfg->awMaxCellTransPwr[byCpPwr] = wPmaxCp;*/

            SWORD16  swReferenceSignalPower = 0;
            WORD16   wMaxCellTransPwr = 0;

            swReferenceSignalPower = (SWORD16)(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellSpeRefSigPwr - 600);
            wMaxCellTransPwr = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wMaxCellTransPwr;

            ptRnlcRruCellCfgReq->atRnlcCpCellCfgInfo[byCpLoop].wCellPower    = GetPCellRealPwr(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucCellRSPortNum,
                                                                                                                                          ((swReferenceSignalPower -wMaxCellTransPwr + wPmaxCp)/10),
                                                                                                                                            awsubFreqNum[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlSysBandWidth]/*TDD������ϵͳ������һ����*/,
                                                                                                                                          (tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucCellRSPortNum == 0)?alNumda[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB][0]:alNumda[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB][1],
                                                                                                                                            alPAForDTCHPub[tDBSGetCellInfoByCellId_ACK.tCellAttr.tPowerInfo.ucPAForDTCHPub]);

        }
        byCpPwr++;
    }

    /*lint -restore*/
    return RNLC_SUCC;
}



/*****************************************************************************/
/*                                ȫ�ֺ���                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                                �ֲ�����                                   */
/*****************************************************************************/

