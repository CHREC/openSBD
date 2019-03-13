/*
 * Copyright (c) 2018 NSF Center for Space, High-performance, and Resilient Computing (SHREC)
 * University of Pittsburgh. All rights reserved.
 *
 * This is governed by the NASA Open Source Agreement and may be used,
 * distributed and modified only pursuant to the terms of that agreement.
 */
#ifndef CFE_SB_API_MOD_H
#define CFE_SB_API_MOD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cfe.h>

/* This is used on a node receiving a message, since the original
 * CFE_SB_SendMsgFull depends on the sender and receiver existing on the same
 * host.
 */
int32 CFE_SB_SendMsgFullOnRecv(CFE_SB_Msg_t *MsgPtr, uint32 TlmCntIncrements,
                               uint32 CopyMode, const char *SenderName,
                               unsigned long ProcessorId);

/* Performs validation of the message and sends events. */
int32 CFE_SB_SendMsgFullOnSend(CFE_SB_Msg_t *MsgPtr, uint32 TlmCntIncrements,
                               uint32 CopyMode);

/* Performs normal pipe operations for subscription. */
int32 CFE_SB_SubscribeFullInternal(CFE_SB_MsgId_t MsgId,
                                   CFE_SB_PipeId_t PipeId,
                                   CFE_SB_Qos_t Quality, uint16 MsgLim,
                                   uint8 Scope);

#ifdef __cplusplus
}
#endif

#endif /* not CFE_SB_API_MOD_H */
