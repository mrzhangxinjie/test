/****************************************************************************
* 版权所有 (C)2013 北京比邻信通
*
* 文件名称: ccm_cim_celldelcomponent.cpp
* 文件标识:
* 内容摘要: 小区删除构件实现文件
* 其它说明: 
* 当前版本: V3.0
**    
*    
*
* 修改记录1: 
*    
***************************************************************************/

/*****************************************************************************/
/*               #include（依次为标准库头文件、非标准库头文件）            */
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
/*                                常量定义                                   */
/*****************************************************************************/
/* 小区删除构件内部定时器定义 */
const WORD16 TIMER_RRMCELLDEL_DURATION = 65000;
const WORD16 TIMER_CELLDEL_DURATION = 10000;

/* 小区删除构件是否收到外部系统小区删除响应的标志 */
const BYTE NULL_REL_COMPLETE = 0;
const BYTE RRU_REL_COMPLETE  = 1;
const BYTE RNLU_REL_COMPLETE = 2;
const BYTE BB_REL_COMPLETE   = 4;
const BYTE BBALG_REL_COMPLETE = 8;
const BYTE ALL_REL_COMPLETE  = 15;


/* 小区删除构件内部状态标志 */
const BYTE S_CIM_DELCOM_IDLE = 0;
const BYTE S_CIM_DELCOM_WAIT_RRMDELRSP = 1;
const BYTE S_CIM_DELCOM_WAIT_DELRSP = 2;
const BYTE S_CIM_DELCOM_WAIT_DELOK = 3;

/*****************************************************************************/
/*                            文件内部使用的宏                               */
/*****************************************************************************/

/*****************************************************************************/
/*                         文件内部使用的数据类型                            */
/*****************************************************************************/


/*****************************************************************************/
/*                                 全局变量                                  */
/*****************************************************************************/
/* 该全局变量是用来控制调试的时候是否调试外部系统的标志 */
extern T_SystemDbgSwitch g_tSystemDbgSwitch;

/*****************************************************************************/
/*                        本地变量（即静态全局变量）                         */
/*****************************************************************************/


/*****************************************************************************/
/*                              局部函数原型                                 */
/*****************************************************************************/

/*****************************************************************************/
/*                                类的实现                                   */
/*****************************************************************************/

/*****************************************************************************
* 函数名称: CCM_CIM_CellDelComponentFsm::Idle
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::WaitRRMDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::WaitDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::DelOK
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Init
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_CIMDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_PRMDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_PRMDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_RRMDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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

         /* RRM 释放失败，也要继续走下面的释放流程，目前对删除小区失败的情况
            没有处理策略,仅记录下RRM释放失败，不删除构件，程序不返回。*/
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

    /* 增加给dcm释放消息 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_RRUDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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

        /* RRU 释放失败，也要继续走下面的释放流程，目前对删除小区失败的情况
        没有处理策略,仅记录下RRU释放失败，不删除构件，程序不返回。*/
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_RNLUDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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

        /* RNLU 释放失败，也要继续走下面的释放流程，目前对删除小区失败的情况
        没有处理策略,仅记录下RNLU释放失败，不删除构件，程序不返回。*/
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_BBDelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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

        /* BB 释放失败，也要继续走下面的释放流程，目前对删除小区失败的情况
        没有处理策略,仅记录下BB释放失败，不删除构件，程序不返回。*/
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_RRMTimeOut
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::Handle_RRMTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    RnlcLogTrace(PRINT_RNLC_CCM,  //所属模块号 
                 __FILE__,        //文件名号
                 __LINE__,        //文件行号
                 GetJobContext()->tCIMVar.wCellId,  //小区标识 
                 RNLC_INVALID_WORD,  //UE GID 
                 ERR_CCM_CIM_RRMREL_TIMEOUT,  //异常探针错误码 
                 GetJobContext()->tCIMVar.wInstanceNo,  //异常探针辅助码
                 pMsg->m_dwSignal,  //异常探针扩展辅助码
                 RNLC_FATAL_LEVEL,  //打印级别
                 "\n CCM_CIM_CellDelComponentFsm RRM Del TimeOut! \n");
    CIMPrintRecvMessage("TimeOut Msg", pMsg->m_dwSignal);

    /* RRM 释放超时，也要继续走下面的释放流程，目前对删除小区失败的情况
    没有处理策略,仅记录下RRM释放失败，不删除构件，程序不返回。*/
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
    
    /* 某一外围设备释放超时，也要继续走下面的释放流程，目前对删除小区失败的情况
    没有处理策略,仅记录下释放失败，不删除构件，程序不返回。*/
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

    /* 下面这种情况是，跑进了超时这个分支，却发现接收到了每个外围系统的响应
        这种情况理论上是不可能发生的，还是写到这里，打印出来看一下吧! */
    if (ALL_REL_COMPLETE == GetCompleteSystemFlg(ALL_REL_COMPLETE, 
                     GetComponentContext()->tCIMCellDelVar.ucCompleteSystemFlg)) 
    {
         CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLDEL_STATE_ERROR,
                                 GetJobContext()->tCIMVar.wInstanceNo,
                                 pMsg->m_dwSignal,
                                 RNLC_FATAL_LEVEL, 
                                 " CCM_CIM_CellDelComponentFsm STATE_ERROR! ");
    }

    /*定时器超时后，给CMM回复响应 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::Handle_ErrorMsg
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::InitComponentContext
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRRUDelReqOrNot
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReqOrNot(VOID)
{
    WORD32 dwResult = RNLC_SUCC;
    
    // 正常情况，发送RRU小区删除请求; 只有TDD制式，才有这条消息
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
    // 如果不调试RRU，或者FDD制式，不用给RRU发小区删除请求, 但需要自己构造响应
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRNLUDelReqOrNot
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRNLUDelReqOrNot(VOID)
{
    //正常情况下，发送用户面小区删除请求
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
    // 如果不调试用户面，不用给用户面发重配消息,但需要构造响应发送给自己
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToBBDelReqOrNot
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToBBDelReqOrNot(VOID)
{
    //正常情况下，发送小区建立请求给基带
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
    // 如果不调试BB,不用给BB发送配置请求，但需要构造BB小区配置响应发给自己
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRRMDelReqOrNot
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRMDelReqOrNot(BYTE byDelCause)
{
    // 正常情况下，发送小区建立请求给RRM
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
    // 如果不调试RRM，不用给RRM发送配置请求。但需要构造RRM配置响应发给自己
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToMainComDelRsp
* 功能描述: 发送小区建立相应消息给CIM主构件
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToPRMDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToPRMDelReq(VOID)
{
    /*nothing!*/
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRRUDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_TddV20(VOID)
{
    T_RnlcRruCellCfgReq            tRnlcRruCellCfgReq; 
    
    tRnlcRruCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCellCfgReq.ucRadioMode = GetJobContext()->tCIMVar.ucRadioMode;
    tRnlcRruCellCfgReq.wOperateType = 2; /*2 表示 Del */

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

    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRRUDelReq_FddV20
* 功能描述:
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID
* 其他说明:
**    
*    
* 版    本: V3.1
* 修改记录:
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
    
    //获取rru信息
    tRnlcRruCommResCfgReq.wOperateType = 2;//  2标示删除
    tRnlcRruCommResCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcRruCommResCfgReq.ucRruNum = 1;//暂时一个小区只支持一个rru
    tRnlcRruCommResCfgReq.tRRUInfo[0].byRack = tDbsGetRruInfoAck.ucRruRack;
    tRnlcRruCommResCfgReq.tRRUInfo[0].byShelf = tDbsGetRruInfoAck.ucRruShelf;
    tRnlcRruCommResCfgReq.tRRUInfo[0].bySlot = tDbsGetRruInfoAck.ucRruSlot;

    /* 根据srvcp表中得到下行天线位图*/
    {
        T_DBS_GetSvrcpInfo_REQ   tDbsGetSrvcpInfoReq;
        T_DBS_GetSvrcpInfo_ACK   tDbsGetSrvcpInfoAck;

        memset(&tDbsGetSrvcpInfoReq, 0x00, sizeof(tDbsGetSrvcpInfoReq));
        memset(&tDbsGetSrvcpInfoAck, 0x00, sizeof(tDbsGetSrvcpInfoAck));

        tDbsGetSrvcpInfoReq.wCallType  = USF_MSG_CALL;
        tDbsGetSrvcpInfoReq.wCId    = GetJobContext()->tCIMVar.wCellId;
        tDbsGetSrvcpInfoReq.wBpType    = 0;/* v2基带板 */
        bResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tDbsGetSrvcpInfoReq, (VOID *)&tDbsGetSrvcpInfoAck);
        if ((FALSE == bResult) || (0 != tDbsGetSrvcpInfoAck.dwResult))
        {
            CCM_CIM_ExceptionReport(ERR_CMM_CIM_ADPTBB_GetRruInfoByCellId, bResult,tDbsGetSrvcpInfoAck.dwResult,\
                                                        RNLC_FATAL_LEVEL, "Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
            return FALSE;
        }
        //根据天线位图得到天线通道数
        //目前认为fdd仅仅支持一个cp，一个rru
        for ( WORD32 dwLoop = 0; dwLoop < MAX_RFCHAN_NUM; dwLoop++)
        {
            if ( (1 << dwLoop) == (tDbsGetSrvcpInfoAck.atCpInfo[0].ucDLAntMap & (1 << dwLoop)))
            {
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].byChannelId
                    = (BYTE)dwLoop;
   #if 0
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].dwStatus
                    = 0;// 1 使用 0 不适用
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
    //此处因为事件号相同结构不同，所以需要区分信令跟踪

    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRNLUDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToRNLUDelReq(VOID)
{      
    T_RnlcRnluCellRelReq tRnlcRnluCellRelReq;
    memset(&tRnlcRnluCellRelReq, 0, sizeof(tRnlcRnluCellRelReq));

    /* 小区ID */
    tRnlcRnluCellRelReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引，0~5 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToBBDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::SendToBBDelReq(VOID)
{    
    T_RnlcBbCellRelReq  tRnlcBbCellRelReq;
    memset(&tRnlcBbCellRelReq, 0, sizeof(tRnlcBbCellRelReq));

    /*BB请求头*/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tRnlcBbCellRelReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellRelReq);
    /* 消息类型 */
    tRnlcBbCellRelReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_REL_REQ; 

    /* 流水号 */
    tRnlcBbCellRelReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* 小区ID */
    tRnlcBbCellRelReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引 */
    tRnlcBbCellRelReq.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::SendToRRMDelReq
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::CIMPrintRecvMessage
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::CIMPrintSendMessage
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::CIMPrintTranState
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRURelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRURelRsp(VOID)
{
    T_RruRnlcCellRelRsp tRruRnlcCellRelRsp;
    memset(&tRruRnlcCellRelRsp, 0, sizeof(tRruRnlcCellRelRsp));
    
    /* 小区ID */
    tRruRnlcCellRelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引，0~5 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRNLURelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRNLURelRsp(VOID)
{
    T_RnluRnlcCellRelRsp tRnluRnlcCellRelRsp;
    memset(&tRnluRnlcCellRelRsp, 0, sizeof(tRnluRnlcCellRelRsp));
    /* 小区ID */
    tRnluRnlcCellRelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引，0~5 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::DbgSendToSelfBBRelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfBBRelRsp(VOID)
{
    T_BbRnlcCellRelRsp tBbRnlcCellRelRsp;
    memset(&tBbRnlcCellRelRsp, 0, sizeof(tBbRnlcCellRelRsp));

    /*BB请求头*/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tBbRnlcCellRelRsp.tMsgHeader.wMsgLen = sizeof(tBbRnlcCellRelRsp);
    /* 消息类型 */
    tBbRnlcCellRelRsp.tMsgHeader.wMsgType = EV_BB_RNLC_CELL_REL_RSP;    

    /* 流水号 */
    tBbRnlcCellRelRsp.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* 小区ID */
    tBbRnlcCellRelRsp.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* 小区板内索引 */
    tBbRnlcCellRelRsp.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
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
* 函数名称: CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRMRelRsp
* 功能描述: 发送RRM小区重配响应给自身构件，主要用于屏蔽RRM进行调试
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_CellDelComponentFsm::DbgSendToSelfRRMRelRsp(VOID)
{
    T_RrmRnlcCellRelRsp tRrmRnlcCellRelRsp;
    memset(&tRrmRnlcCellRelRsp, 0, sizeof(tRrmRnlcCellRelRsp));
    /* 小区ID */
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
* 函数名称: SendCellDelToDcm
* 功能描述: 发送小区删除消息到dcm
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: SendCellDelToBbAlg
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
* 完成日期: 
* 版    本: V3.1
* 修改记录: 
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
          
              /*BB请求头*/
              /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
              tRnlcBbCellRelReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellRelReq);
              /* 消息类型 */
              tRnlcBbCellRelReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_ALGPARA_CFG_REQ; 
          
              /* 流水号 */
              tRnlcBbCellRelReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
              
              /* 小区ID */
              tRnlcBbCellRelReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
              /* 小区板内索引 */
              tRnlcBbCellRelReq.tMsgHeader.ucCellIdInBoard = 
                                                 GetJobContext()->tCIMVar.ucCellIdInBoard;
              /* 时间戳 */
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
          // 如果不调试BB,不用给BB发送配置请求，但需要构造BB小区配置响应发给自己
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
* 函数名称: Handle_BBDelAlgRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
* 完成日期: 
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: DbgSendToSelfBBAlgRelRsp
* 功能描述: 
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
* 完成日期: 
* 版    本: V3.1
* 修改记录: 
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
            /*将主板构建的状态保存起来*/
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
    
    /* 调用数据库判断小区所属的基带板状态是否正常 */
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
         /*打印错误*/
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

    tRnlcRruCellCfgReq.wOperateType = 2; /* 2 表示 Del*/
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
                    = pSuperCpInfo->ucRRUCarrierNo;// 1 使用 0 不适用
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

        //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
    tDbsGetSrvcpInfoReq.wBpType    = 0;/* v2基带板 */
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

    tRnlcRruCellCfgReq.wOperateType = 2; /* 2 表示删除  */
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
                    = pCpInfo->ucRRUCarrierNo;// 1 使用 0 不适用
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

        //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
/*                                全局函数                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                                局部函数                                   */
/*****************************************************************************/
