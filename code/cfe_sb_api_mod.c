/* Functions with large portions taken from the original cfe_sb_api.c.
 * Modifications are minimal so that they should diff nicely with future
 * versions.
 */
/*
 * Copyright (c) 2018 NSF Center for Space, High-performance, and Resilient Computing (SHREC)
 * University of Pittsburgh. All rights reserved.
 *
 * Copyright (c) 2004-2012, United States government as represented by the
 * administrator of the National Aeronautics Space Administration.
 * All rights reserved. This software(cFE) was created at NASA's Goddard
 * Space Flight Center pursuant to government contracts.
 *
 * This is governed by the NASA Open Source Agreement and may be used,
 * distributed and modified only pursuant to the terms of that agreement.
 */
#include <string.h>
#include <cfe.h>
#include <cfe_sb_events.h>
#include "cfe_sb_priv.h"
#include "cfe_sb_api_mod.h"

/*
 * Macro to reflect size of PipeDepthStats Telemetry array -
 * this may or may not be the same as CFE_SB_MSG_MAX_PIPES
 */
#define CFE_SB_TLM_PIPEDEPTHSTATS_SIZE (sizeof(CFE_SB.StatTlmMsg.Payload.PipeDepthStats) / sizeof(CFE_SB.StatTlmMsg.Payload.PipeDepthStats[0]))

/* Original code from CFE_SB_SendMsgFull. */
int32  CFE_SB_SendMsgFullOnRecv(CFE_SB_Msg_t    *MsgPtr,
                                uint32           TlmCntIncrements,
                                uint32           CopyMode,
                                const char      *SenderName,
                                unsigned long    ProcessorId)
{
/*
 * All calls to CFE_SB_GetAppTskName have the same arguments in this function.
 * These macros are workarounds for avoiding modification of the original
 * source.
 */
#define CFE_SB_GetAppTskName(TskId, FullName) (SenderName)
#define CFE_PSP_GetProcessorId(a) (ProcessorId)
    CFE_SB_MsgId_t          MsgId;
    int32                   Status;
    CFE_SB_DestinationD_t   *DestPtr = NULL;
    CFE_SB_PipeD_t          *PipeDscPtr;
    CFE_SB_RouteEntry_t     *RtgTblPtr;
    CFE_SB_BufferD_t        *BufDscPtr;
    uint16                  TotalMsgSize;
    uint16                  RtgTblIdx;
    uint32                  TskId = 0;
    uint16                  i;
    char                    FullName[(OS_MAX_API_NAME * 2)];
    CFE_SB_EventBuf_t       SBSndErr;

    SBSndErr.EvtsToSnd = 0;

    /* get task id for events and Sender Info*/
    TskId = OS_TaskGetId();

    /* check input parameter */
    if(MsgPtr == NULL){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_SEND_BAD_ARG_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Bad input argument,Arg 0x%lx,App %s",
            (unsigned long)MsgPtr,CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    MsgId = CFE_SB_GetMsgId(MsgPtr);

    /* validate the msgid in the message */
    if(CFE_SB_ValidateMsgId(MsgId) != CFE_SUCCESS){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
        if (CopyMode == CFE_SB_SEND_ZEROCOPY)
        {
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_SEND_INV_MSGID_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Invalid MsgId(0x%x)in msg,App %s",
            MsgId,CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    TotalMsgSize = CFE_SB_GetTotalMsgLength(MsgPtr);

    /* Verify the size of the pkt is < or = the mission defined max */
    if(TotalMsgSize > CFE_SB_MAX_SB_MSG_SIZE){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
#if 0
        if (CopyMode == CFE_SB_SEND_ZEROCOPY)
        {
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }
#endif
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_MSG_TOO_BIG_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Msg Too Big MsgId=0x%x,app=%s,size=%d,MaxSz=%d",
            MsgId,CFE_SB_GetAppTskName(TskId,FullName),TotalMsgSize,CFE_SB_MAX_SB_MSG_SIZE);
        return CFE_SB_MSG_TOO_BIG;
    }/* end if */

    /* take semaphore to prevent a task switch during this call */
    CFE_SB_LockSharedData(__func__,__LINE__);

    RtgTblIdx = CFE_SB_GetRoutingTblIdx(MsgId);

    /* if there have been no subscriptions for this pkt, */
    /* increment the dropped pkt cnt, send event and return success */
    if(RtgTblIdx == CFE_SB_AVAILABLE){

        CFE_SB.HKTlmMsg.Payload.NoSubscribersCnt++;

#if 0
        if (CopyMode == CFE_SB_SEND_ZEROCOPY){
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }
#endif

        CFE_SB_UnlockSharedData(__func__,__LINE__);

        /* Determine if event can be sent without causing recursive event problem */
        if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_SEND_NO_SUBS_EID_BIT) == CFE_SB_GRANTED){

           CFE_EVS_SendEventWithAppID(CFE_SB_SEND_NO_SUBS_EID,CFE_EVS_INFORMATION,CFE_SB.AppId,
              "No subscribers for MsgId 0x%x,sender %s",
              MsgId,CFE_SB_GetAppTskName(TskId,FullName));

           /* clear the bit so the task may send this event again */
           CFE_SB_FinishSendEvent(TskId,CFE_SB_SEND_NO_SUBS_EID_BIT);
        }/* end if */

        return CFE_SUCCESS;
    }/* end if */

#if 0
    /* Allocate a new buffer. */
    if (CopyMode == CFE_SB_SEND_ZEROCOPY){
        BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
    }
    else{
        BufDscPtr = CFE_SB_GetBufferFromPool(MsgId, TotalMsgSize);
    }
#else
    BufDscPtr = CFE_SB_GetBufferFromPool(MsgId, TotalMsgSize);
#endif
    if (BufDscPtr == NULL){
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);

        /* Determine if event can be sent without causing recursive event problem */
        if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_GET_BUF_ERR_EID_BIT) == CFE_SB_GRANTED){

            CFE_EVS_SendEventWithAppID(CFE_SB_GET_BUF_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
              "Send Err:Request for Buffer Failed. MsgId 0x%x,app %s,size %d",
              MsgId,CFE_SB_GetAppTskName(TskId,FullName),TotalMsgSize);

            /* clear the bit so the task may send this event again */
            CFE_SB_FinishSendEvent(TskId,CFE_SB_GET_BUF_ERR_EID_BIT);
        }/* end if */

        return CFE_SB_BUF_ALOC_ERR;
    }/* end if */

    /* Copy the packet into the SB memory space */
#if 0
    if (CopyMode != CFE_SB_SEND_ZEROCOPY){
        /* Copy the packet into the SB memory space */
        CFE_PSP_MemCpy( BufDscPtr->Buffer, MsgPtr, (uint16)TotalMsgSize );
    }
#else
    CFE_PSP_MemCpy( BufDscPtr->Buffer, MsgPtr, (uint16)TotalMsgSize );
#endif

    /* For Tlm packets, increment the seq count if requested */
    if((CFE_SB_GetPktType(MsgId)==CFE_SB_TLM) &&
       (TlmCntIncrements==CFE_SB_INCREMENT_TLM)){
        CFE_SB.RoutingTbl[RtgTblIdx].SeqCnt++;
        CFE_SB_SetMsgSeqCnt((CFE_SB_Msg_t *)BufDscPtr->Buffer,
                              CFE_SB.RoutingTbl[RtgTblIdx].SeqCnt);
    }/* end if */

    /* store the sender information */
    if(CFE_SB.SenderReporting != 0)
    {
       BufDscPtr->Sender.ProcessorId = CFE_PSP_GetProcessorId();
#if 0 // Bugfix
       strncpy(&BufDscPtr->Sender.AppName[0],CFE_SB_GetAppTskName(TskId,FullName),OS_MAX_API_NAME);
#else
       strncpy(BufDscPtr->Sender.AppName, SenderName,
               sizeof(BufDscPtr->Sender.AppName) - 1);
       BufDscPtr->Sender.AppName[sizeof(BufDscPtr->Sender.AppName) - 1] =
         '\0';
#endif
    }

    RtgTblPtr = &CFE_SB.RoutingTbl[RtgTblIdx];

    /* At this point there must be at least one destination for pkt */

    DestPtr = RtgTblPtr -> ListHeadPtr;

    /* Send the packet to all destinations  */
    for (i=0; i < RtgTblPtr -> Destinations; i++) {

        /* The DestPtr should never be NULL in this loop, this is just extra
           protection in case of the unforseen */
        if(DestPtr == NULL){
          break;
        }

        if (DestPtr->Active != CFE_SB_INACTIVE)    /* destination is active */
        {

        PipeDscPtr = &CFE_SB.PipeTbl[DestPtr->PipeId];

        /* if Msg limit exceeded, log event, increment counter */
        /* and go to next destination */
        if(DestPtr->BuffCount >= DestPtr->MsgId2PipeLim){

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_MSGID_LIM_ERR_EID;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.MsgLimErrCnt++;
            PipeDscPtr->SendErrors++;

            }else{
        /*
        ** Write the buffer descriptor to the queue of the pipe.  If the write
        ** failed, log info and increment the pipe's error counter.
        */
                Status = OS_QueuePut(PipeDscPtr->SysQueueId,(void *)&BufDscPtr,
                                     sizeof(CFE_SB_BufferD_t *),0);

        if (Status == OS_SUCCESS) {
            BufDscPtr->UseCount++;    /* used for releasing buffer  */
            DestPtr->BuffCount++; /* used for checking MsgId2PipeLimit */
            DestPtr->DestCnt++;   /* used for statistics */
            if (DestPtr->PipeId < CFE_SB_TLM_PIPEDEPTHSTATS_SIZE)
            {
                CFE_SB_PipeDepthStats_t *StatObj =
                        &CFE_SB.StatTlmMsg.Payload.PipeDepthStats[DestPtr->PipeId];
                StatObj->InUse++;
                if(StatObj->InUse > StatObj->PeakInUse){
                    StatObj->PeakInUse = StatObj->InUse;
            }/* end if */
            }

        }else if(Status == OS_QUEUE_FULL) {

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_Q_FULL_ERR_EID;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.PipeOverflowErrCnt++;
            PipeDscPtr->SendErrors++;


        }else{ /* Unexpected error while writing to queue. */

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_Q_WR_ERR_EID;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].ErrStat = Status;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.InternalErrCnt++;
            PipeDscPtr->SendErrors++;

                }/*end if */
            }/*end if */
        }/*end if */

        DestPtr = DestPtr -> Next;

    } /* end loop over destinations */

    /*
    ** Decrement the buffer UseCount and free buffer if cnt=0. This decrement is done
    ** because the use cnt is initialized to 1 in CFE_SB_GetBufferFromPool.
    ** Initializing the count to 1 (as opposed to zero) and decrementing it here are
    ** done to ensure the buffer gets released when there are destinations that have
    ** been disabled via ground command.
    */
    CFE_SB_DecrBufUseCnt(BufDscPtr);

    /* release the semaphore */
    CFE_SB_UnlockSharedData(__func__,__LINE__);


    /* send an event for each pipe write error that may have occurred */
    for(i=0;i < SBSndErr.EvtsToSnd; i++)
    {
        if(SBSndErr.EvtBuf[i].EventId == CFE_SB_MSGID_LIM_ERR_EID)
        {
            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_MSGID_LIM_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_ES_PerfLogEntry(CFE_SB_MSG_LIM_PERF_ID);
              CFE_ES_PerfLogExit(CFE_SB_MSG_LIM_PERF_ID);

              CFE_EVS_SendEventWithAppID(CFE_SB_MSGID_LIM_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                "Msg Limit Err,MsgId 0x%x,pipe %s,sender %s",
                (unsigned int)RtgTblPtr->MsgId,
                CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                CFE_SB_GetAppTskName(TskId,FullName));

              /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_MSGID_LIM_ERR_EID_BIT);
            }/* end if */

        }else if(SBSndErr.EvtBuf[i].EventId == CFE_SB_Q_FULL_ERR_EID){

            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_Q_FULL_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_ES_PerfLogEntry(CFE_SB_PIPE_OFLOW_PERF_ID);
              CFE_ES_PerfLogExit(CFE_SB_PIPE_OFLOW_PERF_ID);

              CFE_EVS_SendEventWithAppID(CFE_SB_Q_FULL_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                  "Pipe Overflow,MsgId 0x%x,pipe %s,sender %s",
                  (unsigned int)RtgTblPtr->MsgId,
                  CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                  CFE_SB_GetAppTskName(TskId,FullName));

               /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_Q_FULL_ERR_EID_BIT);
            }/* end if */

        }else{

            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_Q_WR_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_EVS_SendEventWithAppID(CFE_SB_Q_WR_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                "Pipe Write Err,MsgId 0x%x,pipe %s,sender %s,stat 0x%x",
                (unsigned int)RtgTblPtr->MsgId,
                CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                CFE_SB_GetAppTskName(TskId,FullName),
                (unsigned int)SBSndErr.EvtBuf[i].ErrStat);

               /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_Q_WR_ERR_EID_BIT);
            }/* end if */

        }/* end if */
    }


    return CFE_SUCCESS;

#undef CFE_PSP_GetProcessorId
#undef CFE_SB_GetAppTskName
}

/* Original code from CFE_SB_SendMsgFull. */
int32  CFE_SB_SendMsgFullOnSend(CFE_SB_Msg_t    *MsgPtr,
                                uint32           TlmCntIncrements,
                                uint32           CopyMode)
{
    CFE_SB_MsgId_t          MsgId;
    int32                   Status;
    CFE_SB_DestinationD_t   *DestPtr = NULL;
    CFE_SB_PipeD_t          *PipeDscPtr;
    CFE_SB_RouteEntry_t     *RtgTblPtr;
    CFE_SB_BufferD_t        *BufDscPtr;
    uint16                  TotalMsgSize;
    uint16                  RtgTblIdx;
    uint32                  TskId = 0;
    uint16                  i;
    char                    FullName[(OS_MAX_API_NAME * 2)];
    CFE_SB_EventBuf_t       SBSndErr;

    SBSndErr.EvtsToSnd = 0;

    /* get task id for events and Sender Info*/
    TskId = OS_TaskGetId();

    /* check input parameter */
    if(MsgPtr == NULL){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_SEND_BAD_ARG_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Bad input argument,Arg 0x%lx,App %s",
            (unsigned long)MsgPtr,CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    MsgId = CFE_SB_GetMsgId(MsgPtr);

    /* validate the msgid in the message */
    if(CFE_SB_ValidateMsgId(MsgId) != CFE_SUCCESS){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
#if 0
        if (CopyMode == CFE_SB_SEND_ZEROCOPY)
        {
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }
#endif
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_SEND_INV_MSGID_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Invalid MsgId(0x%x)in msg,App %s",
            MsgId,CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    TotalMsgSize = CFE_SB_GetTotalMsgLength(MsgPtr);

    /* Verify the size of the pkt is < or = the mission defined max */
    if(TotalMsgSize > CFE_SB_MAX_SB_MSG_SIZE){
        CFE_SB_LockSharedData(__func__,__LINE__);
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
#if 0
        if (CopyMode == CFE_SB_SEND_ZEROCOPY)
        {
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }
#endif
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_MSG_TOO_BIG_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Send Err:Msg Too Big MsgId=0x%x,app=%s,size=%d,MaxSz=%d",
            MsgId,CFE_SB_GetAppTskName(TskId,FullName),TotalMsgSize,CFE_SB_MAX_SB_MSG_SIZE);
        return CFE_SB_MSG_TOO_BIG;
    }/* end if */

#if 0
    /* take semaphore to prevent a task switch during this call */
    CFE_SB_LockSharedData(__func__,__LINE__);

    RtgTblIdx = CFE_SB_GetRoutingTblIdx(MsgId);

    /* if there have been no subscriptions for this pkt, */
    /* increment the dropped pkt cnt, send event and return success */
    if(RtgTblIdx == CFE_SB_AVAILABLE){

        CFE_SB.HKTlmMsg.Payload.NoSubscribersCnt++;

        if (CopyMode == CFE_SB_SEND_ZEROCOPY){
            BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
            CFE_SB_DecrBufUseCnt(BufDscPtr);
        }

        CFE_SB_UnlockSharedData(__func__,__LINE__);

        /* Determine if event can be sent without causing recursive event problem */
        if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_SEND_NO_SUBS_EID_BIT) == CFE_SB_GRANTED){

           CFE_EVS_SendEventWithAppID(CFE_SB_SEND_NO_SUBS_EID,CFE_EVS_INFORMATION,CFE_SB.AppId,
              "No subscribers for MsgId 0x%x,sender %s",
              MsgId,CFE_SB_GetAppTskName(TskId,FullName));

           /* clear the bit so the task may send this event again */
           CFE_SB_FinishSendEvent(TskId,CFE_SB_SEND_NO_SUBS_EID_BIT);
        }/* end if */

        return CFE_SUCCESS;
    }/* end if */

    /* Allocate a new buffer. */
    if (CopyMode == CFE_SB_SEND_ZEROCOPY){
        BufDscPtr = CFE_SB_GetBufferFromCaller(MsgId, MsgPtr);
    }
    else{
        BufDscPtr = CFE_SB_GetBufferFromPool(MsgId, TotalMsgSize);
    }
    if (BufDscPtr == NULL){
        CFE_SB.HKTlmMsg.Payload.MsgSendErrCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);

        /* Determine if event can be sent without causing recursive event problem */
        if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_GET_BUF_ERR_EID_BIT) == CFE_SB_GRANTED){

            CFE_EVS_SendEventWithAppID(CFE_SB_GET_BUF_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
              "Send Err:Request for Buffer Failed. MsgId 0x%x,app %s,size %d",
              MsgId,CFE_SB_GetAppTskName(TskId,FullName),TotalMsgSize);

            /* clear the bit so the task may send this event again */
            CFE_SB_FinishSendEvent(TskId,CFE_SB_GET_BUF_ERR_EID_BIT);
        }/* end if */

        return CFE_SB_BUF_ALOC_ERR;
    }/* end if */

    /* Copy the packet into the SB memory space */
    if (CopyMode != CFE_SB_SEND_ZEROCOPY){
        /* Copy the packet into the SB memory space */
        CFE_PSP_MemCpy( BufDscPtr->Buffer, MsgPtr, (uint16)TotalMsgSize );
    }

    /* For Tlm packets, increment the seq count if requested */
    if((CFE_SB_GetPktType(MsgId)==CFE_SB_TLM) &&
       (TlmCntIncrements==CFE_SB_INCREMENT_TLM)){
        CFE_SB.RoutingTbl[RtgTblIdx].SeqCnt++;
        CFE_SB_SetMsgSeqCnt((CFE_SB_Msg_t *)BufDscPtr->Buffer,
                              CFE_SB.RoutingTbl[RtgTblIdx].SeqCnt);
    }/* end if */

    /* store the sender information */
    if(CFE_SB.SenderReporting != 0)
    {
       BufDscPtr->Sender.ProcessorId = CFE_PSP_GetProcessorId();
       strncpy(&BufDscPtr->Sender.AppName[0],CFE_SB_GetAppTskName(TskId,FullName),OS_MAX_API_NAME);
    }

    RtgTblPtr = &CFE_SB.RoutingTbl[RtgTblIdx];

    /* At this point there must be at least one destination for pkt */

    DestPtr = RtgTblPtr -> ListHeadPtr;

    /* Send the packet to all destinations  */
    for (i=0; i < RtgTblPtr -> Destinations; i++) {

        /* The DestPtr should never be NULL in this loop, this is just extra
           protection in case of the unforseen */
        if(DestPtr == NULL){
          break;
        }

        if (DestPtr->Active != CFE_SB_INACTIVE)    /* destination is active */
        {

        PipeDscPtr = &CFE_SB.PipeTbl[DestPtr->PipeId];

        /* if Msg limit exceeded, log event, increment counter */
        /* and go to next destination */
        if(DestPtr->BuffCount >= DestPtr->MsgId2PipeLim){

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_MSGID_LIM_ERR_EID;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.MsgLimErrCnt++;
            PipeDscPtr->SendErrors++;

            }else{
        /*
        ** Write the buffer descriptor to the queue of the pipe.  If the write
        ** failed, log info and increment the pipe's error counter.
        */
                Status = OS_QueuePut(PipeDscPtr->SysQueueId,(void *)&BufDscPtr,
                                     sizeof(CFE_SB_BufferD_t *),0);

        if (Status == OS_SUCCESS) {
            BufDscPtr->UseCount++;    /* used for releasing buffer  */
            DestPtr->BuffCount++; /* used for checking MsgId2PipeLimit */
            DestPtr->DestCnt++;   /* used for statistics */
            if (DestPtr->PipeId < CFE_SB_TLM_PIPEDEPTHSTATS_SIZE)
            {
                CFE_SB_PipeDepthStats_t *StatObj =
                        &CFE_SB.StatTlmMsg.Payload.PipeDepthStats[DestPtr->PipeId];
                StatObj->InUse++;
                if(StatObj->InUse > StatObj->PeakInUse){
                    StatObj->PeakInUse = StatObj->InUse;
            }/* end if */
            }

        }else if(Status == OS_QUEUE_FULL) {

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_Q_FULL_ERR_EID;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.PipeOverflowErrCnt++;
            PipeDscPtr->SendErrors++;


        }else{ /* Unexpected error while writing to queue. */

            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].PipeId  = DestPtr->PipeId;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].EventId = CFE_SB_Q_WR_ERR_EID;
            SBSndErr.EvtBuf[SBSndErr.EvtsToSnd].ErrStat = Status;
            SBSndErr.EvtsToSnd++;
            CFE_SB.HKTlmMsg.Payload.InternalErrCnt++;
            PipeDscPtr->SendErrors++;

                }/*end if */
            }/*end if */
        }/*end if */

        DestPtr = DestPtr -> Next;

    } /* end loop over destinations */

    /*
    ** Decrement the buffer UseCount and free buffer if cnt=0. This decrement is done
    ** because the use cnt is initialized to 1 in CFE_SB_GetBufferFromPool.
    ** Initializing the count to 1 (as opposed to zero) and decrementing it here are
    ** done to ensure the buffer gets released when there are destinations that have
    ** been disabled via ground command.
    */
    CFE_SB_DecrBufUseCnt(BufDscPtr);

    /* release the semaphore */
    CFE_SB_UnlockSharedData(__func__,__LINE__);


    /* send an event for each pipe write error that may have occurred */
    for(i=0;i < SBSndErr.EvtsToSnd; i++)
    {
        if(SBSndErr.EvtBuf[i].EventId == CFE_SB_MSGID_LIM_ERR_EID)
        {
            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_MSGID_LIM_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_ES_PerfLogEntry(CFE_SB_MSG_LIM_PERF_ID);
              CFE_ES_PerfLogExit(CFE_SB_MSG_LIM_PERF_ID);

              CFE_EVS_SendEventWithAppID(CFE_SB_MSGID_LIM_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                "Msg Limit Err,MsgId 0x%x,pipe %s,sender %s",
                (unsigned int)RtgTblPtr->MsgId,
                CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                CFE_SB_GetAppTskName(TskId,FullName));

              /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_MSGID_LIM_ERR_EID_BIT);
            }/* end if */

        }else if(SBSndErr.EvtBuf[i].EventId == CFE_SB_Q_FULL_ERR_EID){

            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_Q_FULL_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_ES_PerfLogEntry(CFE_SB_PIPE_OFLOW_PERF_ID);
              CFE_ES_PerfLogExit(CFE_SB_PIPE_OFLOW_PERF_ID);

              CFE_EVS_SendEventWithAppID(CFE_SB_Q_FULL_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                  "Pipe Overflow,MsgId 0x%x,pipe %s,sender %s",
                  (unsigned int)RtgTblPtr->MsgId,
                  CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                  CFE_SB_GetAppTskName(TskId,FullName));

               /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_Q_FULL_ERR_EID_BIT);
            }/* end if */

        }else{

            /* Determine if event can be sent without causing recursive event problem */
            if(CFE_SB_RequestToSendEvent(TskId,CFE_SB_Q_WR_ERR_EID_BIT) == CFE_SB_GRANTED){

              CFE_EVS_SendEventWithAppID(CFE_SB_Q_WR_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
                "Pipe Write Err,MsgId 0x%x,pipe %s,sender %s,stat 0x%x",
                (unsigned int)RtgTblPtr->MsgId,
                CFE_SB_GetPipeName(SBSndErr.EvtBuf[i].PipeId),
                CFE_SB_GetAppTskName(TskId,FullName),
                (unsigned int)SBSndErr.EvtBuf[i].ErrStat);

               /* clear the bit so the task may send this event again */
              CFE_SB_FinishSendEvent(TskId,CFE_SB_Q_WR_ERR_EID_BIT);
            }/* end if */

        }/* end if */
    }
#endif


    return CFE_SUCCESS;

}

/* Original code from CFE_SB_SubscribeFull. */
int32  CFE_SB_SubscribeFullInternal(CFE_SB_MsgId_t   MsgId,
                                    CFE_SB_PipeId_t  PipeId,
                                    CFE_SB_Qos_t     Quality,
                                    uint16           MsgLim,
                                    uint8            Scope)
{
    uint16 Idx;
    int32  Stat;
    uint32 TskId = 0;
    uint32 AppId = 0xFFFFFFFF;
    uint8  PipeIdx;
    CFE_SB_DestinationD_t *DestBlkPtr = NULL;
    char   FullName[(OS_MAX_API_NAME * 2)];

    /* take semaphore to prevent a task switch during this call */
    CFE_SB_LockSharedData(__func__,__LINE__);

    /* get task id for events */
    TskId = OS_TaskGetId();

    /* get the callers Application Id */
    CFE_ES_GetAppID(&AppId);

    /* check that the pipe has been created */
    PipeIdx = CFE_SB_GetPipeIdx(PipeId);
    if(PipeIdx==CFE_SB_INVALID_PIPE){
      CFE_SB.HKTlmMsg.Payload.SubscribeErrCnt++;
      CFE_SB_UnlockSharedData(__func__,__LINE__);
      CFE_EVS_SendEventWithAppID(CFE_SB_SUB_INV_PIPE_EID,CFE_EVS_ERROR,CFE_SB.AppId,
          "Subscribe Err:Invalid Pipe Id,Msg=0x%x,PipeId=%d,App %s",MsgId,PipeId,
          CFE_SB_GetAppTskName(TskId,FullName));
      return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    /* check that the requestor is the owner of the pipe */
    if(CFE_SB.PipeTbl[PipeIdx].AppId != AppId){
      CFE_SB.HKTlmMsg.Payload.SubscribeErrCnt++;
      CFE_SB_UnlockSharedData(__func__,__LINE__);
      CFE_EVS_SendEventWithAppID(CFE_SB_SUB_INV_CALLER_EID,CFE_EVS_ERROR,CFE_SB.AppId,
          "Subscribe Err:Caller(%s) is not the owner of pipe %d,Msg=0x%x",
          CFE_SB_GetAppTskName(TskId,FullName),PipeId,MsgId);
      return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    /* check message id and scope */
    if((CFE_SB_ValidateMsgId(MsgId) != CFE_SUCCESS)||(Scope > 1))
    {
        CFE_SB.HKTlmMsg.Payload.SubscribeErrCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_SUB_ARG_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
          "Subscribe Err:Bad Arg,MsgId 0x%x,PipeId %d,app %s,scope %d",
          MsgId,PipeId,CFE_SB_GetAppTskName(TskId,FullName),Scope);
        return CFE_SB_BAD_ARGUMENT;
    }/* end if */

    /* check for duplicate subscription */
    if(CFE_SB_DuplicateSubscribeCheck(MsgId,PipeId)==CFE_SB_DUPLICATE){
        CFE_SB.HKTlmMsg.Payload.DupSubscriptionsCnt++;
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_DUP_SUBSCRIP_EID,CFE_EVS_INFORMATION,CFE_SB.AppId,
          "Duplicate Subscription,MsgId 0x%x on %s pipe,app %s",
           MsgId,CFE_SB_GetPipeName(PipeId),CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SUCCESS;
    }/* end if */

    /*
    ** If there has been a subscription for this message id earlier,
    ** get the element number in the routing table.
    */
    Idx = CFE_SB_GetRoutingTblIdx(MsgId);

    /* if first subscription for this message... */
    if(Idx==CFE_SB_AVAILABLE){

        /* Get the index to the first available element in the routing table */
        Idx = CFE_SB_GetAvailRoutingIdx();

        /* if all routing table elements are used, send event */
        if(Idx >= CFE_SB_MAX_MSG_IDS){
            CFE_SB_UnlockSharedData(__func__,__LINE__);
            CFE_EVS_SendEventWithAppID(CFE_SB_MAX_MSGS_MET_EID,CFE_EVS_ERROR,CFE_SB.AppId,
              "Subscribe Err:Max Msgs(%d)In Use,MsgId 0x%x,pipe %s,app %s",
              CFE_SB_MAX_MSG_IDS,MsgId,CFE_SB_GetPipeName(PipeId),CFE_SB_GetAppTskName(TskId,FullName));
            return CFE_SB_MAX_MSGS_MET;
        }/* end if */

        /* Increment the MsgIds in use ctr and if it's > the high water mark,*/
        /* adjust the high water mark */
        CFE_SB.StatTlmMsg.Payload.MsgIdsInUse++;
        if(CFE_SB.StatTlmMsg.Payload.MsgIdsInUse > CFE_SB.StatTlmMsg.Payload.PeakMsgIdsInUse){
           CFE_SB.StatTlmMsg.Payload.PeakMsgIdsInUse = CFE_SB.StatTlmMsg.Payload.MsgIdsInUse;
        }/* end if */

        /* populate the look up table with the routing table index */
        CFE_SB_SetRoutingTblIdx(MsgId,Idx);

        /* label the new routing block with the message identifier */
        CFE_SB.RoutingTbl[Idx].MsgId = MsgId;

    }/* end if */

    if(CFE_SB.RoutingTbl[Idx].Destinations >= CFE_SB_MAX_DEST_PER_PKT){
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_MAX_DESTS_MET_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Subscribe Err:Max Dests(%d)In Use For Msg 0x%x,pipe %s,app %s",
             CFE_SB_MAX_DEST_PER_PKT,MsgId,CFE_SB_GetPipeName(PipeId),
             CFE_SB_GetAppTskName(TskId,FullName));
        return CFE_SB_MAX_DESTS_MET;
    }/* end if */

    DestBlkPtr = CFE_SB_GetDestinationBlk();
    if(DestBlkPtr == NULL){
        CFE_SB_UnlockSharedData(__func__,__LINE__);
        CFE_EVS_SendEventWithAppID(CFE_SB_DEST_BLK_ERR_EID,CFE_EVS_ERROR,CFE_SB.AppId,
            "Subscribe Err:Request for Destination Blk failed for Msg 0x%x", (unsigned int)MsgId);
        return CFE_SB_BUF_ALOC_ERR;
    }/* end if */

    /* initialize destination block */
    DestBlkPtr -> PipeId = PipeId;
    DestBlkPtr -> MsgId2PipeLim = (uint16)MsgLim;
    DestBlkPtr -> Active = CFE_SB_ACTIVE;
    DestBlkPtr -> BuffCount = 0;
    DestBlkPtr -> DestCnt = 0;
    DestBlkPtr -> Scope = Scope;
    DestBlkPtr -> Prev = NULL;
    DestBlkPtr -> Next = NULL;

    /* add destination block to head of list */
    CFE_SB_AddDest(Idx, DestBlkPtr);

    CFE_SB.RoutingTbl[Idx].Destinations++;

    CFE_SB.StatTlmMsg.Payload.SubscriptionsInUse++;
    if(CFE_SB.StatTlmMsg.Payload.SubscriptionsInUse > CFE_SB.StatTlmMsg.Payload.PeakSubscriptionsInUse)
    {
       CFE_SB.StatTlmMsg.Payload.PeakSubscriptionsInUse = CFE_SB.StatTlmMsg.Payload.SubscriptionsInUse;
    }/* end if */

    if((CFE_SB.SubscriptionReporting == CFE_SB_ENABLE)&&(Scope==CFE_SB_GLOBAL)){
      CFE_SB.SubRprtMsg.Payload.MsgId = MsgId;
      CFE_SB.SubRprtMsg.Payload.Pipe = PipeId;
      CFE_SB.SubRprtMsg.Payload.Qos.Priority = Quality.Priority;
      CFE_SB.SubRprtMsg.Payload.Qos.Reliability = Quality.Reliability;
      CFE_SB.SubRprtMsg.Payload.SubType = CFE_SB_SUBSCRIPTION;
      CFE_SB_UnlockSharedData(__func__,__LINE__);
      Stat = CFE_SB_SendMsg((CFE_SB_Msg_t *)&CFE_SB.SubRprtMsg);
      CFE_EVS_SendEventWithAppID(CFE_SB_SUBSCRIPTION_RPT_EID,CFE_EVS_DEBUG,CFE_SB.AppId,
            "Sending Subscription Report Msg=0x%x,Pipe=%d,Stat=0x%x",
            (unsigned int)MsgId,(int)PipeId,(unsigned int)Stat);
      CFE_SB_LockSharedData(__func__,__LINE__);/* to prevent back-to-back unlock */
    }/* end if */

    /* release the semaphore */
    CFE_SB_UnlockSharedData(__func__,__LINE__);

#if 0 /* In SBD the subscription is not finished yet. */
    CFE_EVS_SendEventWithAppID(CFE_SB_SUBSCRIPTION_RCVD_EID,CFE_EVS_DEBUG,CFE_SB.AppId,
        "Subscription Rcvd:MsgId 0x%x on %s(%d),app %s",
         (unsigned int)MsgId,CFE_SB_GetPipeName(PipeId),(int)PipeId,CFE_SB_GetAppTskName(TskId,FullName));
#endif

    return CFE_SUCCESS;

}
