// OpenSplice backend interfacing for SB.
//
// NOTE: Zero-copy functionality is disabled, and will give errors.
// NOTE: "scope" subscribe functionality may not behave as expected.
//
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

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdint>
#include "ccpp_SB.h"
#include "ccpp_dds_dcps.h"
extern "C" {
#include <osapi.h>
#include <common_types.h>
#include <cfe_sb.h>
#include <cfe_es.h>
#include <cfe_psp.h>
#include <cfe_error.h>
#include <private/cfe_private.h>
#include <cfe_sb_events.h>
#include "cfe_sb_priv.h"
#include "cfe_sb_api_mod.h"
}

#include "DataReaderListenerImpl.h"

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
#define PARTICIPANT_QOS_DEFAULT (*::DDS::OpenSplice::Utils::FactoryDefaultQosHolder::get_domainParticipantQos_default())

static DDS::DomainId_t domain_id = 100;
static DDS::DomainParticipant_var participant = NULL;
static DDS::Publisher_var pub = DDS::Publisher::_nil();
static DDS::Subscriber_var sub = DDS::Subscriber::_nil();
static SB::MsgTypeSupport_var type_support_msg = SB::MsgTypeSupport::_nil();
static CFE_SB_MsgId_t COMPUTE_MID = 0x0140;
static CFE_SB_MsgId_t SPECIAL_MSG_ID = 0x0817;
static CFE_SB_MsgId_t MIN_MESSAGE_ID = 0x18C0;
static CFE_SB_MsgId_t MAX_MESSAGE_ID = 0x18DF;
 

static std::string create_topic_str(CFE_SB_MsgId_t msgid)
{
  std::stringstream ss;
    if (msgid == SPECIAL_MSG_ID || msgid == COMPUTE_MID) 
	{
      ss << "SB" << std::hex << msgid;
	}
	else if (msgid >= MIN_MESSAGE_ID && msgid <= MAX_MESSAGE_ID) 
	{
      ss << "SB" << std::hex << msgid;
	}
	else 
	{
      ss << "SB" << std::hex << std::dec << msgid << "SP" << std::dec << CFE_PSP_GetSpacecraftId();
	}
  return ss.str();
}

// Attempts to find a topic; creates it if it doesn't exist. Caller must delete_topic() on returned topic if it is a proxy.
static DDS::Topic_var get_sb_topic(CFE_SB_MsgId_t msg_id)
{
  const char* type_name = type_support_msg->get_type_name();
  const std::string topic_string = create_topic_str(msg_id);
  const char* topic_str = topic_string.c_str();
  DDS::Duration_t timeout = {0, 0};
  DDS::Topic_var topic = participant->find_topic(topic_str, timeout);
  if (topic.in() == NULL) {
    topic = participant->create_topic(topic_str, type_name, TOPIC_QOS_DEFAULT,
        0, DDS::STATUS_MASK_NONE);
    if (topic.in() == NULL) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR, "Topic %s creation failed.", topic_str);
      return DDS::Topic::_nil();
    }
  }
  return topic;
}

int CFE_SB_DDSInit(void)
{
  try {
    // Construct to avoid const char * implicit casting for safety.
    char *argv[3];
    char a[3][32] = {"core-linux.bin", "-DCPSConfigFile", "DCPS_TRANSPORT_CFG"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    for (int i = 0; i < argc; ++i)
      argv[i] = a[i];

    // Initialize and create a DomainParticipant
    DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::get_instance();
    if (dpf.in() == NULL){
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "create_factory failed.");
      return 1;
    }
    participant = dpf->create_participant(domain_id, PARTICIPANT_QOS_DEFAULT,
        NULL,
        DDS::STATUS_MASK_NONE);
    if (participant.in() == NULL) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "create_participant failed.");
      return 2;
    }

    // Register Type Support defined with IDL
    type_support_msg = new SB::MsgTypeSupport();
    if (type_support_msg->register_type(participant.in(), type_support_msg->get_type_name()) != DDS::RETCODE_OK) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "Registering type support failed.");
      return 3;
    }

    // Create a Publisher for the SB
    pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
        DDS::STATUS_MASK_NONE);
    if (pub.in() == NULL) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "create_publisher failed.");
      return 4;
    }

    // Create a Subscriber for the SB
    sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
        DDS::SubscriberListener::_nil(), DDS::STATUS_MASK_NONE);
    if (sub.in() == NULL) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "create_subscriber failed.");
      return 5;
    }
  } catch (std::exception &e) {
    DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR, "Exception caught: %s",
      e.what());
    return 6;
  }
  return 0;
}

int32 CFE_SB_SubscribeFull(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId,
  CFE_SB_Qos_t Quality, uint16 MsgLim, uint8 Scope)
{
  int32 status;
  uint32 TskId = OS_TaskGetId();
  char name_buf[OS_MAX_API_NAME * 2 + 1];

  status = CFE_SB_SubscribeFullInternal(MsgId, PipeId, Quality, MsgLim, Scope);
  if (CFE_SUCCESS != status)
    return status;

  // TODO: If anything else fails, what CFE_SB_SubscribeFullInternal did needs to be undone.
  try {
    CFE_SB_LockSharedData(__func__, __LINE__);
    DDS::Topic_var topic = get_sb_topic(MsgId);
    if (topic.in() == NULL) {
      status = -1; // XXX
      goto cleanup;
    }
    

    // Find/Create Data Reader for this topic
    DDS::DataReader_var reader = sub->lookup_datareader(topic->get_name());
    if (reader.in() == NULL) {
      reader = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, NULL,
          DDS::STATUS_MASK_NONE);
      if (reader.in() == NULL) {
        DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
          "Data reader creation failed.");
        status = -1; // XXX
        goto cleanup;
      }

      if (MsgId == SPECIAL_MSG_ID) {
        DDS::DataReaderQos qos;
        reader->get_qos(qos);
        qos.history.kind = DDS::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS;
        qos.history.depth = 1;
        reader->set_qos(qos);
      }

      // create DataListener and associate with Data Reader
      DataReaderListenerImpl *listener = new DataReaderListenerImpl(MsgId);
      listener->m_MsgReader = SB::MsgDataReader::_narrow(reader.in());
      DDS::StatusMask mask = DDS::DATA_AVAILABLE_STATUS | DDS::REQUESTED_DEADLINE_MISSED_STATUS;
      listener->m_MsgReader->set_listener(listener, mask);
    }
  } catch (std::exception &e) {
    DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR, "Exception caught: %s",
      e.what());
    status = -1; // XXX
    goto cleanup;
  }
  status = CFE_SUCCESS;
cleanup:
  CFE_SB_UnlockSharedData(__func__, __LINE__);
  return status;
}

int32 CFE_SB_SendMsgFull(CFE_SB_Msg_t *MsgPtr, uint32 TlmCntIncrements,
  uint32 CopyMode)
{
  int32 status;

  if (CFE_SB_SEND_ZEROCOPY == CopyMode) {
    DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
      "Zero copy mode not supported.");
    return CFE_SB_NOT_IMPLEMENTED;
  }

  status = CFE_SB_SendMsgFullOnSend(MsgPtr, TlmCntIncrements, CopyMode);
  if (status != CFE_SUCCESS)
    return status;

  CFE_SB_MsgId_t MsgId = CFE_SB_GetMsgId(MsgPtr);

  try {
    CFE_SB_LockSharedData(__func__, __LINE__);
    DDS::Topic_var topic = get_sb_topic(MsgId);
    if (topic.in() == NULL) {
      status = -1; // XXX
      goto cleanup;
    }

    // Create Data Writer for topic if necessary
    DDS::DataWriter_var writer = pub->lookup_datawriter(topic->get_name());
    if (writer.in() == NULL) {
      writer = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, NULL,
          DDS::STATUS_MASK_NONE);
      if (writer.in() == NULL) {
        DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
          "Data writer creation failed.");
        status = -1; // XXX
        goto cleanup;
      }
    }


    SB::MsgDataWriter_var writer_msg = SB::MsgDataWriter::_narrow(writer);
    if (writer_msg.in() == NULL) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR,
        "Message data writer creation failed.");
      status = -1; // XXX
      goto cleanup;
    }

    // Write out message
    // * 2 due to task and app name concatenation by CFE_SB_GetAppTskName
    char name_buf[OS_MAX_API_NAME * 2 + 1];

    CFE_SB_GetAppTskName(OS_TaskGetId(), name_buf);

    SB::Msg msg;
    msg.msg_id = MsgId;
    msg.increment_tlm = TlmCntIncrements;

    msg.spacecraft_id = CFE_PSP_GetSpacecraftId();
    msg.sender_processor_id = CFE_PSP_GetProcessorId();

    msg.sender_name = DDS::string_dup(name_buf);
    // Maximum message length is header + 65536
    // TODO: Try as not raw only 
   
    msg.raw = SB::seq_oct(65536 + 
		    (sizeof(CFE_SB_CmdHdr_t) > sizeof(CFE_SB_TlmHdr_t) ?
		     sizeof(CFE_SB_CmdHdr_t) :
		     sizeof(CFE_SB_TlmHdr_t)),
	          CFE_SB_GetTotalMsgLength(MsgPtr),
		  (DDS::Octet *) MsgPtr,
		  FALSE);

    DDS::ReturnCode_t error = writer_msg->write(msg, DDS::HANDLE_NIL);
    if (DDS::RETCODE_OK != error) {
      DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR, "Write failed.");
      status = -1; // XXX
      goto cleanup;
    }

  } catch (std::exception &e) {
    DEBUG_MSG(CFE_SB_DDS_ERR_EID, CFE_EVS_ERROR, "Exception caught: %s",
      e.what());
    status = -1; // XXX
    goto cleanup;
  }
cleanup:
  CFE_SB_UnlockSharedData(__func__, __LINE__);
  return status;
}

