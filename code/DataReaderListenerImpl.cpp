// Copyright (c) 2018 NSF Center for Space, High-performance, and Resilient Computing (SHREC)
// University of Pittsburgh. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided
// that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
// OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of SHREC.
//
// Authors: Patrick Gauvin <patrick.gauvin@chrec.org> <pggauvin@gmail.com>
// Authors: Christopher Manderino <christopher.manderino@nsf-shrec.org> <cmanderino@gmail.com>
// Authors: Stephen Snow 

#include <iostream>
#include "DataReaderListenerImpl.h"

extern "C" {
#include <cfe.h>
#include "cfe_sb_priv.h"
#include "cfe_sb_api_mod.h"
#include "cfe_sb_events.h"
}

#define DEBUG_MSG(eid, etype, fmt, ...) do \
{ \
  uint32 _task_id = OS_TaskGetId(); \
  bool _event_ok = \
    CFE_SB_RequestToSendEvent(_task_id, CFE_SB_GET_BUF_ERR_EID_BIT) \
    == CFE_SB_GRANTED; \
  if (_event_ok) \
    CFE_EVS_SendEventWithAppID((eid), (etype), CFE_SB.AppId, (fmt), \
      ##__VA_ARGS__); \
  else \
    OS_printf("%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while (0);

static CFE_SB_MsgId_t MIN_MESSAGE_ID = 0x18C0;
static CFE_SB_MsgId_t MAX_MESSAGE_ID = 0x18DF;

DataReaderListenerImpl::DataReaderListenerImpl(CFE_SB_MsgId_t message_id)
{
  this->message_id = message_id;
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr reader,
  const DDS::RequestedDeadlineMissedStatus &status)
{
  (void)reader;
  (void)status;
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr reader,
  const DDS::RequestedIncompatibleQosStatus &status)
{
  (void)reader;
  (void)status;
}

void DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr reader,
  const DDS::SampleRejectedStatus &status)
{
  (void)reader;
  (void)status;
}

void DataReaderListenerImpl::on_liveliness_changed(DDS::DataReader_ptr reader,
  const DDS::LivelinessChangedStatus &status)
{
  (void)reader;
  (void)status;
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  SB::Msg msg;
  SB::MsgSeq msgList;
  DDS::SampleInfoSeq infoSeq;
  DDS::ReturnCode_t error = m_MsgReader->take(msgList, infoSeq, 1,
		  DDS::ANY_SAMPLE_STATE, 
		  DDS::ANY_VIEW_STATE, 
		  DDS::ANY_INSTANCE_STATE);
	if (DDS::RETCODE_OK == error) 
	{
    	uint32 spacecraft_id = CFE_PSP_GetSpacecraftId();
   		msg = msgList[0];
    	if (spacecraft_id == msg.spacecraft_id || msg.msg_id == 0x0817 || msg.msg_id == 0x0140) 
		{
      		int32 status =
              CFE_SB_SendMsgFullOnRecv((CFE_SB_MsgPtr_t) msg.raw.get_buffer(),
                                       msg.increment_tlm, CFE_SB_SEND_ONECOPY, msg.sender_name.in(),
                                       msg.sender_processor_id);
 	     	if (CFE_SUCCESS != status)
    	    	std::cerr << "CFE_SB_SendMsgFullOnRecv failed: " << status;
		}
    	else if (msg.msg_id >= MIN_MESSAGE_ID && msg.msg_id <= MAX_MESSAGE_ID) 
		{
      		int32 status =
              CFE_SB_SendMsgFullOnRecv((CFE_SB_MsgPtr_t) msg.raw.get_buffer(),
                                       msg.increment_tlm, CFE_SB_SEND_ONECOPY, msg.sender_name.in(),
                                       msg.sender_processor_id);
      		if (CFE_SUCCESS != status)
        		std::cerr << "CFE_SB_SendMsgFullOnRecv failed: " << status;
		}
 	} 
	else 
	{
		if (msg.msg_id != this->message_id) 
		{
	       std::cerr << "ERROR 1: DDS Listener Mismatch" << msg.msg_id << ", current MSG should be MSG_ID: " << this->message_id << "-- 'Send Window' may be full; may lead to deadlock" << std::endl;
     	} 
		else 
		{
       		std::cerr << "ERROR 2: Listener for Pipe Error: " <<
                 error << "Listening for MSG ID:" << std::dec << msg.msg_id << "0x" << std::hex << msg.msg_id << std::endl;
     	}
	}
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr reader,
  const DDS::SubscriptionMatchedStatus &status)
{
  (void)reader;
  (void)status;
}

void DataReaderListenerImpl::on_sample_lost(DDS::DataReader_ptr reader,
  const DDS::SampleLostStatus &status)
{
  (void)reader;
  (void)status;
}

