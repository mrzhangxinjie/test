 /****************************************************************************
* 版权所有 (C)2013 北京比邻信通
*
* 文件名称: ccm_cim_maincomponent.cpp
* 文件标识:
* 内容摘要: 小区CIM主控构件，主要负责维护CIM中的建立、删除、重配的流程           
* 其它说明: 该构件主要为了实现小区删除的高优先级而设定的
* 当前版本: V3.0
**    
*    
*
* 修改记录1: 
*    
***************************************************************************/

/*****************************************************************************/
/*               #include（依次为标准库头文件、非标准库头文件）              */
/*****************************************************************************/
#ifdef VS2008
extern "C"{
#include "OSS_Sche.h"
}
#endif
#include "pub_lte_dbs.h"
#include "usf_bpl_pub.h"

#include "ccm_common.h"
#include "ccm_eventdef.h"
#include "ccm_timer.h"
#include "ccm_debug.h"
#include "ccm_common_struct.h"

#include "ccm_prm_pub_interface.h"

#include "ccm_cim_common.h"
#include "ccm_cim_maincomponent.h"
#include "Pub_lte_OAM_Interface.h"
/*****************************************************************************/
/*                                常量定义                                   */
/*****************************************************************************/
//CIM 主构件内部定时器时长常量定义
const WORD32 TIMER_CIMCELLSETUP_DURATION = 45000;
const WORD32 TIMER_CIMCELLDEL_DURATION = 75000;
const WORD32 TIMER_CIMCELLRECFG_DURATION = 30000;
const WORD32 TIMER_CIMCELLSETUPCANCEL_DURATION = 5000;
const WORD32 TIMER_CIMCELLRECFGCANCEL_DURATION = 5000;

//CIM 主构件内部状态常量定义
const BYTE S_CIM_MAINCOM_IDLE = 0;
const BYTE S_CIM_MAINCOM_WAITSETUPRSP = 1;
const BYTE S_CIM_MAINCOM_WAITDELRSP = 2;
const BYTE S_CIM_MAINCOM_WAITRECFGRSP = 3;
const BYTE S_CIM_MAINCOM_READY = 4;
const BYTE S_CIM_MAINCOM_WAITSETUPCANCELRSP = 5;
const BYTE S_CIM_MAINCOM_WAITRECFGCANCELRSP = 6;
const BYTE S_CIM_MAINCOM_WAITDELREQ = 7;
/*****************************************************************************/
/*                            文件内部使用的宏                               */
/*****************************************************************************/


/*****************************************************************************/
/*                         文件内部使用的数据类型                            */
/*****************************************************************************/


/*****************************************************************************/
/*                                 全局变量                                  */
/*****************************************************************************/

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

/*<FUNC>***********************************************************************
* 函数名称:PreFsm(Message *pMsg, BOOLEAN &bContinue)
* 功能描述:构建消息预处理函数
* 算法描述:无
* 全局变量:无
* Input参数:Message *pMsg:消息指针
* 返 回 值:BOOLEAN &bContinue :是否还需要继续转发消息到状态机
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
*  2011.06      1.0            创建
**<FUNC>**********************************************************************/
VOID CCM_CIM_MainComponentFsm::PreFsm(Message *pMsg, bool &bContinue)
{
    bContinue = TRUE;//继续走状态机

    if (NULL == pMsg)
    {
        CCM_LOG(DEBUG_LEVEL,"PreFsm pMsg is NULL.\n");

        //bContinue = FALSE;在这不做FALSE，因为有些消息参数为空
        return;
    }

    //TraceMe打印接收到的消息
    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_MainComponentFsm PreFsm receive msg:%lu.\n",pMsg->m_dwSignal);

    switch(pMsg->m_dwSignal)
    {
        case EV_CMM_CIM_DTX_IND:
        {
            HandleDtxNotifyFromCMM(pMsg);
            bContinue = FALSE;
            break;
        }

        case EV_RNLC_BB_RUSPTDTX_CFG_RSP:
        {
            Handle_EV_RNLC_BB_RUSPTDTX_CFG_RSP(pMsg);
            bContinue = FALSE;
            break;
        }

        case EV_BB_RNLC_SF_CTRL_REQ:  /*bb给发*/
        {
            Handle_EV_BB_RNLC_SF_CTRL_REQ(pMsg);
            bContinue = FALSE;
            break;
        }
        case EV_OAM_RNLC_SF_CTRL_IND:  /*oam给发*/
        {
            Handle_EV_OAM_RNLC_SF_CTRL_IND(pMsg);
            bContinue = FALSE;
            break;
        }
        default:
        {
            bContinue = TRUE;
            break;
        }
    }

    return;
}
/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Idle
* 功能描述: 小区CIM主控构件Idle状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::Idle(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_CMM_CIM_CELL_CFG_REQ:
        {
            Handle_CMMCfgReq(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitSetupRsp
* 功能描述: 小区CIM主控构件WaitSetupRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitSetupRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_CFG_RSP:
        {
            Handle_CellSetupComCfgRsp(pMsg);
            break;
        }
        case EV_CMM_CIM_CELL_REL_REQ:
        {
            Handle_CMMDelReqInSetup(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CELLSETUP_TIMEOUT:
        {
            Handle_CellSetupTimeOut(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitDelRsp
* 功能描述: 小区CIM主控构件WaitDelRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitDelRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_REL_RSP:
        {
            Handle_CellDelComDelRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CELLDEL_TIMEOUT:
        {
            Handle_CellDelTimeOut(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitReCfgRsp
* 功能描述: 小区CIM主控构件WaitReCfgRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitReCfgRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_RECFG_RSP:
        {
            Handle_CellReCfgComReCfgRsp(pMsg);
            break;
        }
        case EV_CMM_CIM_CELL_REL_REQ:
        {
            Handle_CMMDelReqInReCfg(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CELLRECFG_TIMEOUT:
        {
            Handle_CellReCfgTimeOut(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::Ready
* 功能描述: 小区CIM主控构件Ready状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::Ready(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_CMM_CIM_CELL_REL_REQ:
        {
            Handle_CMMRelReq(pMsg);
            break;
        }
        case EV_CMM_CIM_CELL_RECFG_REQ:
        {
            Handle_CMMReCfgReq(pMsg);
            break;
        }

       case CMD_CIM_MASTER_TO_SLAVE:\
       {\
           HandleMasterToSlave(pMsg);\
           break;\
       }
       case EV_CMM_CIM_CELL_ACBARINFO_REQ:\
       {
          HandleSendAcBarInfoForCpuRate(pMsg);\
          break;\
       }
       case EV_OAM_RNLC_SYMBOLMAP_RSP:
       {
           CCM_LOG(RNLC_INFO_LEVEL,"recieve  EV_OAM_RNLC_SYMBOLMAP_RSP sucess.\n");
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitSetupCancelRsp
* 功能描述: 小区CIM主控构件WaitSetupCancelRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitSetupCancelRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_CFG_CANCEL_RSP:
        {
            Handle_SetupCancelRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CELLSETUPCANCEL_TIMEOUT:
        {
            Handle_SetupCancelTimeOut(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitReCfgCancelRsp
* 功能描述: 小区CIM主控构件WaitReCfgCancelRsp状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitReCfgCancelRsp(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case CMD_CIM_CELL_CFG_CANCEL_RSP:
        {
            Handle_ReCfgCancelRsp(pMsg);
            break;
        }
        case EV_T_CCM_CIM_CELLRECFGCANCEL_TIMEOUT:
        {
            Handle_ReCfgCancelTimeOut(pMsg);
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
* 函数名称: CCM_CIM_MainComponentFsm::WaitDelReq
* 功能描述: 小区CIM主控构件WaitDelReq状态处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数, 该状态为小区建立失败，等待CMM发起删除的状态
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::WaitDelReq(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg);
    CCM_CIM_NULL_POINTER_CHECK_VOID(pMsg->m_pSignalPara);

    switch(pMsg->m_dwSignal)
    {
        case EV_CMM_CIM_CELL_REL_REQ:
        {
            Handle_CMMRelReq(pMsg);
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

/*****************************************************************************/
/*                                消息处理函数                               */
/*****************************************************************************/

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Init
* 功能描述: 小区CIM主控构件初始化函数
* 算法描述: 无
* 全局变量: 无
* Input参数: Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 平台框架要求填写的函数, 该构件初始化，或者该业务容器被重启时都会别执行
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
void CCM_CIM_MainComponentFsm::Init(VOID)
{
    InitJobContext();
    InitComponentContext();

    /* 建立其他构件 */
    CreateAll();

    BYTE  ucBoardMSState = BOARD_MS_UNCERTAIN;
   
    ucBoardMSState = OSS_GetBoardMSState();
    
    if (BOARD_SLAVE == ucBoardMSState)
    {
            CimPowerOnInSlave();
            SetComponentSlave();
            return;
    }

    TranStateWithTag(CCM_CIM_MainComponentFsm, Idle, S_CIM_MAINCOM_IDLE);
    CIMPrintTranState("S_CIM_MAINCOM_IDLE", S_CIM_MAINCOM_IDLE);
    
    return;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMCfgReq
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_CFG_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CMMCfgReq(const Message *pMsg)
{        
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmmCimCellCfgReq), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_CMM_CIM_CELL_CFG_REQ", pMsg->m_dwSignal);
        
    const T_CmmCimCellCfgReq *ptCmmCimCellCfgReq = 
                          static_cast<T_CmmCimCellCfgReq*>(pMsg->m_pSignalPara);
    WORD16 wBoardType;
    WORD32 dwResult ;
    BOOL      bResult = FALSE;

    T_PRMV25CellInfo tPRMV25CellInfo;

    memset(&tPRMV25CellInfo,0,sizeof(tPRMV25CellInfo));

    // 这个标志位暂时仅用于debug构件的输出判断，暂时个人认为不需要在这判断标志位
    // 该标志位的取消依赖于小区删除时的构件重启和构件数据区的初始化
    GetJobContext()->tCIMVar.bUsedFlg = TRUE;
    
    SaveCMMCellReqPara(ptCmmCimCellCfgReq);
    /***********Symbel begin*************/
    InitOamAntSymbel();
    /***********Symbel End*************/
     dwResult = USF_GetBplBoardTypeByCellId(ptCmmCimCellCfgReq->wCellId, &wBoardType);
    if (RNLC_SUCC != dwResult)
    {
        return RNLC_FAIL;
    }

    if( USF_BPLBOARDTYPE_BPL0 == wBoardType )
    {
        dwResult = GetSaveCellIdInBoard();
    }
    else
    {
        BYTE ucCellIndex = 0xFF;
        
        dwResult = PRM_V25RequestBPRes(ptCmmCimCellCfgReq->wCellId,&tPRMV25CellInfo,&ucCellIndex);    

        
        if(0xFF == ucCellIndex)
        {
            return RNLC_FAIL;
        }
        else
        {
             GetJobContext()->tCIMVar.ucCellIdInBoard = ucCellIndex;
        }

        /*向数据库中写入*/
        T_DBS_SetMasterBpInfo_REQ     tSetMasterBpInfoReq;
        T_DBS_SetMasterBpInfo_ACK     tSetMasterBpInfoAck;

        memset(&tSetMasterBpInfoReq,0x0,sizeof(T_DBS_SetMasterBpInfo_REQ));
        memset(&tSetMasterBpInfoAck,0x0,sizeof(T_DBS_SetMasterBpInfo_ACK));

            /* 调用数据库得到基带板信息 */
        tSetMasterBpInfoReq.wCallType = USF_MSG_CALL;
        tSetMasterBpInfoReq.wCellId   = ptCmmCimCellCfgReq->wCellId;

        tSetMasterBpInfoReq.ucCellRackNo = tPRMV25CellInfo.tMacResInfo.ucRack;
        tSetMasterBpInfoReq.ucCellShellNo = tPRMV25CellInfo.tMacResInfo.ucShelf;
        tSetMasterBpInfoReq.ucCellSlotNo = tPRMV25CellInfo.tMacResInfo.ucSlotNo;
        tSetMasterBpInfoReq.ucRnluCpuID = tPRMV25CellInfo.tMacResInfo.ucRnluCpuID;
        tSetMasterBpInfoReq.ucCmacCpuID = tPRMV25CellInfo.tMacResInfo.ucMacCpuID;

        bResult = UsfDbsAccess(EV_DBS_SetMasterBpInfo_REQ, &tSetMasterBpInfoReq, &tSetMasterBpInfoAck);

        if(TRUE != bResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            bResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_SetMasterBpInfo_REQ Fail In Handle_CMMCfgReq.\n");
        }

        /*SERCELB 写入*/
       dwResult = Ccm_cim_Main_SetCellInBoard(GetJobContext()->tCIMVar.wCellId, 
                                                                    GetJobContext()->tCIMVar.ucCellIdInBoard, 
                                                                    tPRMV25CellInfo.tMacResInfo.ucRnluCpuID, 
                                                                    tPRMV25CellInfo.tMacResInfo.ucMacCpuID);
        

        T_DBS_SetCpBuferIndex_REQ tSetCpBuferIndexReq;
        T_DBS_SetCpBuferIndex_ACK tSetCpBuferIndexAck;

        memset(&tSetCpBuferIndexReq,0x0,sizeof(T_DBS_SetCpBuferIndex_REQ));
        memset(&tSetCpBuferIndexAck,0x0,sizeof(T_DBS_SetCpBuferIndex_ACK)); 

        tSetCpBuferIndexReq.wCallType = USF_MSG_CALL;
        tSetCpBuferIndexReq.wCellId = ptCmmCimCellCfgReq->wCellId;
        tSetCpBuferIndexReq.ucCpNum = tPRMV25CellInfo.ucCpNum;

        for(BYTE ucBuferLoop = 0; ucBuferLoop < tPRMV25CellInfo.ucCpNum && ucBuferLoop < DBS_MAX_CP_NUM_PER_CID;ucBuferLoop++)
        {
            tSetCpBuferIndexReq.atSetCpInfo[ucBuferLoop].ucCPId = tPRMV25CellInfo.atCpResInfo[ucBuferLoop].ucCpId;
            tSetCpBuferIndexReq.atSetCpInfo[ucBuferLoop].ucBufferIdInBoard = tPRMV25CellInfo.atCpResInfo[ucBuferLoop].ucCellIdInBoard;  
        }
#if 0
        bResult = UsfDbsAccess(EV_DBS_SetCpBuferIndex_REQ, &tSetCpBuferIndexReq, &tSetCpBuferIndexAck);

        if(TRUE != bResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            bResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_CpBuferIndex_REQ Fail In Handle_CMMCfgReq.\n");
        }
#endif

    }


    /*WORD32 dwResult = GetSaveCellIdInBoard();*/


    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetSaveCellInBoard,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetSaveCellInBoard! ");
                                
      
        return ERR_CCM_CIM_FunctionCallFail_GetSaveCellInBoard;
    }

    //ulcomp---begin
    CCM_LOG(RNLC_DEBUG_LEVEL, "[ULCOMP]CcmUpdateAllCellIndexAndCmacCpuIdWithDevicedReCalc cell:[%d] \n",ptCmmCimCellCfgReq->wCellId);    

    //在此更新
    CcmUpdateAllCellIndexAndCmacCpuIdWithDevicedReCalc(ptCmmCimCellCfgReq->wCellId);
    //ulcomp---end

    
    dwResult = GetSaveBoardType();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetSaveBoardType,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetSaveBoardType! ");
        
        return ERR_CCM_CIM_FunctionCallFail_GetSaveBoardType;
    }

    dwResult = UpdateCellCfgInfobyDBS();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_UpdateCellCfgInfobyDBS,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_UpdateCellCfgInfobyDBS! ");

        return ERR_CCM_CIM_FunctionCallFail_UpdateCellCfgInfobyDBS;
    }
    
    dwResult = GetSaveAllPid();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetSaveAllPid,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetSaveAllPid! ");
        
        return ERR_CCM_CIM_FunctionCallFail_GetSaveAllPid;
    }
  
    dwResult = SendCellCfgReqToSetup(ptCmmCimCellCfgReq->ucCellOprReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendCellCfgReqToSetup,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendCellCfgReqToSetup! ");

        return ERR_CCM_CIM_FunctionCallFail_SendCellCfgReqToSetup;
    }

    WORD32 dwCIMCellSetupTimerId = 
                         USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLSETUP, 
                                                  TIMER_CIMCELLSETUP_DURATION, 
                                                  PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellSetupTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm SetRelativeTimer Failed! ");

        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellSetupTimerId = dwCIMCellSetupTimerId;

    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitSetupRsp, S_CIM_MAINCOM_WAITSETUPRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITSETUPRSP", S_CIM_MAINCOM_WAITSETUPRSP);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CellSetupComCfgRsp
* 功能描述: 小区CIM主控构件处理 CMD_CIM_CELL_CFG_RSP 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellSetupComCfgRsp(const Message *pMsg)
{    
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellCfgRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_RSP", pMsg->m_dwSignal);
   
    const T_CmdCIMCellCfgRsp *ptCmdCIMCellCfgRsp = 
                          static_cast<T_CmdCIMCellCfgRsp*>(pMsg->m_pSignalPara);
    WORD16 wBoardType = 0;
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->tCIMMainVar.\
                                                         dwCIMCellSetupTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, &wBoardType);
    if (RNLC_SUCC != dwResult)
    {
        CCM_LOG(DEBUG_LEVEL,"CCM_CIM_mainComponentFsm GetBplBoardType failed.\n");
    }
    
    dwResult = SendToCMMCfgRsp(ptCmdCIMCellCfgRsp->dwResult);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "FunctionCallFail_SendToCMMCfgRsp!");

        return ERR_CCM_CIM_FunctionCallFail_SendToCMMCfgRsp;
    }
    
    if (CCM_CELLSETUP_OK == ptCmdCIMCellCfgRsp->dwResult)
    {
        SendToOamSymbolMapReq();
        
        TranStateWithTag(CCM_CIM_MainComponentFsm, Ready, S_CIM_MAINCOM_READY);
        CIMPrintTranState("S_CIM_MAINCOM_READY", S_CIM_MAINCOM_READY);
    }
    else
    {       
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLSETUP_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCIMCellCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm CellSetup Fail! ");

        TranStateWithTag(CCM_CIM_MainComponentFsm, 
                         WaitDelReq, 
                         S_CIM_MAINCOM_WAITDELREQ);
        CIMPrintTranState("S_CIM_MAINCOM_WAITDELREQ", S_CIM_MAINCOM_WAITDELREQ);
    }
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CellSetupTimeOut
* 功能描述: 小区CIM主控构件处理 EV_T_CCM_CIM_CELLSETUP_TIMEOUT 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellSetupTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_MAINCOM_CELLSETUP_TIMEOUT,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            0,
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_MainComponentFsm CellSetup TimeOut! ");
    CIMPrintRecvMessage("EV_T_CCM_CIM_CELLSETUP_TIMEOUT", pMsg->m_dwSignal);

    WORD32 dwResult = SendToCMMCfgRsp(ERR_CCM_CIM_MAINCOM_CELLSETUP_TIMEOUT);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "FunctionCallFail_SendToCMMCfgRsp!");

        return ERR_CCM_CIM_FunctionCallFail_SendToCMMCfgRsp;
    }

    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelReq, S_CIM_MAINCOM_WAITDELREQ);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELREQ", S_CIM_MAINCOM_WAITDELREQ); 

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMDelReqInSetup
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_REL_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CMMDelReqInSetup(const Message *pMsg)
{    
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmmCimCellDelReq), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_CMM_CIM_CELL_REL_REQ", pMsg->m_dwSignal);

    const T_CmmCimCellDelReq *ptCmmCimCellDelReq = 
                      static_cast<T_CmmCimCellDelReq*>(pMsg->m_pSignalPara);

    CCM_CIM_CELLID_CHECK(ptCmmCimCellDelReq->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    m_ucCellDelReason = (BYTE)ptCmmCimCellDelReq->dwCause;
    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                    " CIM InstanceNo: %d,Handle_CMMDelReqInSetup Recvs m_ucCellDelReason = %u",
                    wInstanceNo, m_ucCellDelReason);  
    
    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->tCIMMainVar.\
                                                         dwCIMCellSetupTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm Kill Timer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    
    dwResult = SendToCellSetupComCancelReq();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellSetupComCancelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                "FunctionCallFail_SendToCellSetupComCancelReq!");

        return ERR_CCM_CIM_FunctionCallFail_SendToCellSetupComCancelReq;
    }
    WORD32 dwCIMCellSetupCancelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLSETUPCANCEL, 
                                                              TIMER_CIMCELLSETUPCANCEL_DURATION, 
                                                              PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellSetupCancelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellSetupCancelTimerId = 
                                                    dwCIMCellSetupCancelTimerId;
    
    CIMPrintTranState("S_CIM_MAINCOM_WAITSETUPCANCELRSP", 
                       S_CIM_MAINCOM_WAITSETUPCANCELRSP); 
    TranStateWithTag(CCM_CIM_MainComponentFsm, 
                     WaitSetupCancelRsp, 
                     S_CIM_MAINCOM_WAITSETUPCANCELRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_SetupCancelRsp
* 功能描述: 小区CIM主控构件处理 CMD_CIM_CELL_CFG_CANCEL_RSP 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_SetupCancelRsp(const Message *pMsg)
{    
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellCfgCancelRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_CANCEL_RSP", pMsg->m_dwSignal);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->tCIMMainVar.\
                                                   dwCIMCellSetupCancelTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    
    const T_CmdCIMCellCfgCancelRsp *ptCmdCIMCellCfgCancelRsp = 
                    static_cast<T_CmdCIMCellCfgCancelRsp*>(pMsg->m_pSignalPara);
    
    if (CCM_CELLSETUPCANCEL_OK != ptCmdCIMCellCfgCancelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLSETUPCANCEL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCIMCellCfgCancelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm SetupCancel Fail! ");
    }

    /* 无论是否能够取消掉建立流程，删除流程都正常进行 */
    dwResult = SendToCellDelComDelReq(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq;
    }
   
    WORD32 dwCIMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLDEL, 
                                                      TIMER_CIMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = dwCIMCellDelTimerId;

    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelRsp, S_CIM_MAINCOM_WAITDELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELRSP", S_CIM_MAINCOM_WAITDELRSP);
   
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_SetupCancelTimeOut
* 功能描述: 小区CIM主控构件处理 EV_T_CCM_CIM_CELLSETUPCANCEL_TIMEOUT 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_SetupCancelTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);    
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLSETUPCANCEL_TIMEOUT,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            GetTag(),
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_MainComponentFsm SetupCancel TimeOut! ");
    CIMPrintRecvMessage("EV_T_CCM_CIM_CELLSETUPCANCEL_TIMEOUT", pMsg->m_dwSignal);
    
    WORD32 dwResult = SendToCellDelComDelReq(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellDelComDelReq ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq;
    }

    WORD32 dwCIMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLDEL, 
                                                      TIMER_CIMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = dwCIMCellDelTimerId;
    
    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelRsp, S_CIM_MAINCOM_WAITDELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELRSP", S_CIM_MAINCOM_WAITDELRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMRelReq
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_REL_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CMMRelReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmmCimCellDelReq), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_CMM_CIM_CELL_REL_REQ", pMsg->m_dwSignal);
   
    const T_CmmCimCellDelReq *ptCmmCimCellDelReq = 
                          static_cast<T_CmmCimCellDelReq*>(pMsg->m_pSignalPara);

    CCM_CIM_CELLID_CHECK(ptCmmCimCellDelReq->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    m_ucCellDelReason = (BYTE)ptCmmCimCellDelReq->dwCause;
    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                    " CIM InstanceNo: %d,Handle_CMMRelReq Recvs m_ucCellDelReason = %u",
                    wInstanceNo, m_ucCellDelReason); 
    
    WORD32 dwResult = SendToCellDelComDelReq(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellDelComDelReq ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq;
    }

    WORD32 dwCIMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLDEL, 
                                                      TIMER_CIMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
       
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = dwCIMCellDelTimerId;
    
    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelRsp, S_CIM_MAINCOM_WAITDELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELRSP", S_CIM_MAINCOM_WAITDELRSP);
  
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMRelReq
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_REL_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellDelComDelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellDelRsp), pMsg->m_wLen);
    CIMPrintRecvMessage("CMD_CIM_CELL_REL_RSP", pMsg->m_dwSignal);

    WORD32 dwResult;
    BYTE      ucResult;
    const T_CmdCIMCellDelRsp *ptCmdCIMCellDelRsp = 
                          static_cast<T_CmdCIMCellDelRsp*>(pMsg->m_pSignalPara);

    T_PRMBpResInfo tPRMBpResInfo;
    if (USF_BPLBOARDTYPE_BPL0 == (GetJobContext()->tCIMVar.wBoardType))
    {
        dwResult = PRM_GetBPResInfoByCellId(GetJobContext()->tCIMVar.wCellId, &tPRMBpResInfo);
        if (CCM_PRM_NOFINDCELL != dwResult)
        {
            dwResult = PRM_ReleaseBPRes(GetJobContext()->tCIMVar.wCellId);
            if (RNLC_SUCC != dwResult)
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_PRM_ReleaseBPRes,
                                        GetJobContext()->tCIMVar.wInstanceNo,
                                        dwResult,
                                        RNLC_FATAL_LEVEL, 
                                        " FunctionCallFail_PRM_ReleaseBPRes! ");        
            }
        }
    }
    else
    {
     dwResult = PRM_ReleaseV25BPRes(GetJobContext()->tCIMVar.wCellId);
    }
    
    dwResult = CIMKillTimerByTimerId(GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
    }
    
    if (CCM_CELLDEL_OK != ptCmdCIMCellDelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLDEL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCIMCellDelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm CellDel Fail! ");
    }

    T_CellDelDetailResult tCellDelDetailResult;
    memset(&tCellDelDetailResult, 0, sizeof(tCellDelDetailResult));
    tCellDelDetailResult.dwResult = ptCmdCIMCellDelRsp->dwResult;
    tCellDelDetailResult.dwRRMResult = ptCmdCIMCellDelRsp->dwRRMResult;
    tCellDelDetailResult.dwRNLUResult = ptCmdCIMCellDelRsp->dwRNLUResult;
    tCellDelDetailResult.dwRRUResult = ptCmdCIMCellDelRsp->dwRRUResult;
    tCellDelDetailResult.dwBBResult = ptCmdCIMCellDelRsp->dwBBResult;
    
    dwResult = SendToCMMDelRsp(tCellDelDetailResult);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMDelRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCMMDelRsp! ");
    }
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                " CIM InstanceNo: %d, TEST CCM_CIM_Process job Reset! m_ucCellDelReason = %u",
                wInstanceNo, m_ucCellDelReason);    

    if(OPRREASON_DEL_ByTestRunMode != m_ucCellDelReason)
    {
        dwResult = Ccm_cim_Main_SetCellInBoard(GetJobContext()->tCIMVar.wCellId, 0xFFFF, 0xFF, 0xFF);
        if (RNLC_SUCC != dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            dwResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "Ccm_cim_Main_SetCellInBoard return Fail");
        }

        T_DBS_SetCpBuferIndex_REQ tSetCpBuferIndexReq;
        T_DBS_SetCpBuferIndex_ACK tSetCpBuferIndexAck;

        memset(&tSetCpBuferIndexReq,0x0,sizeof(T_DBS_SetCpBuferIndex_REQ));
        memset(&tSetCpBuferIndexAck,0x0,sizeof(T_DBS_SetCpBuferIndex_ACK));

        T_DBS_GetSuperCpInfo_REQ tDbsGetSuperCpInfoReq;
        T_DBS_GetSuperCpInfo_ACK tDbsGetSuperCpInfoAck;
        memset(&tDbsGetSuperCpInfoReq, 0x00, sizeof(tDbsGetSuperCpInfoReq));
        memset(&tDbsGetSuperCpInfoAck, 0x00, sizeof(tDbsGetSuperCpInfoAck));
        tDbsGetSuperCpInfoReq.wCallType = USF_MSG_CALL;
        tDbsGetSuperCpInfoReq.wCId = GetJobContext()->tCIMVar.wCellId;

        ucResult = UsfDbsAccess(EV_DBS_GetSuperCpInfo_REQ, (VOID *)&tDbsGetSuperCpInfoReq, (VOID *)&tDbsGetSuperCpInfoAck);
        if ((FALSE == ucResult) || (0 != tDbsGetSuperCpInfoAck.dwResult))
        {
            /*打印错误*/
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            ucResult, 
                            tDbsGetSuperCpInfoAck.dwResult,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_GetSuperCpInfo_REQ return Fail in Handle_CellDelComDelRsp");
        }
        else
        {
            /***************4T8R*****************/
            if(GetJobContext()->tCIMVar.ucCpMergeType)
            {
                CcmCommonCpMerge(GetJobContext()->tCIMVar.wCellId,(VOID *)&tDbsGetSuperCpInfoAck);
            }
            /************************************/
        
            tSetCpBuferIndexReq.wCallType = USF_MSG_CALL;
            tSetCpBuferIndexReq.wCellId = GetJobContext()->tCIMVar.wCellId;
            tSetCpBuferIndexReq.ucCpNum = tDbsGetSuperCpInfoAck.ucCPNum;
            for(BYTE iCpLoop = 0;iCpLoop < tDbsGetSuperCpInfoAck.ucCPNum && iCpLoop < DBS_MAX_CP_NUM_PER_CID;iCpLoop++)
            {
                tSetCpBuferIndexReq.atSetCpInfo[iCpLoop].ucCPId = tDbsGetSuperCpInfoAck.atCpInfo[iCpLoop].ucCPId;
                tSetCpBuferIndexReq.atSetCpInfo[iCpLoop].ucBufferIdInBoard = 0xFF;
            }

            ucResult = UsfDbsAccess(EV_DBS_SetCpBuferIndex_REQ, &tSetCpBuferIndexReq, &tSetCpBuferIndexAck);
            if (FALSE == ucResult)
            {
                /*打印错误*/
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            ucResult, 
                            tSetCpBuferIndexAck.dwResult,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_SetCpBuferIndex_REQ return Fail in Handle_CellDelComDelRsp");
            }
        }
    }

    /* 无论删除是否成功，都重启本线程，是否有问题?  */
    // 如果非DBS删除小区，不应该重启所有线程的，这个地方设计还是有点问题
    /********    ********/
    if ((OPRREASON_DEL_ByDBS == m_ucCellDelReason) ||
        (OPRREASON_DEL_ByAudit == m_ucCellDelReason))
    {
        dwResult = JobReset();
        if (SSL_OK != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_JobRest,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " CCM_CIM_MainComponentFsm JobReset() Fail! ");

            return ERR_CCM_CIM_FunctionCallFail_USF_JobRest;
        }
                
        CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                    " CIM InstanceNo: %d, CCM_CIM_Process job Reset! ",
                    wInstanceNo);
    }
    else
    {
        dwResult = ResetComponents();
        /*此处添加一个构件reset 失败的打印，方便问题定位，
    */
        /*lint -save -e666 */
        if(RNLC_SUCC != dwResult)
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                                    " CIM InstanceNo: %d, CCM_CIM_Process Components Reset fail! ",
                                    GetJobContext()->tCIMVar.wInstanceNo);    
        }
        else
        {
            CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                                    " CIM InstanceNo: %d, CCM_CIM_Process Components Reset! m_ucCellDelReason = %u",
                                    wInstanceNo, m_ucCellDelReason);  
        }
        /*lint -restore*/
    }
/**********************************************************************************************/
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CellDelTimeOut
* 功能描述: 小区CIM主控构件处理 EV_T_CCM_CIM_CELLDEL_TIMEOUT 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellDelTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLDEL_TIMEOUT,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            GetTag(),
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_MainComponentFsm CellDel TimeOut! ");
    WORD32 dwResult;
    BYTE     ucResult;
                            
    CIMPrintRecvMessage("EV_T_CCM_CIM_CELLDEL_TIMEOUT", pMsg->m_dwSignal);

    if (USF_BPLBOARDTYPE_BPL0 == (GetJobContext()->tCIMVar.wBoardType))
    {
        T_PRMBpResInfo tPRMBpResInfo;
        dwResult = PRM_GetBPResInfoByCellId(GetJobContext()->tCIMVar.wCellId, &tPRMBpResInfo);
        if (CCM_PRM_NOFINDCELL != dwResult)
        {
            dwResult = PRM_ReleaseBPRes(GetJobContext()->tCIMVar.wCellId);
            if (RNLC_SUCC != dwResult)
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_PRM_ReleaseBPRes,
                                        GetJobContext()->tCIMVar.wInstanceNo,
                                        dwResult,
                                        RNLC_FATAL_LEVEL, 
                                        " FunctionCallFail_PRM_ReleaseBPRes! ");        
            }
        }
    }
    else
    {
        dwResult = PRM_ReleaseV25BPRes(GetJobContext()->tCIMVar.wCellId);
    }



    
    // 小区删除超时，各个子原因值，就填为OK吧，否则也不知道应该怎么填。
    T_CellDelDetailResult tCellDelDetailResult;
    memset(&tCellDelDetailResult, 0, sizeof(tCellDelDetailResult));
    tCellDelDetailResult.dwResult = ERR_CCM_CIM_CELLDEL_TIMEOUT;
    tCellDelDetailResult.dwRRMResult = CCM_CELLDEL_OK;
    tCellDelDetailResult.dwRNLUResult = CCM_CELLDEL_OK;
    tCellDelDetailResult.dwRRUResult = CCM_CELLDEL_OK;
    tCellDelDetailResult.dwBBResult = CCM_CELLDEL_OK;

    dwResult = SendToCMMDelRsp(tCellDelDetailResult);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMDelRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCMMDelRsp! ");
    }
   
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                " CIM InstanceNo: %d, Handle_CellDelTimeOut m_ucCellDelReason = %u",
                wInstanceNo, m_ucCellDelReason);    
   
    if(OPRREASON_DEL_ByTestRunMode != m_ucCellDelReason)
    {
        dwResult = Ccm_cim_Main_SetCellInBoard(GetJobContext()->tCIMVar.wCellId, 0xFFFF, 0xFF, 0xFF);
        if (RNLC_SUCC != dwResult)
        {
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            dwResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "Ccm_cim_Main_SetCellInBoard return Fail");
        }

        T_DBS_SetCpBuferIndex_REQ tSetCpBuferIndexReq;
        T_DBS_SetCpBuferIndex_ACK tSetCpBuferIndexAck;

        memset(&tSetCpBuferIndexReq,0x0,sizeof(T_DBS_SetCpBuferIndex_REQ));
        memset(&tSetCpBuferIndexAck,0x0,sizeof(T_DBS_SetCpBuferIndex_ACK));

        T_DBS_GetSuperCpInfo_REQ tDbsGetSuperCpInfoReq;
        T_DBS_GetSuperCpInfo_ACK tDbsGetSuperCpInfoAck;
        memset(&tDbsGetSuperCpInfoReq, 0x00, sizeof(tDbsGetSuperCpInfoReq));
        memset(&tDbsGetSuperCpInfoAck, 0x00, sizeof(tDbsGetSuperCpInfoAck));
        tDbsGetSuperCpInfoReq.wCallType = USF_MSG_CALL;
        tDbsGetSuperCpInfoReq.wCId = GetJobContext()->tCIMVar.wCellId;

        ucResult = UsfDbsAccess(EV_DBS_GetSuperCpInfo_REQ, (VOID *)&tDbsGetSuperCpInfoReq, (VOID *)&tDbsGetSuperCpInfoAck);
        if ((FALSE == ucResult) || (0 != tDbsGetSuperCpInfoAck.dwResult))
        {
            /*打印错误*/
            CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            ucResult, 
                            tDbsGetSuperCpInfoAck.dwResult,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_GetSuperCpInfo_REQ return Fail in Handle_CellDelComDelRsp");
        }
        else
        {
            /***************4T8R*****************/
            if(GetJobContext()->tCIMVar.ucCpMergeType)
            {
                CcmCommonCpMerge(GetJobContext()->tCIMVar.wCellId,(VOID *)&tDbsGetSuperCpInfoAck);
            }
            /************************************/
            tSetCpBuferIndexReq.wCallType = USF_MSG_CALL;
            tSetCpBuferIndexReq.wCellId = GetJobContext()->tCIMVar.wCellId;
            tSetCpBuferIndexReq.ucCpNum = tDbsGetSuperCpInfoAck.ucCPNum;
            for(BYTE iCpLoop = 0;iCpLoop < tDbsGetSuperCpInfoAck.ucCPNum && iCpLoop < DBS_MAX_CP_NUM_PER_CID;iCpLoop++)
            {
                tSetCpBuferIndexReq.atSetCpInfo[iCpLoop].ucCPId = tDbsGetSuperCpInfoAck.atCpInfo[iCpLoop].ucCPId;
                tSetCpBuferIndexReq.atSetCpInfo[iCpLoop].ucBufferIdInBoard = 0xFF;
            }

            ucResult = UsfDbsAccess(EV_DBS_SetCpBuferIndex_REQ, &tSetCpBuferIndexReq, &tSetCpBuferIndexAck);
            if (FALSE == ucResult)
            {
                /*打印错误*/
                CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            ucResult, 
                            tSetCpBuferIndexAck.dwResult,
                            RNLC_ERROR_LEVEL, 
                            "EV_DBS_SetCpBuferIndex_REQ return Fail in Handle_CellDelComDelRsp");
            }
        }
    }


    /* 无论删除是否成功，都重启本线程，是否有问题?  */
    // 如果非DBS删除小区，不应该重启所有线程的，这个地方设计还是有点问题
    /********    ********/
    if ((OPRREASON_DEL_ByDBS == m_ucCellDelReason) ||
        (OPRREASON_DEL_ByAudit == m_ucCellDelReason))
    {
        dwResult = JobReset();
        if (SSL_OK != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_JobRest,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " CCM_CIM_MainComponentFsm JobReset() Fail! ");

            return ERR_CCM_CIM_FunctionCallFail_USF_JobRest;
        }
        
        CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                    " CIM InstanceNo: %d, CCM_CIM_Process job Reset! ",
                    wInstanceNo);
    }
    else
    {
        dwResult = ResetComponents();
        /*此处添加一个构件reset 失败的打印，方便问题定位，
    */
        if(RNLC_SUCC != dwResult)
        {
            /*lint -save -e666 */
            CCM_CIM_LOG(RNLC_INFO_LEVEL, 
                        " CIM InstanceNo: %d, CCM_CIM_Process Components Reset fail! ",
                        GetJobContext()->tCIMVar.wInstanceNo);    
        /*lint -restore*/
        }

    }
    /**********************************************************************************************/
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMReCfgReq
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_RECFG_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CMMReCfgReq(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmmCimCellReCfgReq), pMsg->m_wLen);
    CIMPrintRecvMessage("EV_CMM_CIM_CELL_RECFG_REQ", pMsg->m_dwSignal);
    
    const T_CmmCimCellReCfgReq *ptCmmCimCellReCfgReq = 
                        static_cast<T_CmmCimCellReCfgReq*>(pMsg->m_pSignalPara);

    CCM_CIM_CELLID_CHECK(ptCmmCimCellReCfgReq->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);

    WORD32 dwResult = UpdateCellCfgInfobyDBS();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_UpdateCellCfgInfobyDBS,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_UpdateCellCfgInfobyDBS! ");
        
        return ERR_CCM_CIM_FunctionCallFail_UpdateCellCfgInfobyDBS;
    }
    
    dwResult = SendToCellReCfgComReCfgReq(ptCmmCimCellReCfgReq->dwCellOprPara);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellReCfgComReCfgReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellReCfgComReCfgReq! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellReCfgComReCfgReq;
    }

    WORD32 dwCIMCellReCfgTimerId = 
                                USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLRECFG, 
                                                     TIMER_CIMCELLRECFG_DURATION, 
                                                     PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellReCfgTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
      
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.\
                      dwCIMCellReCfgTimerId = dwCIMCellReCfgTimerId;
    
    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitReCfgRsp, S_CIM_MAINCOM_WAITRECFGRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITRECFGRSP", S_CIM_MAINCOM_WAITRECFGRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CellReCfgComReCfgRsp
* 功能描述: 小区CIM主控构件处理 CMD_CIM_CELL_RECFG_RSP 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellReCfgComReCfgRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellReCfgRsp), pMsg->m_wLen);   
    CIMPrintRecvMessage("CMD_CIM_CELL_RECFG_RSP", pMsg->m_dwSignal);

    const T_CmdCIMCellReCfgRsp *ptCmdCIMCellReCfgRsp = 
                        static_cast<T_CmdCIMCellReCfgRsp*>(pMsg->m_pSignalPara);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()\
                                           ->tCIMMainVar.dwCIMCellReCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
        
    dwResult = SendToCMMReCfgRsp(ptCmdCIMCellReCfgRsp->dwResult);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMReCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCMMReCfgRsp! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCMMReCfgRsp;
    }

    SendToOamSymbolMapReq();
    if (CCM_CELLRECFG_OK != ptCmdCIMCellReCfgRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLRECFG_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCIMCellReCfgRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm Cell ReCfg Fail! ");
    }

    TranStateWithTag(CCM_CIM_MainComponentFsm, Ready, S_CIM_MAINCOM_READY);
    CIMPrintTranState("S_CIM_MAINCOM_READY", S_CIM_MAINCOM_READY); 

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CellReCfgTimeOut
* 功能描述: 小区CIM主控构件处理 EV_T_CCM_CIM_CELLRECFG_TIMEOUT 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CellReCfgTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLRECFG_FAIL,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            GetTag(),
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_MainComponentFsm CellReCfg TimerOut Fail! ");
    CIMPrintRecvMessage("EV_T_CCM_CIM_CELLRECFG_TIMEOUT", pMsg->m_dwSignal);
    
    WORD32 dwResult = SendToCMMReCfgRsp(ERR_CCM_CIM_CELLRECFG_TIMEOUT);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCMMReCfgRsp,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCMMReCfgRsp! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCMMReCfgRsp;
    }

    TranStateWithTag(CCM_CIM_MainComponentFsm, Ready, S_CIM_MAINCOM_READY);
    CIMPrintTranState("S_CIM_MAINCOM_READY", S_CIM_MAINCOM_READY);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_CMMDelReqInReCfg
* 功能描述: 小区CIM主控构件处理 EV_CMM_CIM_CELL_REL_REQ 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_CMMDelReqInReCfg(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmmCimCellDelReq), pMsg->m_wLen);   
    CIMPrintRecvMessage("EV_CMM_CIM_CELL_REL_REQ", pMsg->m_dwSignal);

    const T_CmmCimCellDelReq *ptCmmCimCellDelReq = 
                          static_cast<T_CmmCimCellDelReq*>(pMsg->m_pSignalPara);

    CCM_CIM_CELLID_CHECK(ptCmmCimCellDelReq->wCellId, 
                         GetJobContext()->tCIMVar.wCellId);
    
    m_ucCellDelReason = (BYTE)ptCmmCimCellDelReq->dwCause;

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->\
                                             tCIMMainVar.dwCIMCellReCfgTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
        
    dwResult = SendToCellReCfgComCancelReq();
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellReCfgComCancelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellReCfgComCancelReq! ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellReCfgComCancelReq;
    }
    
    WORD32 dwCIMCellReCfgCancelTimerId = 
                         USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLRECFGCANCEL, 
                                              TIMER_CIMCELLRECFGCANCEL_DURATION, 
                                              PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellReCfgCancelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellReCfgCancelTimerId = 
                                                    dwCIMCellReCfgCancelTimerId;

    TranStateWithTag(CCM_CIM_MainComponentFsm, 
                     WaitReCfgCancelRsp,
                     S_CIM_MAINCOM_WAITSETUPCANCELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITSETUPCANCELRSP", 
                      S_CIM_MAINCOM_WAITSETUPCANCELRSP);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_ReCfgCancelTimeOut
* 功能描述: 小区CIM主控构件处理 EV_T_CCM_CIM_CELLRECFGCANCEL_TIMEOUT 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_ReCfgCancelTimeOut(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);    
    CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLRECFGCANCEL_TIMEOUT,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            GetTag(),
                            RNLC_FATAL_LEVEL, 
                            " CCM_CIM_MainComponentFsm CellReCfg Cancel TimeOut! ");
    CIMPrintRecvMessage("EV_T_CCM_CIM_CELLRECFGCANCEL_TIMEOUT", pMsg->m_dwSignal);
    
    WORD32 dwResult = SendToCellDelComDelReq(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellDelComDelReq ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq;
    }

    WORD32 dwCIMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLDEL, 
                                                      TIMER_CIMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = dwCIMCellDelTimerId;
    
    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelRsp, S_CIM_MAINCOM_WAITDELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELRSP", S_CIM_MAINCOM_WAITDELRSP); 

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_ReCfgCancelRsp
* 功能描述: 小区CIM主控构件处理 CMD_CIM_CELL_CFG_CANCEL_RSP 消息的函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Handle_ReCfgCancelRsp(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CCM_CIM_MSGLEN_CHECK(sizeof(T_CmdCIMCellReCfgCancelRsp), pMsg->m_wLen);   
    CIMPrintRecvMessage("CMD_CIM_CELL_CFG_CANCEL_RSP", pMsg->m_dwSignal);

    const T_CmdCIMCellReCfgCancelRsp *ptCmdCIMCellReCfgCancelRsp = 
              static_cast<T_CmdCIMCellReCfgCancelRsp*>(pMsg->m_pSignalPara);

    WORD32 dwResult = CIMKillTimerByTimerId(GetComponentContext()->tCIMMainVar.\
                                                   dwCIMCellReCfgCancelTimerId);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
            
    dwResult = SendToCellDelComDelReq(m_ucCellDelReason);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_SendToCellDelComDelReq ");
        
        return ERR_CCM_CIM_FunctionCallFail_SendToCellDelComDelReq;
    }

    if (CCM_CELLSETUPCANCEL_OK != ptCmdCIMCellReCfgCancelRsp->dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_CELLRECFGCANCEL_FAIL,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                ptCmdCIMCellReCfgCancelRsp->dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm Cell ReCfg Cancel Fail! ");
    }

    WORD32 dwCIMCellDelTimerId = USF_OSS_SetRelativeTimer(TIMER_CCM_CIM_CELLDEL, 
                                                      TIMER_CIMCELLDEL_DURATION, 
                                                      PARAM_NULL);
    if (INVALID_TIMER_ID == dwCIMCellDelTimerId)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                GetTag(),
                                RNLC_FATAL_LEVEL, 
                                "CCM_CIM_MainComponentFsm SetRelativeTimer Failed!");
        
        return ERR_CCM_CIM_FunctionCallFail_CIMKillTimerByTimerId;
    }
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = dwCIMCellDelTimerId;
    
    TranStateWithTag(CCM_CIM_MainComponentFsm, WaitDelRsp, S_CIM_MAINCOM_WAITDELRSP);
    CIMPrintTranState("S_CIM_MAINCOM_WAITDELRSP", S_CIM_MAINCOM_WAITDELRSP); 
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Handle_ErrorMsg
* 功能描述: 小区CIM主控构件处理错误消息统一处理函数
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm ::Handle_ErrorMsg(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    CIMPrintRecvMessage("ErrorMsg", pMsg->m_dwSignal);

    CCM_CIM_ExceptionReport(ERR_CCM_CIM_ErrorMsg,
                            GetJobContext()->tCIMVar.wInstanceNo,
                            pMsg->m_dwSignal,
                            RNLC_WARN_LEVEL, 
                            "CCM_CIM_MainComponentFsm Received Error Msg!");

    return RNLC_SUCC;
}
/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::HandleSendAcBarInfoForCpuRate
* 功能描述: 给所有可用的小区发送基带接入配置参数。
*                        为了控制cpu的利用率
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::HandleSendAcBarInfoForCpuRate(const Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg);
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);
    T_CIMVar *ptCIMVar=NULL;
    WORD32 dwResult  = 0;
    T_RnlcBbCapsInfo tRnlcBbCapsInfo;
    memset((void *)&tRnlcBbCapsInfo,0,sizeof(T_RnlcBbCapsInfo));

    const TCmmCimBbCapsBarReq *ptCimCapsBarInfo = 
                        static_cast<TCmmCimBbCapsBarReq*>(pMsg->m_pSignalPara);

    ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    PID tBBPid = ptCIMVar->tCIMPIDinfo.tBBPid;
    if (USF_BPLBOARDTYPE_BPL1 == ptCIMVar->wBoardType)
    {
        /*****************************BB请求头*************************************/
        /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
        tRnlcBbCapsInfo.tMsgHeader.wMsgLen = sizeof(tRnlcBbCapsInfo);
        /* 消息类型 */
        tRnlcBbCapsInfo.tMsgHeader.wMsgType = EV_RNLC_BB_AC_BARRING_IND;    
        /* 流水号 */
        tRnlcBbCapsInfo.tMsgHeader.wFlowNo = RnlcGetFlowNumber();   
        /* 小区ID */
        tRnlcBbCapsInfo.tMsgHeader.wL3CellId = ptCIMVar->wCellId;   
        /* 小区板内索引 */
        tRnlcBbCapsInfo.tMsgHeader.ucCellIdInBoard = ptCIMVar->ucCellIdInBoard;
        /* 时间戳 */
        tRnlcBbCapsInfo.tMsgHeader.wTimeStamp = 0;
        /*****************************消息内容*************************************/
        tRnlcBbCapsInfo.ucActiveInd = ptCimCapsBarInfo->ucActiveInd;
        tRnlcBbCapsInfo.wacBarringNum = ptCimCapsBarInfo->wacBarringNum;

        /*发送caps的指示信息到基带板*/
        dwResult = USF_OSS_SendAsynMsg( EV_RNLC_BB_AC_BARRING_IND, (void *)&tRnlcBbCapsInfo,(WORD16)sizeof(tRnlcBbCapsInfo), COMM_RELIABLE, &tBBPid);
        /*返回失败打印错误信息*/
        if ( dwResult!= RNLC_SUCC )
        {
            /*打印错误*/
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg, dwResult,\
                                    0, RNLC_FATAL_LEVEL, "\n  [Cim Main]Fail to send Message[%d] to BB! \n",EV_RNLC_BB_AC_BARRING_IND);
        }
        else
        {
            /*为了调试，这里打印出来码流*/          
            CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n Key Word: ucCellIdInBoard:%d,wL3CellId:%d,ucActiveInd:%d,wacBarringNum:%d \n",\
                       tRnlcBbCapsInfo.tMsgHeader.ucCellIdInBoard,\
                       tRnlcBbCapsInfo.tMsgHeader.wL3CellId,\
                       tRnlcBbCapsInfo.ucActiveInd,\
                       tRnlcBbCapsInfo.wacBarringNum);
        }
    }
    else if(USF_BPLBOARDTYPE_BPL0 == ptCIMVar->wBoardType)
    {

        TCmmCimBbCapsBarReq tCmmCimCapsInfo;
        memset((void *)&tCmmCimCapsInfo,0,sizeof(TCmmCimBbCapsBarReq));

        tCmmCimCapsInfo.wCellId = ptCIMVar->wCellId;
        tCmmCimCapsInfo.wacBarringNum = ptCimCapsBarInfo->wacBarringNum;
        tCmmCimCapsInfo.ucActiveInd = ptCimCapsBarInfo->ucActiveInd;

        Message tRnlcCmacCapsInfoFDDV20;
        tRnlcCmacCapsInfoFDDV20.m_wSourceId = ID_CCM_CIM_MainComponent;
        tRnlcCmacCapsInfoFDDV20.m_dwSignal = EV_RNLC_BB_AC_BARRING_IND;
        tRnlcCmacCapsInfoFDDV20.m_wLen = sizeof(tRnlcCmacCapsInfoFDDV20);
        tRnlcCmacCapsInfoFDDV20.m_pSignalPara = static_cast<void*>(&tCmmCimCapsInfo);

        dwResult = SendTo(ID_CCM_CIM_AdptBBComponent, &tRnlcCmacCapsInfoFDDV20); 
        if (SSL_OK != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                              static_cast<WORD32>(GetJobContext()->tCIMVar.wCellId), 
                              dwResult,
                              RNLC_ERROR_LEVEL,
                              "\n FunctionCallFail_SDF_SendTo \n");
            
            return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
        }

        CIMPrintSendMessage("EV_RNLC_BB_AC_BARRING_IND", 
                             EV_RNLC_BB_AC_BARRING_IND);                 
    }
    else
    {
       
       CCM_CIM_LOG(RNLC_INFO_LEVEL, "\n Unknow BPL Type!\n");
       return RNLC_FAIL;
    }
    return RNLC_SUCC;
}
/*****************************************************************************/
/*                                消息发送函数                               */
/*****************************************************************************/

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendCellCfgReqToSetup
* 功能描述: 给小区建立构件发送小区建立请求
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendCellCfgReqToSetup(BYTE ucCellOprReason)
{ 
    T_CmdCIMCellCfgReq tCmdCIMCellCfgReq;
    memset(&tCmdCIMCellCfgReq, 0, sizeof(tCmdCIMCellCfgReq));
    tCmdCIMCellCfgReq.ucCellOprReason = ucCellOprReason;

    Message tCfgReqMsg;
    tCfgReqMsg.m_wSourceId = ID_CCM_CIM_MainComponent;
    tCfgReqMsg.m_dwSignal = CMD_CIM_CELL_CFG_REQ;
    tCfgReqMsg.m_wLen = sizeof(T_CmdCIMCellCfgReq);
    tCfgReqMsg.m_pSignalPara = static_cast<void*>(&tCmdCIMCellCfgReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tCfgReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }

    CIMPrintSendMessage("CMD_CIM_CELL_CFG_REQ", CMD_CIM_CELL_CFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_CFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCIMCellCfgReq),
                     (const BYTE *)(&tCmdCIMCellCfgReq));

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCMMCfgRsp
* 功能描述: 给CMM模块发送小区建立响应
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCMMCfgRsp(WORD32 dwRspResult)
{     
    T_CimCmmCellCfgRsp tCimCmmCellCfgRsp;
    memset(&tCimCmmCellCfgRsp, 0, sizeof(tCimCmmCellCfgRsp));
    
    tCimCmmCellCfgRsp.dwResult = dwRspResult;
    tCimCmmCellCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;

    PID tCMMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tCMMPid;
    
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_CELL_CFG_RSP, 
                                             &tCimCmmCellCfgRsp, 
                                             sizeof(tCimCmmCellCfgRsp), 
                                             COMM_RELIABLE, 
                                             &tCMMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }

    CIMPrintSendMessage("EV_CIM_CMM_CELL_CFG_RSP", EV_CIM_CMM_CELL_CFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_CIM_CMM_CELL_CFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCimCmmCellCfgRsp),
                     (const BYTE *)(&tCimCmmCellCfgRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCMMDelRsp
* 功能描述: 给CMM模块发送小区删除响应
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCMMDelRsp(const T_CellDelDetailResult tCellDelDetailResult)
{    
    T_CimCmmCellDelRsp tCimCmmCellDelRsp;
    memset(&tCimCmmCellDelRsp, 0, sizeof(tCimCmmCellDelRsp));
    
    tCimCmmCellDelRsp.wCellId = GetJobContext()->tCIMVar.wCellId;
    tCimCmmCellDelRsp.ucCellOprReason = m_ucCellDelReason;
    tCimCmmCellDelRsp.dwResult = tCellDelDetailResult.dwResult;
    tCimCmmCellDelRsp.dwRRMResult = tCellDelDetailResult.dwRRMResult;
    tCimCmmCellDelRsp.dwRRUResult = tCellDelDetailResult.dwRRUResult;
    tCimCmmCellDelRsp.dwRNLUResult = tCellDelDetailResult.dwRNLUResult;
    tCimCmmCellDelRsp.dwBBResult = tCellDelDetailResult.dwBBResult;

    PID tCMMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tCMMPid;  
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_CELL_REL_RSP, 
                                             &tCimCmmCellDelRsp, 
                                             sizeof(tCimCmmCellDelRsp), 
                                             COMM_RELIABLE, 
                                             &tCMMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }
    CIMPrintSendMessage("EV_CIM_CMM_CELL_REL_RSP", EV_CIM_CMM_CELL_REL_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_CIM_CMM_CELL_REL_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCimCmmCellDelRsp),
                     (const BYTE *)(&tCimCmmCellDelRsp));

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCMMReCfgRsp
* 功能描述: 给CMM模块发送小区重配响应
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCMMReCfgRsp(WORD32 dwRspResult)
{
    T_CimCmmCellReCfgRsp tCimCmmCellReCfgRsp;
    memset(&tCimCmmCellReCfgRsp, 0, sizeof(tCimCmmCellReCfgRsp));
    
    tCimCmmCellReCfgRsp.dwResult = dwRspResult;
    tCimCmmCellReCfgRsp.wCellId = GetJobContext()->tCIMVar.wCellId;

    PID tCMMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tCMMPid;  
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_CIM_CMM_CELL_RECFG_RSP, 
                                             &tCimCmmCellReCfgRsp, 
                                             sizeof(tCimCmmCellReCfgRsp), 
                                             COMM_RELIABLE, 
                                             &tCMMPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_SendAsynMsg! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg;
    }
    CIMPrintSendMessage("EV_CIM_CMM_CELL_RECFG_RSP", EV_CIM_CMM_CELL_RECFG_RSP);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     EV_CIM_CMM_CELL_RECFG_RSP, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCimCmmCellReCfgRsp),
                     (const BYTE *)(&tCimCmmCellReCfgRsp));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCellSetupComCancelReq
* 功能描述: 给小区建立模块发送小区建立取消响应
*           发送该消息的场景一般是小区建立被小区删除操作打断
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCellSetupComCancelReq(VOID)
{
    T_CmdCIMCellCfgCancelReq tCmdCIMCellCfgCancelReq;
    memset(&tCmdCIMCellCfgCancelReq, 0, sizeof(tCmdCIMCellCfgCancelReq));

    Message tCfgCancelReqMsg;
    tCfgCancelReqMsg.m_wSourceId = ID_CCM_CIM_MainComponent;
    tCfgCancelReqMsg.m_dwSignal = CMD_CIM_CELL_CFG_CANCEL_REQ;
    tCfgCancelReqMsg.m_wLen = sizeof(T_CmdCIMCellCfgCancelReq);
    tCfgCancelReqMsg.m_pSignalPara = static_cast<void*>(&tCmdCIMCellCfgCancelReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_CellSetupComponent, &tCfgCancelReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_SendTo! ");
   
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }
    CIMPrintSendMessage("CMD_CIM_CELL_CFG_CANCEL_REQ", CMD_CIM_CELL_CFG_CANCEL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_CFG_CANCEL_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCfgCancelReqMsg),
                     (const BYTE *)(&tCfgCancelReqMsg));

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCellReCfgComCancelReq
* 功能描述: 给小区重配模块发送小区建立取消响应
*           发送该消息的场景一般是小区重配被小区删除操作打断
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCellReCfgComCancelReq(VOID)
{
    T_CmdCIMCellReCfgCancelReq tCmdCIMCellReCfgCancelReq;
    memset(&tCmdCIMCellReCfgCancelReq, 0, sizeof(tCmdCIMCellReCfgCancelReq));

    Message tReCfgCancelReqMsg;
    tReCfgCancelReqMsg.m_wSourceId = ID_CCM_CIM_MainComponent;
    tReCfgCancelReqMsg.m_dwSignal = CMD_CIM_CELL_RECFG_CANCEL_REQ;
    tReCfgCancelReqMsg.m_wLen = sizeof(tCmdCIMCellReCfgCancelReq);
    tReCfgCancelReqMsg.m_pSignalPara = (void*)(&tCmdCIMCellReCfgCancelReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_CellReCfgComponent, &tReCfgCancelReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }
    CIMPrintSendMessage("CMD_CIM_CELL_RECFG_CANCEL_REQ", CMD_CIM_CELL_RECFG_CANCEL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_RECFG_CANCEL_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCIMCellReCfgCancelReq),
                     (const BYTE *)(&tCmdCIMCellReCfgCancelReq));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCellReCfgComReCfgReq
* 功能描述: 给小区重配模块发送小区重配请求
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCellReCfgComReCfgReq(WORD32 dwCellOprPara)
{
    T_CmdCIMCellReCfgReq tCmdCIMCellReCfgReq;
    memset(&tCmdCIMCellReCfgReq, 0, sizeof(tCmdCIMCellReCfgReq));
    tCmdCIMCellReCfgReq.dwCellOprPara = dwCellOprPara;
    tCmdCIMCellReCfgReq.wCellId = GetJobContext()->tCIMVar.wCellId;

    Message tReCfgReqMsg;
    tReCfgReqMsg.m_wSourceId = ID_CCM_CIM_MainComponent;
    tReCfgReqMsg.m_dwSignal = CMD_CIM_CELL_RECFG_REQ;
    tReCfgReqMsg.m_wLen = sizeof(tCmdCIMCellReCfgReq);
    tReCfgReqMsg.m_pSignalPara = (void*)(&tCmdCIMCellReCfgReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_CellReCfgComponent, &tReCfgReqMsg);
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }
    CIMPrintSendMessage("CMD_CIM_CELL_RECFG_REQ", CMD_CIM_CELL_RECFG_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_RECFG_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCIMCellReCfgReq),
                     (const BYTE *)(&tCmdCIMCellReCfgReq));
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::SendToCellReCfgComReCfgReq
* 功能描述: 给小区重配模块发送小区重配请求
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::SendToCellDelComDelReq(WORD32 dwCause)
{
    T_CmdCIMCellDelReq tCmdCIMCellDelReq;
    memset(&tCmdCIMCellDelReq, 0, sizeof(tCmdCIMCellDelReq));
    tCmdCIMCellDelReq.dwCause = dwCause;
    tCmdCIMCellDelReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    
    Message tDelReqMsg;
    tDelReqMsg.m_wSourceId = ID_CCM_CIM_MainComponent;
    tDelReqMsg.m_dwSignal = CMD_CIM_CELL_REL_REQ;
    tDelReqMsg.m_wLen = sizeof(T_CmdCIMCellDelReq);
    tDelReqMsg.m_pSignalPara = static_cast<void*>(&tCmdCIMCellDelReq);

    WORD32 dwResult = SendTo(ID_CCM_CIM_CellDelComponent, &tDelReqMsg); 
    if (SSL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_SendTo,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_SendTo! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_SendTo;
    }
    CIMPrintSendMessage("CMD_CIM_CELL_REL_REQ", CMD_CIM_CELL_REL_REQ);
    RnlcSendSigTrace(SIG_TRACE_MSG_TYPE_CCM_CMD,
                     CMD_CIM_CELL_REL_REQ, 
                     GetJobContext()->tCIMVar.wCellId,
                     RNLC_INVALID_WORD,
                     RNLC_INVALID_WORD,
                     RNLC_ENB_TRACE_OTHER, 
                     sizeof(tCmdCIMCellDelReq),
                     (const BYTE *)(&tCmdCIMCellDelReq));

    return RNLC_SUCC;
}

/*****************************************************************************/
/*                                其他私有函数                               */
/*****************************************************************************/

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::UpdateCellCfgInfobyDBS
* 功能描述: 通过读取数据库更新本地小区配置信息
*           该函数在小区建立和小区重配时被分别执行一次
* 算法描述: 无
* 全局变量: 无
* Input参数: const Message *pMsg: 消息
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::UpdateCellCfgInfobyDBS(VOID)
{
    if (USF_BPLBOARDTYPE_BPL1 == GetJobContext()->tCIMVar.wBoardType)
    {
        /* 获取数据库信息, 更新小区配置信息 */
        T_DBS_GetCellCfgInfoForV25_REQ tDBSGetCellCfgInfoV25Req;
        memset(&tDBSGetCellCfgInfoV25Req, 0, sizeof(tDBSGetCellCfgInfoV25Req));  
        tDBSGetCellCfgInfoV25Req.wCellId = GetJobContext()->tCIMVar.wCellId;
        tDBSGetCellCfgInfoV25Req.wCallType = USF_MSG_CALL;
        
        T_DBS_GetCellCfgInfoForV25_ACK tDBSGetCellCfgInfoV25Ack;
        memset(&tDBSGetCellCfgInfoV25Ack, 0, sizeof(tDBSGetCellCfgInfoV25Ack));
        BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetCellCfgInfoForV25_REQ,
                                         static_cast<VOID*>(&tDBSGetCellCfgInfoV25Req),
                                         static_cast<VOID*>(&tDBSGetCellCfgInfoV25Ack));
        if ((!bDbResult) || (0 != tDBSGetCellCfgInfoV25Ack.dwResult))
        {        
            const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetCellCfgInfoReq,
                                    tDBSGetCellCfgInfoV25Ack.dwResult,
                                    bDbResult,
                                    RNLC_FATAL_LEVEL, 
                                    " CCM_CIM_MainComponentFsm DBAccessFail_GetCellCfgInfoReq, wInstanceNo:%d! ",
                                    wInstanceNo);
            
            return ERR_CCM_CIM_DBAccessFail_GetCellCfgInfoReq;
        }
        
        GetJobContext()->tCIMVar.tCellCfgInfoForV25 = tDBSGetCellCfgInfoV25Ack.tCellCfgInfoForV25;

        /* 获取数据库信息, 更新 RRU 信息 */
        T_DBS_GetRruInfoByCellId_REQ tGetRruInfoByCellReq;
        memset(&tGetRruInfoByCellReq, 0, sizeof(tGetRruInfoByCellReq));
        tGetRruInfoByCellReq.wCallType = USF_MSG_CALL;
        tGetRruInfoByCellReq.wCellId = GetJobContext()->tCIMVar.wCellId;

        T_DBS_GetRruInfoByCellId_ACK tGetRruInfoByCellAck;
        memset(&tGetRruInfoByCellAck, 0, sizeof(tGetRruInfoByCellAck));
#if 0
        bDbResult = UsfDbsAccess(EV_DBS_GetRruInfoByCellId_REQ,
                                 static_cast<VOID*>(&tGetRruInfoByCellReq),
                                 static_cast<VOID*>(&tGetRruInfoByCellAck));
        if ((!bDbResult) || (0 != tGetRruInfoByCellAck.dwResult))
        {
            const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetRruInfoByCellReq, 
                                    tGetRruInfoByCellAck.dwResult,
                                    bDbResult,
                                    RNLC_FATAL_LEVEL, 
                                    "CCM_CIM_MainComponentFsm DBAccessFail_GetRruInfoByCellReq, wInstanceNo:%d! ",
                                    wInstanceNo);
#ifndef VS2008
            return ERR_CCM_CIM_DBAccessFail_GetRruInfoByCellReq;
#endif
        }

        GetJobContext()->tCIMVar.tRRUInfo.ucRruRack = tGetRruInfoByCellAck.ucRruRack;
        GetJobContext()->tCIMVar.tRRUInfo.ucRruShelf = tGetRruInfoByCellAck.ucRruShelf;
        GetJobContext()->tCIMVar.tRRUInfo.ucRruSlot = tGetRruInfoByCellAck.ucRruSlot;
        GetJobContext()->tCIMVar.tRRUInfo.dwStatus = tGetRruInfoByCellAck.dwStatus;
        GetJobContext()->tCIMVar.tRRUInfo.ucBpRack = tGetRruInfoByCellAck.ucBpRack;
        GetJobContext()->tCIMVar.tRRUInfo.ucBpShelf = tGetRruInfoByCellAck.ucBpShelf;
        GetJobContext()->tCIMVar.tRRUInfo.ucBpSlot = tGetRruInfoByCellAck.ucBpSlot;
        GetJobContext()->tCIMVar.tRRUInfo.ucBpPort = tGetRruInfoByCellAck.ucBpPort;
        GetJobContext()->tCIMVar.tRRUInfo.ucDlEnabledAntNum = tGetRruInfoByCellAck.ucDlEnabledAntNum;
        GetJobContext()->tCIMVar.tRRUInfo.ucDlAntMap = tGetRruInfoByCellAck.ucDlAntMap;
        GetJobContext()->tCIMVar.tRRUInfo.ucUlEnabledAntNum = tGetRruInfoByCellAck.ucUlEnabledAntNum;
        GetJobContext()->tCIMVar.tRRUInfo.ucUlAntMap = tGetRruInfoByCellAck.ucUlAntMap;
        GetJobContext()->tCIMVar.tRRUInfo.ucUlAntCapacity = tGetRruInfoByCellAck.ucUlAntCapacity;
#endif
        // 这个制式信息在3.0/3.1下面都会用到的
        GetJobContext()->tCIMVar.ucRadioMode = 
            tDBSGetCellCfgInfoV25Ack.tCellCfgInfoForV25.ucRadioMode;
#if 0
        // 保存基带天线和端口信息
        if (TDD_RADIOMODE == GetJobContext()->tCIMVar.ucRadioMode)
        {
            // 读取数据库，获取CP信息，计算TDD上下行端口号
            T_DBS_GetSvrcpInfo_REQ tGetSvrcpInfoReq;
            memset((VOID*)&tGetSvrcpInfoReq,  0, sizeof(tGetSvrcpInfoReq));

            T_DBS_GetSvrcpInfo_ACK tGetSvrcpInfoAck;
            memset((VOID*)&tGetSvrcpInfoAck,  0, sizeof(tGetSvrcpInfoAck));   

            tGetSvrcpInfoReq.wCallType = USF_MSG_CALL;
            tGetSvrcpInfoReq.wCId = GetJobContext()->tCIMVar.wCellId;
            tGetSvrcpInfoReq.wBpType = 1; 
            bDbResult = UsfDbsAccess(EV_DBS_GetSvrcpInfo_REQ, 
                                             (VOID *)&tGetSvrcpInfoReq, 
                                             (VOID *)&tGetSvrcpInfoAck);
            if ((FALSE == bDbResult) || (0 != tGetSvrcpInfoAck.dwResult))
            {
                CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSvrcpInfoReq, 
                                        bDbResult,
                                        tGetSvrcpInfoAck.dwResult,
                                        RNLC_FATAL_LEVEL, 
                                        " DBAccessFail_GetSvrcpInfoReq! ");
                
                return ERR_CCM_CIM_DBAccessFail_GetSvrcpInfoReq;
            }

            /* 计算下行天线端口数 */
            BYTE ucTempAntPortNum = 0;
            BYTE ucTempAntPortMap = 0;
            for (BYTE ucAntNumLoop = 0; ucAntNumLoop < 8; ucAntNumLoop++)
            {
                ucTempAntPortMap |= (BYTE)(1 << tGetSvrcpInfoAck.atCpInfo[0].aucAnttoPortMapV25[ucAntNumLoop]);
            }

            for (BYTE ucAntPortNumLoop = 0; ucAntPortNumLoop < 4; ucAntPortNumLoop++)
            {
                if (0 != (ucTempAntPortMap & (1 << ucAntPortNumLoop)))
                {
                    ucTempAntPortNum++;
                }
            }

            // 将下行天线端口数转换为枚举值
            if (1 == ucTempAntPortNum)
            {
                GetJobContext()->tCIMVar.tRRUInfo.ucDlAntPortNum = 0;        
            }
            else if (2 == ucTempAntPortNum)
            {
                GetJobContext()->tCIMVar.tRRUInfo.ucDlAntPortNum = 1;        
            }
            else if (4 ==ucTempAntPortNum )
            {
                GetJobContext()->tCIMVar.tRRUInfo.ucDlAntPortNum = 2;
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

        }
        else if (FDD_RADIOMODE ==GetJobContext()->tCIMVar.ucRadioMode)
        {
            /* FDD 模式下，天线数目与天线端口数目是相等的  */
            GetJobContext()->tCIMVar.tRRUInfo.ucDlAntPortNum = 
                               GetJobContext()->tCIMVar.tRRUInfo.ucDlEnabledAntNum;
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
#endif
    }
    else if (USF_BPLBOARDTYPE_BPL0 == GetJobContext()->tCIMVar.wBoardType)
    {
        WORD32 dwResult = GetSaveCellCfgForV20();
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetSaveCellCfgForV20,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " FunctionCallFail_GetSaveCellCfgForV20! ");
            
            return ERR_CCM_CIM_FunctionCallFail_GetSaveCellCfgForV20;
        }
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

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::InitJobContext(VOID)
{
    /* 系统上电的时候获取本线程的实例号，为了打印方便设置 */
#ifndef VS2008
    GetJobContext()->tCIMVar.wInstanceNo = OSS_GetSelfInstanceNo();
#else
    GetJobContext()->tCIMVar.wInstanceNo = 0;
#endif
    GetJobContext()->tCIMVar.wCellId = INVALID_CELLID;
    GetJobContext()->tCIMVar.bUsedFlg = FALSE;
    GetJobContext()->tCIMVar.wBoardType = 0;
    GetJobContext()->tCIMVar.ucCellIdInBoard = RNLC_INVALID_BYTE;
    GetJobContext()->tCIMVar.ucBBCfgType = 0;

    memset(&(GetJobContext()->tCIMVar.tCIMPIDinfo),
           0,
           sizeof(GetJobContext()->tCIMVar.tCIMPIDinfo));
    memset(&(GetJobContext()->tCIMVar.tChannelGidInfo),
           0,
           sizeof(GetJobContext()->tCIMVar.tChannelGidInfo));
    memset(&(GetJobContext()->tCIMVar.tCellCfgInfoForV25),
           0,
           sizeof(GetJobContext()->tCIMVar.tCellCfgInfoForV25));
    memset(&(GetJobContext()->tCIMVar.tCellMacRbInfo),
           0,
           sizeof(GetJobContext()->tCIMVar.tCellMacRbInfo));

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitComponentContext
* 功能描述: 初始化小区CIM主控构件数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::InitComponentContext(VOID)
{
    GetComponentContext()->tCIMMainVar.dwCIMCellSetupTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMMainVar.dwCIMCellReCfgTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMMainVar.dwCIMCellDelTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMMainVar.dwCIMCellSetupCancelTimerId = INVALID_TIMER_ID;
    GetComponentContext()->tCIMMainVar.dwCIMCellReCfgCancelTimerId = INVALID_TIMER_ID;

    return RNLC_SUCC;
}

WORD32 CCM_CIM_MainComponentFsm::ResetComponents()
{
    ComponentReset(ID_CCM_CIM_MainComponent);
    ComponentReset(ID_CCM_CIM_CellSetupComponent);
    ComponentReset(ID_CCM_CIM_CellReCfgComponent);
    ComponentReset(ID_CCM_CIM_CellDelComponent);
    ComponentReset(ID_CCM_CIM_CellBlockComponent);
    ComponentReset(ID_CCM_CIM_CellUnBlockComponent);
    ComponentReset(ID_CCM_CIM_BBAlgParaCfgComponent);
    ComponentReset(ID_CCM_CIM_AdptBBComponent);
    ComponentReset(ID_CCM_CIM_AdptRnluComponent);
    ComponentReset(ID_CCM_CIM_DebugComponent);
    ComponentReset(ID_CCM_CIM_SyncComponent);

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetSaveAllPid
* 功能描述: 获取并且保存所有需要的外围设备的PID
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetSaveAllPid(VOID)
{
    WORD32 dwResult = RNLC_SUCC;
    T_PRMV25CellInfo tGetPRMV25CellInfo;
    WORD32                              dwPno;
    WORD16                              dwFuncResult;
    WORD16                              wProcType;
    
    PID tCMMPid;
    memset(&tCMMPid, 0, sizeof(tCMMPid));
    dwResult = GetCMMPid(&tCMMPid);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetCMMPid,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetCMMPid! ");
        
        return ERR_CCM_CIM_FunctionCallFail_GetCMMPid;
    }
    GetJobContext()->tCIMVar.tCIMPIDinfo.tCMMPid = tCMMPid;

    PID tRRMPid;
    memset(&tRRMPid, 0, sizeof(tRRMPid));
    dwResult = GetRRMPid(&tRRMPid);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetRRMPid,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetRRMPid! ");
        
        return ERR_CCM_CIM_FunctionCallFail_GetRRMPid;
    }
    GetJobContext()->tCIMVar.tCIMPIDinfo.tRRMPid = tRRMPid;

    PID tOAMPid;
    memset(&tOAMPid, 0, sizeof(tOAMPid));
    dwResult = GetOAMPid(&tOAMPid);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_GetOAMPid,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " FunctionCallFail_GetRRMPid! ");
        
        return ERR_CCM_CIM_FunctionCallFail_GetRRMPid;
    }
    GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid = tOAMPid;

    PID tBBPid;
    memset(&tBBPid, 0, sizeof(tBBPid));
#ifdef VS2008
    GetJobContext()->tCIMVar.wBoardType = USF_BPLBOARDTYPE_BPL0;
#endif
    if(USF_BPLBOARDTYPE_BPL0 == GetJobContext()->tCIMVar.wBoardType)
    {
        dwResult = USF_GetBplPIDByCellId(GetJobContext()->tCIMVar.wCellId, 
                                         USF_CMAC_PROCESS,
                                         &tBBPid);
        if (USFBPL_OK != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_GetBplPIDByCellId,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " CCM_CIM_MainComponentFsm FunctionCallFail_USF_GetBplPIDByCellId! ");

        }
    }
    else
    {
        memset(&tGetPRMV25CellInfo,0,sizeof(tGetPRMV25CellInfo));
        dwResult = PRM_GetV25BPResInfoByCellId(GetJobContext()->tCIMVar.wCellId,
                                                                           &tGetPRMV25CellInfo);
        #if (_LOGIC_BOARD == _LOGIC_BSTRQA)
            wProcType = 0x3100;
        #else
            wProcType = 0xffff; /* 待确认 */
        #endif
 
        /* 构造PID */
        dwPno = OSS_ConstructPNO(wProcType, 1);
        /*lint -save -e734 */
        dwFuncResult = OamGetPIDByPos(tGetPRMV25CellInfo.tMacResInfo.ucRack,
                                                        tGetPRMV25CellInfo.tMacResInfo.ucShelf,
                                                        tGetPRMV25CellInfo.tMacResInfo.ucSlotNo,
                                                        tGetPRMV25CellInfo.tMacResInfo.ucMacCpuID,
                                                        dwPno,
                                                        &tBBPid);
        /*lint -restore*/
        if (0 != dwFuncResult)
        {
            return ((dwFuncResult << 16) | (USFBPL_CALLOAMFAIL));
        }
    }
    

    GetJobContext()->tCIMVar.tCIMPIDinfo.tBBPid = tBBPid;
    
    PID tRNLUPid;
    memset(&tRNLUPid, 0, sizeof(tRNLUPid));

    if(USF_BPLBOARDTYPE_BPL0 == GetJobContext()->tCIMVar.wBoardType)
    {
        dwResult = USF_GetBplPIDByCellId(GetJobContext()->tCIMVar.wCellId, 
                                         USF_RNLU_PROCESS,
                                         &tRNLUPid);
    }
    else
    {
        memset(&tGetPRMV25CellInfo,0,sizeof(tGetPRMV25CellInfo));
        dwResult = PRM_GetV25BPResInfoByCellId(GetJobContext()->tCIMVar.wCellId,
                                                                           &tGetPRMV25CellInfo);

       wProcType = P_S_LTE_RNLU_USM_PROC;

                      /* 构造PID */
        dwPno = OSS_ConstructPNO(wProcType, 1);
        /*lint -save -e734 */
        dwFuncResult = OamGetPIDByPos(tGetPRMV25CellInfo.tMacResInfo.ucRack,
                                      tGetPRMV25CellInfo.tMacResInfo.ucShelf,
                                      tGetPRMV25CellInfo.tMacResInfo.ucSlotNo,
                                       tGetPRMV25CellInfo.tMacResInfo.ucRnluCpuID,
                                      dwPno,
                                      &tRNLUPid);
        /*lint -restore*/
        if (0 != dwFuncResult)
        {
            return ((dwFuncResult << 16) | (USFBPL_CALLOAMFAIL));
        }
    }

    
    if (USFBPL_OK != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_GetBplPIDByCellId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_USF_GetBplPIDByCellId! ");

    }
    
    GetJobContext()->tCIMVar.tCIMPIDinfo.tRNLUPid = tRNLUPid;

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetSaveCellCfgForV20
* 功能描述: 获取并且保存小区的无线制式
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetSaveCellCfgForV20()
{
    /* 获取数据库信息, 更新小区无线制式信息 */
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
                                " CCM_CIM_MainComponentFsm DBAccessFail_GetSrvcelRecordReq, wInstanceNo:%d! ",
                                wInstanceNo);
        
        return ERR_CCM_CIM_DBAccessFail_GetSrvcelRecordReq;
    }
    
    GetJobContext()->tCIMVar.ucRadioMode = 
                      tDBSGetSrvcelRecordByCellId_ACK.tSrvcelRecord.ucRadioMode;
    

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetSaveAllPid
* 功能描述: 获取并且保存所有需要的外围设备的PID
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::
SaveCMMCellReqPara(const T_CmmCimCellCfgReq *ptCmmCimCellCfgReq)
{
    CCM_CIM_NULL_POINTER_CHECK(ptCmmCimCellCfgReq);
    
    GetJobContext()->tCIMVar.wCellId = ptCmmCimCellCfgReq->wCellId;
    GetJobContext()->tCIMVar.tChannelGidInfo = 
                                            ptCmmCimCellCfgReq->tChannelGidInfo;

    GetJobContext()->tCIMVar.ucCpMergeType = 0; /* 0 表示关闭，1表示打开*/

    T_DBS_GetCellMergeType_REQ tDbsGetCellMergeTypeReq;
    T_DBS_GetCellMergeType_ACK tDbsGetCellMergeTypeAck;

    memset(&tDbsGetCellMergeTypeReq,0,sizeof(T_DBS_GetCellMergeType_REQ));
    memset(&tDbsGetCellMergeTypeAck,0,sizeof(T_DBS_GetCellMergeType_ACK));

    tDbsGetCellMergeTypeReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDbsGetCellMergeTypeReq.wCallType = USF_MSG_CALL;

    BYTE ucResult = UsfDbsAccess(EV_DBS_GetCellMergeType_REQ, (VOID *)&tDbsGetCellMergeTypeReq, (VOID *)&tDbsGetCellMergeTypeAck);
    if ((FALSE == ucResult) || (0 != tDbsGetCellMergeTypeAck.dwResult))
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetSvrcpInfoReq, ucResult,tDbsGetCellMergeTypeAck.dwResult,\
                                                    RNLC_FATAL_LEVEL, "[CPMergeType]Call EV_DBS_GetCellMergeType_REQ fail!\n");
        return FALSE;
    }

    GetJobContext()->tCIMVar.ucCpMergeType = tDbsGetCellMergeTypeAck.ucMergeType;

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetCMMPid
* 功能描述: 获取CMM模块的PID
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetCMMPid(PID *ptPid)
{
    CCM_CIM_NULL_POINTER_CHECK(ptPid);

    WORD32 dwPno = OSS_ConstructPNO(P_S_LTE_CCM_MAIN, 1);
    OSS_STATUS dwOssStatus = USF_OSS_GetPIDByPNO(dwPno, ptPid);
    if (OSS_SUCCESS != dwOssStatus)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_GetPIDByPNO! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO;
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetRRMPid
* 功能描述: 获取RRM模块的PID
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetRRMPid(PID *ptPid)
{
    CCM_CIM_NULL_POINTER_CHECK(ptPid);
    
    WORD32 dwPno = OSS_ConstructPNO(P_S_LTE_RRM_ALG_MAIN, 1);
    
    OSS_STATUS dwOssStatus = USF_OSS_GetPIDByPNO(dwPno, ptPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_GetPIDByPNO! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO;
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::GetOAMPid
* 功能描述: 获取RRU OAM代理模块的PID
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetOAMPid(PID *ptPid)
{
    CCM_CIM_NULL_POINTER_CHECK(ptPid);
        
    WORD32 dwPno = OSS_ConstructPNO(P_LTE_OAM_CMP_MGR, 1);

    OSS_STATUS dwOssStatus = USF_OSS_GetPIDByPNO(dwPno, ptPid);
    if (OSS_SUCCESS != dwOssStatus)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwOssStatus,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_OSS_GetPIDByPNO! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_OSS_GetPIDByPNO;
    }

    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetSaveCellIdInBoard(VOID)
{
    T_PRMBpResInfo tPRMBpResInfo;
    memset(&tPRMBpResInfo, 0, sizeof(tPRMBpResInfo));

    // 申请资源以前，先查询一把，防止资源吊死情况
    WORD32 dwResult = PRM_GetBPResInfoByCellId(GetJobContext()->tCIMVar.wCellId, 
                                               &tPRMBpResInfo);
    if (CCM_PRM_OK != dwResult)
    {
        dwResult = PRM_RequestBPRes(GetJobContext()->tCIMVar.wCellId, 
                                           &tPRMBpResInfo);
        if (RNLC_SUCC != dwResult)
        {
            CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_PRM_RequestBPRes,
                                    GetJobContext()->tCIMVar.wInstanceNo,
                                    dwResult,
                                    RNLC_FATAL_LEVEL, 
                                    " CCM_CIM_MainComponentFsm FunctionCallFail_PRM_RequestBPRes! ");
            
            return ERR_CCM_CIM_FunctionCallFail_PRM_RequestBPRes;
        }
    }
    else
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_BPResInfo_ERROR,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_PRM_RequestBPRes! ");
    }
    
    GetJobContext()->tCIMVar.ucCellIdInBoard = tPRMBpResInfo.ucCellIdInBoard;

    dwResult = Ccm_cim_Main_SetCellInBoard(GetJobContext()->tCIMVar.wCellId, 
                                        tPRMBpResInfo.ucCellIdInBoard, 
                                        tPRMBpResInfo.ucRnluCpuID, 
                                        tPRMBpResInfo.ucCmacCpuID);
    if (RNLC_SUCC != dwResult)
    {
        CCM_ExceptionReport(RNLC_INVALID_DWORD, 
                            dwResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "Ccm_cim_Main_SetCellInBoard return Fail");
        return dwResult;
    }

#if 0

    /* 保存CellIdInBoard 到数据库中 */
    T_DBS_SetCellIndexBPL_REQ tDBSSetCellIndexBPLReq;
    memset(&tDBSSetCellIndexBPLReq, 0, sizeof(tDBSSetCellIndexBPLReq));
    tDBSSetCellIndexBPLReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSSetCellIndexBPLReq.wCallType = USF_MSG_CALL;
    tDBSSetCellIndexBPLReq.wCellIndexBPL = tPRMBpResInfo.ucCellIdInBoard;
    tDBSSetCellIndexBPLReq.ucRnluCpuID = tPRMBpResInfo.ucRnluCpuID;
    tDBSSetCellIndexBPLReq.ucCmacCpuID = tPRMBpResInfo.ucCmacCpuID;

    dwResult = CCM_DBS_SetCellInBoard(&tDBSSetCellIndexBPLReq);
    if (RNLC_SUCC != dwResult)
    {
        CCM_LOG(RNLC_ERROR_LEVEL, "CCM_DBS_SetCellInBoard Return Fail");
        return ERR_CCM_CIM_DBAccessFail_SetCellIndexBPLReq;
    }
#endif
#if 0
    T_DBS_SetCellIndexBPL_ACK tDBSSetCellIndexBPLAck;
    memset(&tDBSSetCellIndexBPLAck, 0, sizeof(tDBSSetCellIndexBPLAck));

    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_SetCellIndexBPL_REQ,
                                     static_cast<VOID*>(&tDBSSetCellIndexBPLReq),
                                     static_cast<VOID*>(&tDBSSetCellIndexBPLAck));
    if(FALSE == bDbResult || 0 != tDBSSetCellIndexBPLAck.dwResult)
    {        
        CCM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_SetCellIndexBPLReq,
                            tDBSSetCellIndexBPLAck.dwResult,
                            bDbResult,
                            RNLC_ERROR_LEVEL, 
                            " DBAccessFail_SetCellIndexBPL_REQ! ");

        return ERR_CCM_CIM_DBAccessFail_SetCellIndexBPLReq;
    }
#endif
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::GetSaveBoardType(VOID)
{
    WORD16 wBoardType;
    WORD32 dwResult = USF_GetBplBoardTypeByCellId(GetJobContext()->tCIMVar.wCellId, 
                                                  &wBoardType);
    if (RNLC_SUCC != dwResult)
    {
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_GetBplBoardTypeByCellId,
                                GetJobContext()->tCIMVar.wInstanceNo,
                                dwResult,
                                RNLC_FATAL_LEVEL, 
                                " CCM_CIM_MainComponentFsm FunctionCallFail_USF_GetBplBoardTypeByCellId! ");
        
        return ERR_CCM_CIM_FunctionCallFail_USF_GetBplBoardTypeByCellId;
    }
    GetJobContext()->tCIMVar.wBoardType = wBoardType;
    
    return RNLC_SUCC;
}


/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::
CIMPrintRecvMessage(const void *pSignal, WORD32 dwSignal)
{
    CCM_CIM_NULL_POINTER_CHECK(pSignal);

    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                "CIM InstanceNo: %d, CCM_CIM_MainComponentFsm CurrentState: %d, Receive Message %s, MessageID: %d",
                wInstanceNo, dwTag, pSignal, dwSignal);
                                   
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::
CIMPrintSendMessage(const void *pSignal, WORD32 dwSignal)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    const WORD32 dwTag = GetTag();
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo: %d, CCM_CIM_MainComponentFsm CurrentState: %d, Send Message %s, MessageID: %d ",
                wInstanceNo, dwTag, pSignal, dwSignal);
    
    return RNLC_SUCC;
}

/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::InitJobContext
* 功能描述: 初始化业务容器数据区
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::CIMPrintTranState(const void  *pTargetState, BYTE ucTargetState)
{    
    const WORD16 wInstanceNo = GetJobContext()->tCIMVar.wInstanceNo;
    CCM_CIM_LOG(RNLC_INFO_LEVEL,
                " CIM InstanceNo:%d, CCM_CIM_MainComponentFsm TranState To %s, StateID: %d ",
                wInstanceNo, pTargetState, ucTargetState);
    
    return RNLC_SUCC;
}
/*****************************************************************************
* 函数名称: CCM_CIM_MainComponentFsm::Ccm_cim_Main_SetCellInBoard
* 功能描述:
* 算法描述: 无
* 全局变量: 无
* Input参数: VOID
* 输出参数: 无
* 返 回 值: VOID 
* 其他说明: 无
**    
*    
* 版    本: V3.1
* 修改记录: 
*    
****************************************************************************/
WORD32 CCM_CIM_MainComponentFsm::Ccm_cim_Main_SetCellInBoard(WORD16 wCellId, WORD16 wCellInBoard, BYTE ucRnluCpuId, BYTE ucCmacCpuId)
{
    /* 保存CellIdInBoard 到数据库中 */
    T_DBS_SetCellIndexBPL_REQ tDBSSetCellIndexBPLReq;
    WORD32 dwResult = RNLC_SUCC;
    
    memset(&tDBSSetCellIndexBPLReq, 0, sizeof(tDBSSetCellIndexBPLReq));
    
    tDBSSetCellIndexBPLReq.wCellId = wCellId;
    tDBSSetCellIndexBPLReq.wCallType = USF_MSG_CALL;
    tDBSSetCellIndexBPLReq.wCellIndexBPL = wCellInBoard;
    tDBSSetCellIndexBPLReq.ucRnluCpuID = ucRnluCpuId;
    tDBSSetCellIndexBPLReq.ucCmacCpuID = ucCmacCpuId;

    dwResult = CCM_DBS_SetCellInBoard(&tDBSSetCellIndexBPLReq);
    if (RNLC_SUCC != dwResult)
    {
        CCM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_SetCellIndexBPLReq, 
                            dwResult, 
                            RNLC_INVALID_DWORD,
                            RNLC_ERROR_LEVEL, 
                            "CCM_DBS_SetCellInBoard Return Fail");
        return ERR_CCM_CIM_DBAccessFail_SetCellIndexBPLReq;
    }
    return RNLC_SUCC;
}


WORD32 CCM_CIM_MainComponentFsm::CimPowerOnInSlave()
{
   return RNLC_SUCC;
}

VOID CCM_CIM_MainComponentFsm::SetComponentSlave()
{
    TranStateWithTag(CCM_CIM_MainComponentFsm, Handle_InSlaveState,CCM_ALLCOMPONET_SLAVE_STATE);
}

VOID CCM_CIM_MainComponentFsm::Handle_InSlaveState(Message *pMsg)
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
         CCM_LOG(DEBUG_LEVEL,"CCM_CIM_MainComponentFsm get State %d.\n",ucMasterSateCpy);
            break;
        }

        case CMD_CIM_SLAVE_TO_MASTER:
     {
        CCM_LOG(DEBUG_LEVEL,"CCM_CIM_MainComponentFsm SLAVE_TO_MASTER.\n");
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

WORD32 CCM_CIM_MainComponentFsm::HandleMasterToSlave(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_MainComponentFsm HandleMasterToSlave.\n");
    SetComponentSlave();
    return RNLC_SUCC;
}

WORD32 CCM_CIM_MainComponentFsm::HandleSlaveToMaster(Message *pMsg)
{
    CCM_CIM_NULL_POINTER_CHECK(pMsg)
    CCM_CIM_NULL_POINTER_CHECK(pMsg->m_pSignalPara);

    CCM_LOG(DEBUG_LEVEL,"CCM_CIM_MainComponentFsm HandleSlaveToMaster State is %d.\n",ucMasterSateCpy);
    /*lint -save -e30 -e142*/
    switch(ucMasterSateCpy)
    {
        case S_CIM_MAINCOM_IDLE:
               TranStateWithTag(CCM_CIM_MainComponentFsm, Idle, S_CIM_MAINCOM_IDLE);
               break;
    
        case S_CIM_MAINCOM_READY:
               TranStateWithTag(CCM_CIM_MainComponentFsm, Ready, S_CIM_MAINCOM_READY);
               break;
        
        default:
               TranStateWithTag(CCM_CIM_MainComponentFsm, Idle, S_CIM_MAINCOM_IDLE);
               break;
    }
    /*lint -restore*/
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: 
* 功能描述: 在各种状态下处理CMD_PROCCTRL_ROLLBACK_REQ消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::HandleDtxNotifyFromCMM(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK(pMsg); 
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara); 

    CCM_LOG(RNLC_INFO_LEVEL,"recive EV_CMM_CIM_DTX_IND sucess.\n");
    T_CmmCimDtxCellInfoInd *ptCmmCimDtxCellInfoInd = NULL;   

    ptCmmCimDtxCellInfoInd = (T_CmmCimDtxCellInfoInd *)pMsg->m_pSignalPara;

    if (ptCmmCimDtxCellInfoInd->wCellId != GetJobContext()->tCIMVar.wCellId)
    {
        CCM_LOG(RNLC_INFO_LEVEL,"CCM_CIM_MainComponentFsm PreFsm receive Cellid:%d, JobContext Cellid:%d.\n",
                                       ptCmmCimDtxCellInfoInd->wCellId,
                                       GetJobContext()->tCIMVar.wCellId);
        return RNLC_FAIL;
    }
    T_DBS_GetDtxInfoByCellId_REQ tDBSGetDtxInfoByCellId_REQ;
    T_DBS_GetDtxInfoByCellId_ACK tDBSGetDtxInfoByCellId_ACK;

    memset(&tDBSGetDtxInfoByCellId_REQ, 0, sizeof(T_DBS_GetDtxInfoByCellId_REQ)); 
    memset(&tDBSGetDtxInfoByCellId_ACK, 0, sizeof(T_DBS_GetDtxInfoByCellId_ACK)); 

    tDBSGetDtxInfoByCellId_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetDtxInfoByCellId_REQ.wCallType = USF_MSG_CALL;
    tDBSGetDtxInfoByCellId_REQ.ucSrvType = DTX_SRV_TYPE;   /*DTX业务类型值*/
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetDtxInfoByCellId_REQ,
                                     static_cast<VOID*>(&tDBSGetDtxInfoByCellId_REQ),
                                     static_cast<VOID*>(&tDBSGetDtxInfoByCellId_ACK));

    if(FALSE == bDbResult || 0 != tDBSGetDtxInfoByCellId_ACK.dwResult)
    {        
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetDtxInfoByCellId_REQ,
                                tDBSGetDtxInfoByCellId_ACK.dwResult,
                                bDbResult,
                                RNLC_ERROR_LEVEL, 
                                " DBAccessFail_GetDtxInfoByCellId_REQ, wInstanceNo:%d! ");
        
        return ERR_CCM_CIM_DBAccessFail_GetDtxInfoByCellId_REQ;
    }

    /* 节能开关打开，但是RRU不全部支持，上报告警 */
    if (1 == tDBSGetDtxInfoByCellId_ACK.ucEsSwitch &&
        NOT_ALL_RRU_SUPPORT_DTX == tDBSGetDtxInfoByCellId_ACK.bIsAllRRUSupportDtx)
    {
        //上报告警
#if 0//defined(_PRODUCT_TYPE) && (_PRODUCT_TYPE == _PRODUCT_LTE_FDD)
        CCcmAlarm::GetInstance()->CcmAlarmReport(GetJobContext()->tCIMVar.wCellId, FC_LTE_DTX_ENABLE_FAIL);/*错误码需要修改*/
#endif
        return RNLC_SUCC;
    }

    /* 节能开关打开，RRU全部支持*/
    if ((1 == tDBSGetDtxInfoByCellId_ACK.ucEsSwitch) && (ALL_RRU_SUPPORT_DTX == tDBSGetDtxInfoByCellId_ACK.bIsAllRRUSupportDtx))
    {
        //通知基带
        SendRusptDtxCfgReqToBB(&tDBSGetDtxInfoByCellId_ACK);
        return RNLC_SUCC;
    }
    if (0 == tDBSGetDtxInfoByCellId_ACK.ucEsSwitch)
    {
        SendRusptDtxCfgReqToBB(&tDBSGetDtxInfoByCellId_ACK);
    }
    
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: 
* 功能描述: 在各种状态下处理CMD_PROCCTRL_ROLLBACK_REQ消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::Handle_EV_RNLC_BB_RUSPTDTX_CFG_RSP(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK(pMsg); 
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara); 

    //T_RnlcBbRusptdtxCfgRsp *ptRnlcBbRusptdtxCfgRsp = (T_RnlcBbRusptdtxCfgRsp *)(pMsg->m_pSignalPara);

    //CCM_LOG(DEBUG_LEVEL,"Handle_EV_RNLC_BB_RUSPTDTX_CFG_RSP receive wCellId :%d,wResult:%d.\n",T_RnlcBbRusptdtxCfgRsp.wCellId,T_RnlcBbRusptdtxCfgRsp.wResult);
    CCM_LOG(RNLC_INFO_LEVEL,"recive EV_RNLC_BB_RUSPTDTX_CFG_RSP sucess.\n");
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: 
* 功能描述: 在各种状态下处理CMD_PROCCTRL_ROLLBACK_REQ消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::Handle_EV_BB_RNLC_SF_CTRL_REQ(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK(pMsg); 
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara); 

    CCM_LOG(RNLC_INFO_LEVEL,"recive  EV_BB_RNLC_SF_CTRL_REQ sucess.\n");
    T_BbRnlcSubFrmCtrlReq *ptBbRnlcSfCtrlReq = (T_BbRnlcSubFrmCtrlReq *)(pMsg->m_pSignalPara);

    TRnlcRruEsReq  tRnlcOamSfCtrlReq;
    memset(&tRnlcOamSfCtrlReq,0,sizeof(tRnlcOamSfCtrlReq));

    tRnlcOamSfCtrlReq.wCellId = ptBbRnlcSfCtrlReq->tMsgHeader.wL3CellId;
    tRnlcOamSfCtrlReq.wSFStatBitMap = ptBbRnlcSfCtrlReq->wJudgeBitmap;

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_OAM_SF_CTRL_REQ, 
                                             &tRnlcOamSfCtrlReq, 
                                             sizeof(tRnlcOamSfCtrlReq), 
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
    CCM_LOG(RNLC_INFO_LEVEL,"send  EV_RNLC_OAM_SF_CTRL_REQ sucess.cell id is:%d \n",
                                         ptBbRnlcSfCtrlReq->tMsgHeader.wL3CellId);
    return RNLC_SUCC;
}
/*<FUNC>***********************************************************************
* 函数名称: 
* 功能描述: 在各种状态下处理CMD_PROCCTRL_ROLLBACK_REQ消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::Handle_EV_OAM_RNLC_SF_CTRL_IND(Message *pMsg)
{
    CCM_NULL_POINTER_CHECK(pMsg); 
    CCM_NULL_POINTER_CHECK(pMsg->m_pSignalPara); 
    CCM_LOG(RNLC_INFO_LEVEL,"recive  EV_OAM_RNLC_SF_CTRL_IND sucess.\n");
    TRruRnlcEsInd *ptOamRnlcSfCtrlInd = (TRruRnlcEsInd *)(pMsg->m_pSignalPara);

    T_RnlcCmacRRUSubFrmInd tRnlcBbSfCtrlInd;
    memset(&tRnlcBbSfCtrlInd,0,sizeof(tRnlcBbSfCtrlInd));

    /*BB请求头*/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tRnlcBbSfCtrlInd.tMsgHeader.wMsgLen = sizeof(tRnlcBbSfCtrlInd);
    /* 消息类型 */
    tRnlcBbSfCtrlInd.tMsgHeader.wMsgType = EV_RNLC_BB_SF_CTRL_IND; 

    /* 流水号 */
    tRnlcBbSfCtrlInd.tMsgHeader.wFlowNo = RnlcGetFlowNumber();
    
    /* 小区ID */
    tRnlcBbSfCtrlInd.tMsgHeader.wL3CellId = ptOamRnlcSfCtrlInd->wCellId;
    /* 小区板内索引 */
    tRnlcBbSfCtrlInd.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
    tRnlcBbSfCtrlInd.tMsgHeader.wTimeStamp = 0;

    tRnlcBbSfCtrlInd.wEsBitmap = ptOamRnlcSfCtrlInd->wSFStatBitMap;
    tRnlcBbSfCtrlInd.wResult = ptOamRnlcSfCtrlInd->wResult;

    CCM_LOG(RNLC_INFO_LEVEL,"tRnlcBbSfCtrlInd.wEsBitmap:map:%d,result:%d,cellid:%d,oammap:%d,oamresult:%d \n",
                     tRnlcBbSfCtrlInd.wEsBitmap,
                     tRnlcBbSfCtrlInd.wResult,
                     tRnlcBbSfCtrlInd.tMsgHeader.wL3CellId,
                     ptOamRnlcSfCtrlInd->wSFStatBitMap,
                     ptOamRnlcSfCtrlInd->wResult);
    CCM_LOG(RNLC_INFO_LEVEL,"tRnlcBbSfCtrlInd.ucCellIdInBoard:%d \n",
                     tRnlcBbSfCtrlInd.tMsgHeader.ucCellIdInBoard);

    Message Msg;
    Msg.m_wSourceId = ID_CCM_CIM_MainComponent;
    Msg.m_dwSignal = EV_RNLC_BB_SF_CTRL_IND;
    Msg.m_wLen = sizeof(tRnlcBbSfCtrlInd);
    Msg.m_pSignalPara = static_cast<void*>(&tRnlcBbSfCtrlInd);

    T_CIMVar *ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    
    PID tBBPid = ptCIMVar->tCIMPIDinfo.tBBPid;

    WORD32 dwResult = USF_OSS_SendAsynMsg((WORD16)Msg.m_dwSignal , Msg.m_pSignalPara, Msg.m_wLen, COMM_RELIABLE, &tBBPid);
    /*返回失败打印错误信息*/
    if ( dwResult!= RNLC_SUCC )
    {
        /*打印错误*/
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg, dwResult,\
                                0, RNLC_FATAL_LEVEL, "\n  [HandleDtxNotifyFromCMM]Fail to send Message[%d] to BB! \n",Msg.m_dwSignal);
        return RNLC_FAIL;
    }
    else
    {
        CCM_LOG(RNLC_INFO_LEVEL,"send  EV_RNLC_BB_SF_CTRL_IND sucess.\n");
        return RNLC_SUCC;
    }
}
/*<FUNC>***********************************************************************
* 函数名称: 
* 功能描述: 在各种状态下处理CMD_PROCCTRL_ROLLBACK_REQ消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::SendRusptDtxCfgReqToBB(T_DBS_GetDtxInfoByCellId_ACK *ptDBSGetDtxInfoByCellId_ACK)
{
    CCM_NULL_POINTER_CHECK(ptDBSGetDtxInfoByCellId_ACK); 

    T_RnlcBbRusptdtxCfgReq tRnlcBbRusptdtxCfgReq;
    memset(&tRnlcBbRusptdtxCfgReq, 0, sizeof(tRnlcBbRusptdtxCfgReq));
    
    /*BB请求头*/
    /* 消息长度，指消息的长度，包括MsgLen这个字段本身 */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.wMsgLen = sizeof(tRnlcBbRusptdtxCfgReq);
    /* 消息类型 */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.wMsgType = EV_RNLC_BB_RUSPTDTX_CFG_REQ; 

    /* 流水号 */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.wFlowNo = RnlcGetFlowNumber();
    
    /* 小区ID */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.wL3CellId = GetJobContext()->tCIMVar.wCellId;
    /* 小区板内索引 */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.ucCellIdInBoard = 
                                       GetJobContext()->tCIMVar.ucCellIdInBoard;
    /* 时间戳 */
    tRnlcBbRusptdtxCfgReq.tMsgHeader.wTimeStamp = 0;

    tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx.ucEsEnable = ptDBSGetDtxInfoByCellId_ACK->ucEsSwitch;
    tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx.ucnB = ptDBSGetDtxInfoByCellId_ACK->ucnB;
    tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx.ucRBInterval = ptDBSGetDtxInfoByCellId_ACK->ucRBInterval;
    tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx.ucThrClose = ptDBSGetDtxInfoByCellId_ACK->ucThrClose;
    tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx.ucThrOpen = ptDBSGetDtxInfoByCellId_ACK->ucThrOpen;

    CCM_LOG(RNLC_INFO_LEVEL,"tRnlcBbRusptdtxCfgReq.tRnlcBbRusptdtx:%d,%d,%d,CellId: %d \n",
                     ptDBSGetDtxInfoByCellId_ACK->ucEsSwitch,
                     ptDBSGetDtxInfoByCellId_ACK->ucRBInterval,
                     ptDBSGetDtxInfoByCellId_ACK->ucThrClose,
                     GetJobContext()->tCIMVar.wCellId);


    Message Msg;
    Msg.m_wSourceId = ID_CCM_CIM_MainComponent;
    Msg.m_dwSignal = EV_RNLC_BB_RUSPTDTX_CFG_REQ;
    Msg.m_wLen = sizeof(tRnlcBbRusptdtxCfgReq);
    Msg.m_pSignalPara = static_cast<void*>(&tRnlcBbRusptdtxCfgReq);

    T_CIMVar *ptCIMVar = (T_CIMVar*)&(GetJobContext()->tCIMVar);
    
    PID tBBPid = ptCIMVar->tCIMPIDinfo.tBBPid;
    WORD32 dwResult = USF_OSS_SendAsynMsg((WORD16)Msg.m_dwSignal , Msg.m_pSignalPara, Msg.m_wLen, COMM_RELIABLE, &tBBPid);
    /*返回失败打印错误信息*/
    if ( dwResult!= RNLC_SUCC )
    {
        /*打印错误*/
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_FunctionCallFail_USF_OSS_SendAsynMsg, dwResult,\
                                0, RNLC_FATAL_LEVEL, "\n  [HandleDtxNotifyFromCMM]Fail to send Message[%d] to BB! \n",Msg.m_dwSignal);
        return RNLC_FAIL;
    }
    else
    {
        CCM_LOG(INFO_LEVEL,"send EV_RNLC_BB_RUSPTDTX_CFG_REQ sucess.\n");
        return RNLC_SUCC;
    }
}
/*<FUNC>***********************************************************************
* 函数名称: SendToOamSymbolMapReq(VOID)
* 功能描述:小区建立成功后给OAM发送符号位图
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::SendToOamSymbolMapReq(VOID)
{
    T_RnlcOamSymbleMapReq tRnlcOamSymbleMapReq;
    memset(&tRnlcOamSymbleMapReq, 0, sizeof(tRnlcOamSymbleMapReq));

    T_DBS_GetSuperCpInfo_REQ tDbsGetSuperCpInfoReq;
    T_DBS_GetSuperCpInfo_ACK tDbsGetSuperCpInfoAck;

    memset(&tDbsGetSuperCpInfoReq, 0x00, sizeof(tDbsGetSuperCpInfoReq));
    memset(&tDbsGetSuperCpInfoAck, 0x00, sizeof(tDbsGetSuperCpInfoAck));

    tDbsGetSuperCpInfoReq.wCallType = USF_MSG_CALL;
    tDbsGetSuperCpInfoReq.wCId = GetJobContext()->tCIMVar.wCellId;

    BOOL ucResult = UsfDbsAccess(EV_DBS_GetSuperCpInfo_REQ, (VOID *)&tDbsGetSuperCpInfoReq, (VOID *)&tDbsGetSuperCpInfoAck);
    if ((FALSE == ucResult) || (0 != tDbsGetSuperCpInfoAck.dwResult))
    {
         /*打印错误*/
        return FALSE;
    }

    T_DBS_GetBbAlgParaCfgForV25_REQ tDBSGetBbAlgParaCfgForV25_REQ;
    memset(&tDBSGetBbAlgParaCfgForV25_REQ, 0, sizeof(tDBSGetBbAlgParaCfgForV25_REQ));  
    tDBSGetBbAlgParaCfgForV25_REQ.wCellId = GetJobContext()->tCIMVar.wCellId;
    tDBSGetBbAlgParaCfgForV25_REQ.wCallType = USF_MSG_CALL;
    
    T_DBS_GetBbAlgParaCfgForV25_ACK tDBSGetBbAlgParaCfgForV25_ACK;
    memset(&tDBSGetBbAlgParaCfgForV25_ACK, 0, sizeof(tDBSGetBbAlgParaCfgForV25_ACK));
    BOOLEAN bDbResult = UsfDbsAccess(EV_DBS_GetBbAlgParaCfgForV25_REQ,
                                     static_cast<VOID*>(&tDBSGetBbAlgParaCfgForV25_REQ),
                                     static_cast<VOID*>(&tDBSGetBbAlgParaCfgForV25_ACK));
    if(FALSE == bDbResult || 0 != tDBSGetBbAlgParaCfgForV25_ACK.dwResult)
    {        
        /*lint -save -e666 */
        CCM_CIM_ExceptionReport(ERR_CCM_CIM_DBAccessFail_GetBbAlgParaCfgReq,
                                tDBSGetBbAlgParaCfgForV25_ACK.dwResult,
                                bDbResult,
                                RNLC_FATAL_LEVEL, 
                                " DBAccessFail_GetCellCfgInfoReq, wInstanceNo:%d! ",
                                GetJobContext()->tCIMVar.wInstanceNo);
        /*lint -restore*/
        return ERR_CCM_CIM_DBAccessFail_GetBbAlgParaCfgReq;
    }
    BYTE ucCFIMod = tDBSGetBbAlgParaCfgForV25_ACK.tBbAlgPara.tAlgCfiInfo.ucCFIAdaptMod;

    if (TDD_RADIOMODE == GetJobContext()->tCIMVar.ucRadioMode)
    {
        if(0 < ucCFIMod && 4 > ucCFIMod)
        {
            ucCFIMod = ucCFIMod+1;
        }
        if(0 == ucCFIMod || 4 == ucCFIMod)
        {
            ucCFIMod = 4;
        }
    }

    for(BYTE ucCpLoop = 0;ucCpLoop < tDbsGetSuperCpInfoAck.ucCPNum && ucCpLoop < DBS_MAX_CP_NUM_PER_CID;ucCpLoop++ )
    {
        FillSymbolMapReq(ucCFIMod,tRnlcOamSymbleMapReq,tDbsGetSuperCpInfoAck.atCpInfo[ucCpLoop]);
    }

    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: FillSymbolMapReq
* 功能描述:填写符号位图消息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::FillSymbolMapReq(BYTE ucCFI,T_RnlcOamSymbleMapReq &tRnlcOamSymbleMapReq,const TSuperCPInfo &tCpInfo)
{
    BYTE ucCpType = GetJobContext()->tCIMVar.tCellCfgInfoForV25.tCellCfgBaseInfoForV25.ucPhyChCPSel;

    tRnlcOamSymbleMapReq.wCellId = GetJobContext()->tCIMVar.wCellId;
    tRnlcOamSymbleMapReq.wCpId = tCpInfo.ucCPId;
    tRnlcOamSymbleMapReq.ucRack = tCpInfo.ucRruRackNo;
    tRnlcOamSymbleMapReq.ucShelf = tCpInfo.ucRruShelfNo;
    tRnlcOamSymbleMapReq.ucSlot = tCpInfo.ucRruSlotNo;
    tRnlcOamSymbleMapReq.ucCPType = ucCpType;

    for(BYTE ucLoopAnt = 0; ucLoopAnt < 8; ucLoopAnt++)
    {
        if(15 != tCpInfo.aucDlAntPortMap[ucLoopAnt])
        {
            for(BYTE ucLoopSymbel = 0; ucLoopSymbel < 8; ucLoopSymbel++)
            {
                if(ucCpType == atCcmSymbalMap[ucLoopSymbel].ucCpType &&
                  ucCFI == atCcmSymbalMap[ucLoopSymbel].ucCFI &&
                  tCpInfo.aucDlAntPortMap[ucLoopAnt] < 4)         /*端口号为1-3*/
                {
                    BYTE ucPortIndex = tCpInfo.aucDlAntPortMap[ucLoopAnt];  /*8个元素分别表示1-8天线,取值0xf表示无效天线，取值0-3表示映射到的端口号*/                    

                    tRnlcOamSymbleMapReq.wSymbolBitmap[ucLoopAnt] = atCcmSymbalMap[ucLoopSymbel].awPortWgt[ucPortIndex];
                }
            }
        }
        else
        {
            tRnlcOamSymbleMapReq.wSymbolBitmap[ucLoopAnt] = 0;
        }
        
        CCM_LOG(RNLC_INFO_LEVEL,"wSymbolBitmap[ucLoopAnt]: %d\n",tRnlcOamSymbleMapReq.wSymbolBitmap[ucLoopAnt]);
    }

    PID tOAMPid = GetJobContext()->tCIMVar.tCIMPIDinfo.tOAMPid;
    WORD32 dwOssStatus = USF_OSS_SendAsynMsg(EV_RNLC_OAM_SYMBOLMAP_REQ, 
                                             &tRnlcOamSymbleMapReq, 
                                             sizeof(tRnlcOamSymbleMapReq), 
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
    CCM_LOG(RNLC_INFO_LEVEL,"send  EV_RNLC_OAM_SYMBOLMAP_REQ sucess.\n");
    return RNLC_SUCC;
}

/*<FUNC>***********************************************************************
* 函数名称: InitOamAntSymbel
* 功能描述:初始化符号位图信息
* 算法描述:
* 全局变量:
* Input参数: Message *pMsg 消息指针
* 返 回 值:WORD32,操作成功或失败
**    
* 完成日期: 
* 版    本: v1.0
* 修改记录:
*    
* -------------------------------------------------------------------------
**<FUNC>**********************************************************************/
WORD32  CCM_CIM_MainComponentFsm::InitOamAntSymbel(VOID)
{
    memset(atCcmSymbalMap,0xFF,8*sizeof(T_CcmSymbalMap));

    /* 方案中的SymbolBitMap取值表*/
    WORD16 awSymbelTemp[8][4] = {{46958,46958,48892,48892},
                                                      {46956,46956,48892,48892},
                                                      {46952,46952,48888,48888},
                                                      {46944,46944,48880,48880},
                                                      {36278,36278,36732,36732},
                                                      {36276,36276,36732,36732},
                                                      {36272,36272,36728,36728},
                                                      {36272,36272,36720,36720}};

    /*将取之表转化存入到实例数据区*/
    for(BYTE ucLoop = 0;ucLoop < 8;ucLoop++)
    {
        atCcmSymbalMap[ucLoop].ucCpType  = (ucLoop<4)?0:1;
        atCcmSymbalMap[ucLoop].ucCFI = ucLoop % 4 +1;

        for(BYTE ucPortIndex = 0;ucPortIndex < 4;ucPortIndex++)
        {
            atCcmSymbalMap[ucLoop].awPortWgt[ucPortIndex] = awSymbelTemp[ucLoop][ucPortIndex];
        }
    }
    return RNLC_SUCC;
}


