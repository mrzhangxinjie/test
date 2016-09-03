/****************************************************************************
* 版权所有 (C)2013 北京比邻信通
*
* 文件名称: ccm_cim_cellsetupcomponent.cpp
* 文件标识:
* 内容摘要: 小区建立构件实现文件，主要实现小区建立过程中CIM模块与外部子系统的交互
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
/*                                常量定义                                   */
/*****************************************************************************/
/* 小区建立构件内部定时器定义 */
const WORD16 TIMER_PRMCELLCFG_DURATION  = 10000;
const WORD16 TIMER_CIMBBPARACFG_DURATION = 10000;
const WORD16 TIMER_RRUCELLCFG_DURATION = 25000;
const WORD16 TIMER_RNLUCELLCFG_DURATION = 1000000;
const WORD16 TIMER_BBCELLCFG_DURATION = 10000;
const WORD16 TIMER_RRMCELLCFG_DURATION = 10000;
const WORD16 TIMER_SIUPD_DURATION = 10000;

/* 小区建立构件内部状态定义*/
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
/*                            文件内部使用的宏                               */
/*****************************************************************************/ 


/*****************************************************************************/
/*                         文件内部使用的数据类型                            */
/*****************************************************************************/


/*****************************************************************************/
/*                                 全局变量                                  */
/*****************************************************************************/
/* 这个全局变量是用来调试外部子系统的开关，包括RRM, RNLU, RRU, BB
   默认设置为全1，即所有外部系统都存在 */
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

/*****************************************************************************/
/*                                状态函数                                   */
/*****************************************************************************/

/*****************************************************************************
* 函数名称: CCM_CIM_CellSetupComponentFsm::Init
* 功能描述: 每个构件建立时运行的初始化函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: SSL要求编写的函数。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Idle
* 功能描述: 小区CIM主控构件Idle状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitBBParaCfgRsp
* 功能描述: 小区CIM主控构件WaitBBParaCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitRRUCfgRsp
* 功能描述: 小区CIM主控构件WaitRRUCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitRNLUCfgRsp
* 功能描述: 小区CIM主控构件WaitRNLUCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitBBCfgRsp
* 功能描述: 小区CIM主控构件WaitBBCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitRRMCfgRsp
* 功能描述: 小区CIM主控构件WaitRRMCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::WaitSIUpdRsp
* 功能描述: 小区CIM主控构件WaitSIUpdRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SetupOK
* 功能描述: 小区CIM主控构件SetupOK状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 这个状态的增加，是为了表示，小区只能建立一遍，建立成功后该构件不再
*           接受其他的消息。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SetupError
* 功能描述: 小区CIM主控构件SetupError状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 这个状态的增加，是为了表示，小区只能建立一遍，建立成功后该构件不再
*           接受其他的消息。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SetupCancel
* 功能描述: 小区CIM主控构件SetupCancel状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 这个状态的增加，是为了表示，小区只能建立一遍，建立没有完成被取消后，
            该构件不再接受其他的消息。
**    
*    
* 版    本: V3.1
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_CIMCfgReq
* 功能描述: 小区建立构件处理CMD_CIM_CELL_CFG_REQ消息的函数
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
WORD32 CCM_CIM_CellSetupComponentFsm::Handle_CIMCfgReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellCfgReq), pMsg->m_wLen);
       
    const T_CmdCIMCellCfgReq *ptCmmCimCellCellCfgReq = 
                          static_cast<T_CmdCIMCellCfgReq*>(pMsg->m_pSignalPara);

    m_ucCellSetupReason = ptCmmCimCellCellCfgReq->ucCellOprReason;
    
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_REQ", pMsg->m_dwSignal);
    /*初始化小区建立实时信息*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_BBParaCfgRsp
* 功能描述: 小区建立构件处理CMD_CIM_CELL_BBPARA_CFG_RSP消息的函数
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
 
    /*增加小区建立状态跟踪*/
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
                          
         /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
            我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
            如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
             下类同*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_BBParaCfgRsp
* 功能描述: 小区建立构件处理EV_RRU_RNLC_CELL_CFG_RSP消息的函数
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
  
    /*增加小区建立状态跟踪*/
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
         
         /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
            我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
            如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
             下类同*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_RNLUCfgRsp
* 功能描述: 小区建立构件处理EV_RNLU_RNLC_CELL_CFG_RSP消息的函数
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
   
    /*增加小区建立状态跟踪*/
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
         
         /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
            我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
            如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
             下类同*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_BBCfgRsp
* 功能描述: 小区建立构件处理EV_BB_RNLC_CELL_CFG_RSP消息的函数
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
    /*增加小区建立状态跟踪*/
    CimAddCellSetupInfo(CELLSETUP_MODULE_BBCFG,ptBbRnlcCellCfgRsp->dwResult,(T_CIMVar *)&(GetJobContext()->tCIMVar));
#ifdef VS2008
    ptBbRnlcCellCfgRsp->dwResult = 0;
#endif
    if (0 != ptBbRnlcCellCfgRsp->dwResult)
    {
        RnlcLogTrace(PRINT_RNLC_CCM,  //所属模块号 
                     __FILE__,        //文件名号
                     __LINE__,        //文件行号
                     GetJobContext()->tCIMVar.wCellId,  //小区标识 
                     RNLC_INVALID_WORD,  //UE GID 
                     ERR_CCM_CIM_RNLUCELLCFG_FAIL,  //异常探针错误码 
                     GetJobContext()->tCIMVar.wInstanceNo,  //异常探针辅助码
                     ptBbRnlcCellCfgRsp->dwResult,  //异常探针扩展辅助码
                     RNLC_FATAL_LEVEL,  //打印级别
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
        
         /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
            我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
            如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
             下类同*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_RRMCfgRsp
* 功能描述: 小区建立构件处理EV_RRM_RNLC_CELL_CFG_RSP消息的函数
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
  
    /*增加小区建立状态跟踪*/
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
             
        /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
        我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
        如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
         下类同*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_SIUpdRsp
* 功能描述: 小区建立构件处理EV_RRM_RNLC_CELL_CFG_RSP消息的函数
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
   
    /*增加小区建立状态跟踪*/
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

        /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPERROR
        我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
        如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
         下类同*/
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

    /* 这个地方不销毁构件而是仅将状态转移为S_CIMCELLSETUP_SETUPOK
    我的出发点是认为小区建立构件仅能运行一次，如果运行成功了，ok.
    如果运行失败了，也要等待小区删除构件重启整个进程后，再运行下次
     下类同*/
    TranStateWithTag(CCM_CIM_CellSetupComponentFsm, 
                     SetupOK, 
                     S_CIM_SETUPCOM_SETUPOK);
    CIMPrintTranState("S_CIM_SETUPCOM_WAIT_SETUPOK", 
                       S_CIM_SETUPCOM_SETUPOK);

 
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_CfgCancelReq
* 功能描述: 小区建立构件处理CMD_CIM_CELL_CFG_CANCEL_REQ消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 
**    
* 完成日期: 2011年11月
* 版    本: V3
* 修改记录: 
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_TimeOut
* 功能描述: 小区建立构件处理外部系统响应超时的函数，因为每个系统超时的处理方式一样，
*           所以仅有一个处理超时的函数。
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::Handle_ErrorMsg
* 功能描述: 小区在某一状态下受到错误的消息，现在的处理方式是仅上报异常，
*           不做别的处理。
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::InitComponentContext
* 功能描述: 初始化小区建立构件数据区。
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot
* 功能描述: 判断是否发送小区建立请求消息给RRU代理-OAM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot(VOID)
{
    WORD32  dwResult = RNLC_FAIL;
    // 正常情况，发送RRU小区建立请求; 只有TDD制式，才有这条消息
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReqOrNot
* 功能描述: 决定是否发送小区建立请求消息给RNLU
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReqOrNot(VOID)
{
    //正常情况下，发送用户面小区建立请求
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendCfgReqToBBOrNot
* 功能描述: 决定是否发送小区建立请求消息给BB
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCfgReqToBBOrNot(VOID)
{
    //正常情况下，发送小区建立请求给基带
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
    // 如果不调试BB,不用给BB发送配置请求，但需要构造BB小区配置响应发给自己
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReqOrNot
* 功能描述: 决定是否发送小区建立请求消息给RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReqOrNot(VOID)
{
    // 正常情况下，发送小区建立请求给RRM
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
    // 如果不调试RRM，不用给RRM发送配置请求。但需要构造RRM配置响应发给自己
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToPRMCfgReq
* 功能描述: 发送小区建立请求消息给PRM模块，现在不实现。
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToPRMCfgReq(VOID)
{
    /*nothing!*/
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendBBParaCfgReq
* 功能描述: 发送小区建立请求消息给基带算法参数配置构件。
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReqOrNot
* 功能描述: 判断是否发送小区建立请求消息给RRU代理-OAM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_TddV20(VOID)
{    
    T_RnlcRruCellCfgReq tRnlcRruCellCfgReq;
    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));   

    memset(&tRnlcRruCellCfgReq, 0, sizeof(tRnlcRruCellCfgReq));
#if 0
    /* 获取数据库信息*/
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

        /*新增接口，载波压缩配置*/
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucSampleRateModeCfg = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSampleRateCfg;
        /* 小区板内索引，0~5 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
         /* 0:FDD，1:TDD */ 
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucDuplexMode =  GetJobContext()->tCIMVar.ucRadioMode;     
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].wCellPower = 
                                                            tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellTransPwr;
        /* 小区天线组 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucAntGroup = tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucAntSetNo;
        /* 小区载波数/频点数，目前仅支持单载波，目前固定为1 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].ucCarrNum = 1;
        /* 载波号 */  /*OAM不需要，暂时不修改，不算打桩 */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].ucCarrNo = 1;
        /* 中心载频EARFCN ，给rru的频点需要转换之前的值*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*获取频段指示*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* 载波带宽，枚举值，{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* 上下行子帧配置，枚举值，{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* 特殊子帧配置，枚举值，{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        tRnlcRruCellCfgReq.atRnlcRruCellCfgInfo[ucCpLoop].atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* 配置的生效系统子帧号，为10ms帧，0~1023 */
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


    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
* 函数名称: SendToRRUCfgReq_FddV20
* 功能描述: 发送小区建立消息到-OAM----FDD
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_FddV20(VOID)
{
    TRnlcRruCommResCfgReq tRnlcRruCommResCfgReq;


    memset(&tRnlcRruCommResCfgReq, 0x00, sizeof(tRnlcRruCommResCfgReq));
#if 0
    /* 根据rru信息填充架框槽 */
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
        
        //获取rru信息
        tRnlcRruCommResCfgReq.wOperateType = 0;//0 标示建立
        tRnlcRruCommResCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;
        tRnlcRruCommResCfgReq.ucRruNum = 1;//暂时一个小区只支持一个rru
        tRnlcRruCommResCfgReq.tRRUInfo[0].byRack = tDbsGetRruInfoAck.ucRruRack;
        tRnlcRruCommResCfgReq.tRRUInfo[0].byShelf = tDbsGetRruInfoAck.ucRruShelf;
        tRnlcRruCommResCfgReq.tRRUInfo[0].bySlot = tDbsGetRruInfoAck.ucRruSlot;
    }

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
                tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.atChannelInfo[tRnlcRruCommResCfgReq.tRRUInfo[0].tChannelList.dwChannelNum].dwStatus
                    = 1;// 1 使用 0 不适用
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
    //此处因为事件号相同结构不同，所以需要区分信令跟踪


    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRNLUCfgReq
* 功能描述: 发送小区建立请求消息给RNLU
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
  
    /* 小区ID */
    tRnlcRnluCellCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;   
    /* 小区板内索引，0~5 */
    tRnlcRnluCellCfgReq.ucCellIdInBoard = GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 上下行子帧配置，枚举值，{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
    tRnlcRnluCellCfgReq.ucUlDlSlotAlloc = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlDlSlotAlloc;
    tRnlcRnluCellCfgReq.wTimeStamp = 0;

    /* 由于使用了v2.5的接口，这里直接从数据库获取plmn信息 */
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
            tRnlcRnluCellCfgReq.m.dwAddOrRecfgPlmnInfoPresent = 0;//如果获取失败，不带plmn，影响kpi
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

    /* BCCH配置信息 */
    tRnlcRnluCellCfgReq.tAddOrRecfgBcchInfo.wGid = 
                               GetJobContext()->tCIMVar.tChannelGidInfo.wBcchId;   

    /* PCCH配置消息 */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.wGid = 
                               GetJobContext()->tCIMVar.tChannelGidInfo.wPcchId;   
    /* 控制面从数据库中读取寻呼重发次数 */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.wPagingRepeatTime = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucPagRepeatTime;   
    /* 控制面从数据库中读取寻呼重发次数 */
    /* 取值如下从0开始往下推 fourT, twoT, oneT, halfT, quarterT, oneEightT, 
                                               onSixteenthT, oneThirtySecondT */
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.dwDrxLength = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucPagDrxCyc;    
    tRnlcRnluCellCfgReq.tAddOrRecfgPcchInfo.dwNb = 
                 GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPcchInfoForV25.ucnB;

    /* ULCCCH配置消息 */
    tRnlcRnluCellCfgReq.tAddOrRecfgUlCcchInfo.wGid = 
                             GetJobContext()->tCIMVar.tChannelGidInfo.wUlCcchId;
    /* DLCCCH配置消息 */
    tRnlcRnluCellCfgReq.tAddOrRecfgDlCcchInfo.wGid = 
                             GetJobContext()->tCIMVar.tChannelGidInfo.wDlCcchId;

    /*新增dwSwchUserInac字段从数据库中获取*/
    /* ENodeB参数配置读取 */
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

        tRnlcRnluCellCfgReq.ucSwitchUserInac = 1; /* 已跟基带确认，获取失败直接赋值为1 */
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
    
    /* 获取信息 */
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

        tRnlcRnluCellCfgReq.ucTcpOrderEnable = 0;   /*0表示开关关闭*/
        tRnlcRnluCellCfgReq.ucTcpOrderTimerLen = 2;   /*默认初始值为2*/
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendCfgReqToBB
* 功能描述: 发送小区建立请求消息给BB
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendCfgReqToBB(VOID)
{     
    T_RnlcBbCellCfgReq tRnlcBbCellCfgReq;
    memset(&tRnlcBbCellCfgReq, 0, sizeof(tRnlcBbCellCfgReq));

    /*****************************BB请求头*************************************/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tRnlcBbCellCfgReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbCellCfgReq);
    /* 消息类型 */
    tRnlcBbCellCfgReq.tMsgHeader.wMsgType = EV_RNLC_BB_CELL_CFG_REQ;  
    /* 流水号 */
    tRnlcBbCellCfgReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    /* 小区ID */
    tRnlcBbCellCfgReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* 小区板内索引 */
    tRnlcBbCellCfgReq.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
    tRnlcBbCellCfgReq.tMsgHeader.wTimeStamp = 0;

    /****************************BB请求内容************************************/
    /* 物理小区ID */
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
    /* CP的长度 */
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
        /*上行天行能力*/
        /*tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 
                                  GetJobContext()->tCIMVar.tRRUInfo.ucUlAntCapacity;*/ 
        tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 1;
        /* 下行天线端口数 */
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntPortNum = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucCellRSPortNum;
#else
        tRnlcBbCellCfgReq.tCellInfo.ucUlAntCapacity = 1;  
        tRnlcBbCellCfgReq.tCellInfo.ucDlAntPortNum = 1;
#endif

      }
    /*****************4T8R End*******************/


    /* 上行带宽，TDD上下一致 */
    tRnlcBbCellCfgReq.tCellInfo.ucUlBandwidth = 
                         GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlSysBandWidth;
    /* 下行带宽，TDD上下一致 */
    tRnlcBbCellCfgReq.tCellInfo.ucDlBandwidth = 
                          GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucDlSysBandWidth;


    /* 0:FDD，1:TDD */
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
    
    /* 上下行时隙分配 */
    tRnlcBbCellCfgReq.tCellInfo.ucUlDlSwitchCfg =         
                               GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUlDlSlotAlloc; 
    /* 特殊子帧配置，枚举值，{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,ssp5,ssp6,ssp7,ssp8} */
    tRnlcBbCellCfgReq.tCellInfo.ucSpecialSubrameCfg =     
                      GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucSpecSubFramePat;     
    /* 小区半径 单位：10m  */
    tRnlcBbCellCfgReq.tCellInfo.wCellRadius = 
                                   GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.wCellRadius;
    /* C-RNTI和Temp C-RNTI的分界点，MAC使用分界前的，不包括分界点 */
    tRnlcBbCellCfgReq.tCellInfo.wEdgeofCrnti = 
                        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgGlobalInfoForV25.wCrntiTCrntiEdge;
    /* 小区MBMSFN属性 */
    tRnlcBbCellCfgReq.tCellInfo.ucCellMbmsAttri =
                              GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucCellMbsfnAtt; 

    tRnlcBbCellCfgReq.tCellInfo.ucOnlySendUCINum = 
                      GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucOnlySendUCINum; 
    /* 下行天线使能个数 */
    /*支持超级小区*/
    T_DBS_GetSvrcpInfo_REQ tGetSvrcpTddInfoReq;
    T_DBS_GetSvrcpInfo_ACK tGetSvrcpTddInfoAck;

    memset((VOID*)&tGetSvrcpTddInfoReq,  0, sizeof(T_DBS_GetSvrcpInfo_REQ));
    memset((VOID*)&tGetSvrcpTddInfoAck,  0, sizeof(T_DBS_GetSvrcpInfo_ACK));   

    /*获取实例数据区*/
    T_CIMVar *ptCIMVar  = (T_CIMVar*)&(GetJobContext()->tCIMVar);

    /* 获取数据库信息*/
    tGetSvrcpTddInfoReq.wCallType = USF_MSG_CALL;
    tGetSvrcpTddInfoReq.wCId   = ptCIMVar->wCellId;
    tGetSvrcpTddInfoReq.wBpType =1;/*表示BPL1*/
#if 0 /* shijunqiang: 这些信息木有用，封掉 */
    BYTE ucResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, (VOID *)&tGetSvrcpTddInfoReq, (VOID *)&tGetSvrcpTddInfoAck);
    if ((FALSE == ucResult) || (0 != tGetSvrcpTddInfoAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSvrcpInfoReq, ucResult,tGetSvrcpTddInfoAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "[BBALG]Call EV_DBS_GetSvrcpInfo_REQ fail!\n");
        return FALSE;
    }
#endif
#if 0
    /* 当支持超级小区，最多6个CP，当普通小区，取第一个数组元素 */
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
        /* 转化为索引值 */
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
    /* 小区对多用户MIMO的支持属性 */
    tRnlcBbCellCfgReq.tCellInfo.ucMUMIMOSupport = 
                        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucMUMIMOSupport;
    /* 是否支持64QAM解调 */
    tRnlcBbCellCfgReq.tCellInfo.ucUlSupport64QAM =         
                     GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucUl64QamDemSpInd;

    /* 0代表自适应，CMAC上报TM建议值; 1：tm1,2：tm2,3：tm3,4：
                          tm4,5：tm5,6：tm6,7：tm7，8：tm8，此时CMAC不上报TM */
    tRnlcBbCellCfgReq.tCellInfo.ucTmAutoSwitch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucTDDUeTransMode;
    
    /* 时间对齐定时器，单位：10ms */
    tRnlcBbCellCfgReq.tCellInfo.ucTATimer =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucTimeAlignTimer;
    
    /*航线高铁新增*/
    tRnlcBbCellCfgReq.tCellInfo.ucSceneCfg =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucSceneCfg;

     /* Channel GID Info */
    tRnlcBbCellCfgReq.dwChannelGidInfoPresent = 1;
    tRnlcBbCellCfgReq.tChannelGidInfo = GetJobContext()->tCIMVar.tChannelGidInfo;

    /* Active Ue Info */
    tRnlcBbCellCfgReq.dwActiveUeInfoPresent = 1;
    /* 该值为单板能够处理的激活用户数告警门限 */
    tRnlcBbCellCfgReq.tActiveUeInfo.wActiveUEThreshold = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgGlobalInfoForV25.wActiveUEThreshold; 
    /* PHICH Info */
    tRnlcBbCellCfgReq.dwPHICHInfoPresent = 1;
    /* PHICH资源利用率 */
    tRnlcBbCellCfgReq.tPhichInfo.ucNg = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPdchInfoForV25.ucNg;
    /* 指示PHICH占的OFDM符号数 */
    tRnlcBbCellCfgReq.tPhichInfo.ucPhichDuration = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPdchInfoForV25.ucPhichDuration; 

    /* PUCCH Info */
    tRnlcBbCellCfgReq.dwPUCCHInfoPresent = 1;
    /* PUCC循环移位 */
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
    /* 静态配置的PUCCH Format1的信道条数，包括SR，半静态A/N，A/N Repetiton信道条数之和，这三个参数均由后台配置 */
    tRnlcBbCellCfgReq.tPucchInfo.wN_PUCCH1 = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchSemiAn +
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchSr + 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuchInfoForV25.wNPucchAckRep;
    /* PUSCH Info */
    tRnlcBbCellCfgReq.dwPUSCHInfoPresent = 1;
    /* PUCCH和PUSCH解调参考信号的组跳频使能指示 */  
    tRnlcBbCellCfgReq.tPuschInfo.ucGroupHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucGrpHopEnableInd;                        
    /* Sequence-hopping使能指示 */
    tRnlcBbCellCfgReq.tPuschInfo.ucSeqHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucSeqHopEnableInd;
    /* PUSCH跳频时系统带宽需要划分的子带数目 */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucNsb = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschNsb;                        
     /* PUSCH的跳频范围指示 */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucHopMode = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschFHpMode;
    /* 跳频有效带宽 */
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschRB = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschRB;
    /* 跳频的最大RB限制 */
    tRnlcBbCellCfgReq.tPuschInfo.ucUlHopRBLimt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucUlHopRBLimt;
    /* Group Hopping时PUSCH与PUCCH的序列偏移差，0~29 */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschGroupAssig = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschGroupAssig;

    /*PUCCH和PUSCH解调参考信号的序列跳使能指示导频序列跳频是否使能指示。 
    关联下面Hopping三个字段。当此字段为true时，下面三个hopping字段有效*/
    tRnlcBbCellCfgReq.tPuschInfo.ucPilotHopEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPilotHopEnable;
    
    /* 跳频比特信息 */ 
    tRnlcBbCellCfgReq.tPuschInfo.ucHoppingBitsInfo = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucHoppingBitsInfo;
    /* PUSCH跳频起始位置，0~98 */
    tRnlcBbCellCfgReq.tPuschInfo.ucPuschHopOffset = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucPuschhopOfst;
    /*小区中上行参考信号的有效偏移数目 PUSCH解调参考信号循环偏移值，0~7 */
    tRnlcBbCellCfgReq.tPuschInfo.ucCyclicShift = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucnDMRS1;

    /* RACH Info */
    tRnlcBbCellCfgReq.dwRACHInfoPresent = 1;
    /* 竞争接入前导个数 */   
    tRnlcBbCellCfgReq.tRachInfo.ucNumberOfRaPreambles = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucNumRAPreambles;
    /* GroupA前导个数 */
    tRnlcBbCellCfgReq.tRachInfo.ucSizeOfRaPreamblesGroupA = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucSizeRAGroupA;      
    /* 最大接入次数 */
    tRnlcBbCellCfgReq.tRachInfo.ucMaxPreambleTranCt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPreambleTxMax;          
    /* RACH响应的窗口大小，单位是TTI（ms）*/
    tRnlcBbCellCfgReq.tRachInfo.ucRachRspWndSize = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucRARspWinSize;
    /* 竞争解决定时器时长，单位：ms */
    tRnlcBbCellCfgReq.tRachInfo.ucMacConTentionTimer = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucMACContResTimer;
    /* MSG3的最大传输次数，1~8 */      
    tRnlcBbCellCfgReq.tRachInfo.ucMaxMsg3TranCt = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucMaxHARQMsg3Tx;             
     /* 对应前缀允许发送的无线帧号和子帧号，0~63除去30，46，60-62 */
    tRnlcBbCellCfgReq.tRachInfo.ucPrachCfgIndex = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPrachConfig;             
    /* 小区高速移动属性 */ 
    tRnlcBbCellCfgReq.tRachInfo.uchighSpeedFlag = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucCellHighSpdAtt;                                                
    /* 产生64个前缀序列的逻辑根序列的起始索引号，0~837 */
    tRnlcBbCellCfgReq.tRachInfo.wRootSeqIndex =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.wLogRtSeqStNum;               
    /* 循环移位参数，0~15 */  
    tRnlcBbCellCfgReq.tRachInfo.ucNcsCfg =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucNcs;                     
    /* 第一个可用的物理资源块索引，0~104 */
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
    /* SRS配置使能指示 */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsEnable = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsEnable;                
    /* SRS子帧配置 */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsSubframeCfg = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsSubFrameCfg;            
    /* SRS带宽配置 */
    tRnlcBbCellCfgReq.tSrsInfo.ucSrsBwCfg =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tSrsConfInfoForV25.ucSrsBWCfg;
    /* SRS和Ack/Nack是否可以同时发射 */
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
    /* SR发送周期,单位是TTI（ms）*/
    tRnlcBbCellCfgReq.tSRInfo.ucSRTransPeriod =         
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucSRITransPrd;
    /* SR最大传输次数 */
    tRnlcBbCellCfgReq.tSRInfo.ucSRMaxTransNum = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPuschInfoForV25.ucDsrTransMax;
#if 0 
    /* UL Info */
    tRnlcBbCellCfgReq.dwUlAntennaInfoPresent = 1;
    for (BYTE ucCpLoop=0;ucCpLoop<tGetSvrcpTddInfoAck.ucCPNum;ucCpLoop++)
    {
        //---转化实际激活--begin
        
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
        /* 转换为枚举 */
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
        //---转化实际激活--end
    
        //tRnlcBbCellCfgReq.tUlAntennaInfo.ucCellAntennaNum[ucCpLoop] = 
                            //tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucUpActAntBitmap;
                            //tGetSvrcpTddInfoAck.atCpInfo[ucCpLoop].ucUlEnabledAntNum;

        /* 每个字节指示有效天线号 */
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
    /* Msg3 闭环功控参数, 范围0~14，单位：无，步进2；默认取值为6,实际值=内存值-2 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucDeltaPreambleMsg3 = 
            GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucdtaPrmbMsg3;
    /* 初始preamble发射功率，0~210，单位：dB，步进2；默认取值为12,实际值=内存值-120 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucInitPreRcvPw = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPreInitPwr;
    /* 取值范围:enum{0,2 4,6},单位：dBm,默认值：2dBm */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPwRampingStep = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPwRampingStep;
    /* 取值范围: enum {3,4,5,6,7,8,10,20,50,100,200},默认值：10 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPreambleTransMax = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgPrachInfoForV25.ucPreambleTxMax;
    /* 动态调度P0NominalPusch范围，0~150,实际值=内存值-126,默认值46 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPuschDynamic =
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPusch1;              
    /* PUSCH在半静态调度授权方式下发送的数据所需要的小区名义功率，参数是作为计算
    PUSCH发射功率的一部分，用于体现不同小区的功率差异。取值范围是0~150，实际值=内存值-126 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPuschPersistent = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPusch0;              
    /* 小区级路损补偿系数,Enumerate{0 ,0.4, 0.5,0.6,0.7,0.8,0.9,1}，范围是索引值0~7，单位：无，默认取值为0.8对应的索引5 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucAlpha4PL = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucAlfa;              
    /*0~150，默认46, P0NominalPucch，实际值=内存值-126*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPoNominalPucch = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucP0NominalPucch;              
    /*0~63，单位dbm，默认值53, 允许的ue最大发射功率, ，实际值=内存值-30*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPMax = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucUePMax;
    /* 0-- PUSCH采用DCI Format 3格式进行功控;1-- PUSCH采用DCI Format 3A格式进行功控 */
    tRnlcBbCellCfgReq.tUlPCInfo.ucPuschDCIType = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPuschDCIType;
    /* 以PUCCH格式1a作为参考格式，每种PUCCH格式对应的传输格式调整量，小区特定，enumerate{enumerate{-2,0,2},
    enumerate{1,3,5},enumerate{-2,0,1,2},enumerate{-2,0,2},enumerate{-2,0,2}},缺省值{0,3,0,0,2} */
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
    /*0-- PUCCH采用DCI Format 3格式进行功控,1-- PUCCH采用DCI Format 3A格式进行功控*/
    tRnlcBbCellCfgReq.tUlPCInfo.ucPucchDCIType = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgUpcInfoForV25.ucPucchDCIType;
    
    /* DL PC Info */
    tRnlcBbCellCfgReq.dwDlPCInfoPresent = 1;

    /* 包含RS的PDSCH EPRE与不包含RS的PDSCH EPRE的比值索引，0~3 */ 
    tRnlcBbCellCfgReq.tDlPCInfo.ucPB = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.ucPB;
    
#if 0    
    /* 小区最大发射功率，单位：dBm，0~50 */ 
    tRnlcBbCellCfgReq.tDlPCInfo.wMaxCellTransPwr = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.wMaxCellTransPwr;          
    /* 小区发射功率，单位：dBm，0~50 */
    tRnlcBbCellCfgReq.tDlPCInfo.wCellTransPwr = 
        GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgDpcInfoForV25.wCellTransPwr;

    /* 小区参考信号功率，单位：dBm，-60~50 */ 
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
        RnlcLogTrace(PRINT_RNLC_CCM,  //所属模块号 
                     __FILE__,        //文件名号
                     __LINE__,        //文件行号
                     GetJobContext()->tCIMVar.wCellId,  //小区标识 
                     RNLC_INVALID_WORD,  //UE GID 
                     ERR_CCM_CIM_FunctionCallFail_USF_SendTo,  //异常探针错误码 
                     GetJobContext()->tCIMVar.wInstanceNo,  //异常探针辅助码
                     dwResult,  //异常探针扩展辅助码
                     RNLC_FATAL_LEVEL,  //打印级别
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReq
* 功能描述: 发送小区建立请求消息给RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRMCfgReq(VOID)
{
    T_RnlcRrmCellCfgReq tRnlcRrmCellCfgReq;
    memset(&tRnlcRrmCellCfgReq, 0, sizeof(tRnlcRrmCellCfgReq));
    
    /* 小区ID */
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendSIUpdReq
* 功能描述: 发送小区系统广播更新消息给系统广播构件
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendCellCfgRsp
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SendCfgCancelRspToCcmMain
* 功能描述: 发送小区建立取消响应消息给CIM主构件
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::SaveBBCfgRspPara
* 功能描述: 保存基带发送的小区建立相应中携带的信息，之后需要在小区建立请求中带给
*           RRM
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
WORD32 CCM_CIM_CellSetupComponentFsm::
SaveBBCfgRspPara(const T_BbRnlcCellCfgRsp *ptBbRnlcCellCfgRsp)
{
    CCM_NULL_POINTER_CHECK(ptBbRnlcCellCfgRsp);

    /* 小区初始建立时有效，取值为编码效率*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlModulationEfficiency =
        ptBbRnlcCellCfgRsp->wUlModulationEfficiency;      
    /* PUCCH + PRACH使用的PRB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlCommonUsedPRB =
        ptBbRnlcCellCfgRsp->wUlCommonUsedPRB;          
    /* 上行总的可用RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlTotalAvailRB = 
        ptBbRnlcCellCfgRsp->dwUlTotalAvailRB;          
    /* 上行GBR业务使用的RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlGbrUsedPRB =
        ptBbRnlcCellCfgRsp->wUlGbrUsedPRB;             
    /* 上行NGBR业务使用的RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wUlNgbrUsedPRB =
        ptBbRnlcCellCfgRsp->wUlNgbrUsedPRB;           
    /* 上行NGBR上报周期内的PBR的累计值，单位：kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlNgbrTotalPBR =
        ptBbRnlcCellCfgRsp->dwUlNgbrTotalPBR;          
    /* 上行NGBR上报周期内的吞吐量，单位：kBps */ 
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwUlNgbrThroghput =
        ptBbRnlcCellCfgRsp->dwUlNgbrThroghput;         
    /* 小区初始建立时有效，取值为调制编码效率*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlModulationEfficiency =
        ptBbRnlcCellCfgRsp->wDlModulationEfficiency;  
    /* BCCH+PCCH+Msg2+Msg4使用的RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlCommonUsedPRB =
        ptBbRnlcCellCfgRsp->wDlCommonUsedPRB;          
    /* 下行总的可用RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlTotalAvailRB =
        ptBbRnlcCellCfgRsp->dwDlTotalAvailRB;          
    /* 下行GBR业务使用的RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlGbrUsedPRB =
        ptBbRnlcCellCfgRsp->wDlGbrUsedPRB;             
    /* 下行NGBR业务使用的RB数，取值为统计RB数*100 */
    GetJobContext()->tCIMVar.tCellMacRbInfo.wDlNgbrUsedPRB =
        ptBbRnlcCellCfgRsp->wDlNgbrUsedPRB;            
    /* 下行NGBR上报周期内的PBR的累计值，单位：kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrTotalPBR =
        ptBbRnlcCellCfgRsp->dwDlNgbrTotalPBR;          
    /* 下行NGBR上报周期内的吞吐量，单位：kBps */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrThroghput =
        ptBbRnlcCellCfgRsp->dwDlNgbrThroghput;         
    /* 下行专用Gbr业务占用的功率*100 单位：mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlGbrUsedPower =
        ptBbRnlcCellCfgRsp->dwDlGbrUsedPower;          
    /* 下行专用Ngbr业务占用的功率*100 单位：mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlNgbrUsedPower =
        ptBbRnlcCellCfgRsp->dwDlNgbrUsedPower;         
    /* 公共信道专用的功率*100 单位：mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlCommonUsedPower =
        ptBbRnlcCellCfgRsp->dwDlCommonUsedPower;       
    /* 下行可用于业务调度的剩余功率值*100，单位：mw */
    GetJobContext()->tCIMVar.tCellMacRbInfo.dwDlRestAvailPower =
        ptBbRnlcCellCfgRsp->dwDlRestAvailPower;

    return RNLC_SUCC;
}



/*****************************************************************************
* 函数名称: CCM_CIM_CellSetupComponentFsm::CIMPrintTranState
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
WORD32 CCM_CIM_CellSetupComponentFsm::CIMPrintTranState(const void  *pTargetState, BYTE ucTargetState)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo:%d, CCM_CIM_CellSetupComponentFsm TranState To %s, StateID: %d ",
                wInstanceNo, pTargetState, ucTargetState);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_CellSetupComponentFsm::CIMPrintRecvMessage
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::CIMPrintSendMessage
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRUCfgRsp
* 功能描述: 发送RRU小区建立相应给自身构件，主要用于屏蔽RRU进行调试
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRUCfgRsp(VOID)
{
    T_RruRnlcCellCfgRsp tRruRnlcCellCfgRsp;
    memset(&tRruRnlcCellCfgRsp, 0, sizeof(tRruRnlcCellCfgRsp));
    
    /* 小区ID */
    tRruRnlcCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引，0~5 */
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRNLUCfgRsp
* 功能描述: 发送RNLU小区建立相应给自身构件，主要用于屏蔽RNLU进行调试
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRNLUCfgRsp(VOID)
{
    T_RnluRnlcCellCfgRsp tRnluRnlcCellCfgRsp;
    memset(&tRnluRnlcCellCfgRsp, 0, sizeof(tRnluRnlcCellCfgRsp));
    
    /* 小区ID */
    tRnluRnlcCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引，0~5 */
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfBBCfgRsp
* 功能描述: 发送BB小区建立相应给自身构件，主要用于屏蔽BB进行调试
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfBBCfgRsp(VOID)
{
    T_BbRnlcCellCfgRsp tBbRnlcCellCfgRsp;
    memset(&tBbRnlcCellCfgRsp, 0, sizeof(tBbRnlcCellCfgRsp));

    /*BB请求头*/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tBbRnlcCellCfgRsp.tMsgHeader.wMsgLen = sizeof(tBbRnlcCellCfgRsp);
    /* 消息类型 */
    tBbRnlcCellCfgRsp.tMsgHeader.wMsgType = EV_BB_RNLC_CELL_CFG_RSP;    

    /* 流水号 */
    tBbRnlcCellCfgRsp.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
    
    /* 小区ID */
    tBbRnlcCellCfgRsp.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;   
    /* 小区板内索引 */
    tBbRnlcCellCfgRsp.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
    tBbRnlcCellCfgRsp.tMsgHeader.wTimeStamp = 0;

    /* 小区初始建立时有效，取值为编码效率*100 */
    tBbRnlcCellCfgRsp.wUlModulationEfficiency = I_DONT_KNOW;      
    /* PUCCH + PRACH使用的PRB数*100 */
    tBbRnlcCellCfgRsp.wUlCommonUsedPRB = I_DONT_KNOW;          
    /* 上行总的可用RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.dwUlTotalAvailRB = I_DONT_KNOW;          
    /* 上行GBR业务使用的RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.wUlGbrUsedPRB = I_DONT_KNOW;             
    /* 上行NGBR业务使用的RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.wUlNgbrUsedPRB = I_DONT_KNOW;           
    /* 上行NGBR上报周期内的PBR的累计值，单位：kBps */
    tBbRnlcCellCfgRsp.dwUlNgbrTotalPBR = I_DONT_KNOW;          
    /* 上行NGBR上报周期内的吞吐量，单位：kBps */ 
    tBbRnlcCellCfgRsp.dwUlNgbrThroghput = I_DONT_KNOW;         
    /* 小区初始建立时有效，取值为调制编码效率*100 */
    tBbRnlcCellCfgRsp.wDlModulationEfficiency = I_DONT_KNOW;  
    /* BCCH+PCCH+Msg2+Msg4使用的RB数*100 */
    tBbRnlcCellCfgRsp.wDlCommonUsedPRB = I_DONT_KNOW;          
    /* 下行总的可用RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.dwDlTotalAvailRB = I_DONT_KNOW;          
    /* 下行GBR业务使用的RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.wDlGbrUsedPRB = I_DONT_KNOW;             
    /* 下行NGBR业务使用的RB数，取值为统计RB数*100 */
    tBbRnlcCellCfgRsp.wDlNgbrUsedPRB = I_DONT_KNOW;            
    /* 下行NGBR上报周期内的PBR的累计值，单位：kBps */
    tBbRnlcCellCfgRsp.dwDlNgbrTotalPBR = I_DONT_KNOW;          
    /* 下行NGBR上报周期内的吞吐量，单位：kBps */
    tBbRnlcCellCfgRsp.dwDlNgbrThroghput = I_DONT_KNOW;         
    /* 下行专用Gbr业务占用的功率*100 单位：mw */
    tBbRnlcCellCfgRsp.dwDlGbrUsedPower = I_DONT_KNOW;          
    /* 下行专用Ngbr业务占用的功率*100 单位：mw */
    tBbRnlcCellCfgRsp.dwDlNgbrUsedPower = I_DONT_KNOW;         
    /* 公共信道专用的功率*100 单位：mw */
    tBbRnlcCellCfgRsp.dwDlCommonUsedPower = I_DONT_KNOW;       
    /* 下行可用于业务调度的剩余功率值*100，单位：mw */
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
* 函数名称: CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRMCfgRsp
* 功能描述: 发送RRM小区建立相应给自身构件，主要用于屏蔽RRM进行调试
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
WORD32 CCM_CIM_CellSetupComponentFsm::DbgSendToSelfRRMCfgRsp(VOID)
{
    T_RrmRnlcCellCfgRsp tRrmRnlcCellCfgRsp;
    memset(&tRrmRnlcCellCfgRsp, 0, sizeof(tRrmRnlcCellCfgRsp));
    
    /* 小区ID */
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
            /*将主板构建的状态保存起来*/
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
    tRnlcOamCellOperReq.wActionInd = ptCmmCimPaOper->ucOperType;/*0 表示关闭 1 表示打开*/
      
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
    
    /* 调用数据库判断小区所属的基带板状态是否正常 */
     dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);

    if(wBoardType == USF_BPLBOARDTYPE_BPL1)
    {
        /*BPL1的接口配置*/
        dwResult = SendToRruCfgBplV25_Tdd();
    }
    else
    {
        /*BPL0的接口配置*/
        dwResult = SendToRruCfgBplV20_Tdd();
    }
    
    return RNLC_SUCC;
}
WORD32 CCM_CIM_CellSetupComponentFsm::SendToRRUCfgReq_Fdd(VOID)
{
    WORD32 dwResult = 0;
    WORD16 wBoardType = 0 ;
    
    /* 调用数据库判断小区所属的基带板状态是否正常 */
     dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);

    if(wBoardType == USF_BPLBOARDTYPE_BPL1)
    {
        /*BPL1的接口配置*/
        dwResult = SendToRruCfgBplV25_Fdd();
    }
    else
    {
        /*BPL0的接口配置*/
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
         /*打印错误*/
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


        /* 小区载波数/频点数，目前仅支持单载波，目前固定为1 */
        ptRnlcCpCellCfgInfo->ucCarrNum = 1;
        /* 载波号 */  /*OAM不需要，暂时不修改，不算打桩 */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].ucCarrNo = 1;
        /* 中心载频EARFCN ，给rru的频点需要转换之前的值*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*获取频段指示*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* 载波带宽，枚举值，{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* 上下行子帧配置，枚举值，{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* 特殊子帧配置，枚举值，{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* 配置的生效系统子帧号，为10ms帧，0~1023 */
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


    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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

    /* 获取数据库信息*/
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
        /* 异常上报 */
        
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

        /* 小区载波数/频点数，目前仅支持单载波，目前固定为1 */
        ptRnlcCpCellCfgInfo->ucCarrNum = 1;
        /* 载波号 */  /*OAM不需要，暂时不修改，不算打桩 */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].ucCarrNo = 1;
        /* 中心载频EARFCN ，给rru的频点需要转换之前的值*/   
        WORD32 dwCenterFreq = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.dwCenterFreq;
        BYTE  byFreqBandInd=0;/*获取频段指示*/
        CcmGetFreqBandIndFromCenterFreq(dwCenterFreq, &byFreqBandInd);
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCenterFreq = CcmCellCenterFreqConvert(byFreqBandInd, dwCenterFreq);
        
        /* 载波带宽，枚举值，{0,1,2,3,4,5} -> {1.4,3,5,10,15,20} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwCarrWidth = 
                                                                  GetRRUBandWidth(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSysBandWidth);
        /* 上下行子帧配置，枚举值，{0,1,2,3,4,5,6} -> {sa0.sa1,sa2,sa3,sa4,sa5,sa6} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwUlDlCfg = 
                                                          tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlDlSlotAlloc;
        /* 特殊子帧配置，枚举值，{0,1,2,3,4,5,6,7,8} -> {ssp0.ssp1,ssp2,ssp3,ssp4,
                                                             ssp5,ssp6,ssp7,ssp8} */
        ptRnlcCpCellCfgInfo->atCarrCfg[0].dwSpecSubframeCfg = 
                                                       tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucSpecSubFramePat;
        /* 配置的生效系统子帧号，为10ms帧，0~1023 */
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


    //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
                    = pSuperCpInfo->ucRRUCarrierNo;// 1 使用 0 不适用
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
    //此处因为事件号相同结构不同，所以需要区分信令跟踪
        //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
                    = pCpInfo->ucRRUCarrierNo;// 1 使用 0 不适用
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
    //此处因为事件号相同结构不同，所以需要区分信令跟踪
            //为EV_RNLC_RRU_CELL_CFG_REQ 信令解析FDD TDD通用
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
    
    /* 获取数据库信息*/
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
            /**************参数检查begin***************/
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
            /**************参数检查End***************/
        
            /*ptCmacParaDCfg->awMaxCellTransPwr[byCpPwr] = wPmaxCp;*/

            SWORD16  swReferenceSignalPower = 0;
            WORD16   wMaxCellTransPwr = 0;

            swReferenceSignalPower = (SWORD16)(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wCellSpeRefSigPwr - 600);
            wMaxCellTransPwr = tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.wMaxCellTransPwr;

            ptRnlcRruCellCfgReq->atRnlcCpCellCfgInfo[byCpLoop].wCellPower    = GetPCellRealPwr(tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucCellRSPortNum,
                                                                                                                                          ((swReferenceSignalPower -wMaxCellTransPwr + wPmaxCp)/10),
                                                                                                                                            awsubFreqNum[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucUlSysBandWidth]/*TDD上下行系统带宽是一样的*/,
                                                                                                                                          (tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucCellRSPortNum == 0)?alNumda[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB][0]:alNumda[tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucPB][1],
                                                                                                                                            alPAForDTCHPub[tDBSGetCellInfoByCellId_ACK.tCellAttr.tPowerInfo.ucPAForDTCHPub]);

        }
        byCpPwr++;
    }

    /*lint -restore*/
    return RNLC_SUCC;
}



/*****************************************************************************/
/*                                全局函数                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                                局部函数                                   */
/*****************************************************************************/

