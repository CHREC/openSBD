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


#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include "ccpp_dds_dcps.h"
#include "ccpp_SB.h"

extern "C" {
#include <cfe_sb.h>
}

class DataReaderListenerImpl
  : public virtual DDS::DataReaderListener
{
private:
  CFE_SB_PipeId_t pipe_id;
  CFE_SB_MsgId_t message_id;
public:
  SB::MsgDataReader_var m_MsgReader;
  DataReaderListenerImpl(CFE_SB_MsgId_t);
//  DataReaderListenerImpl();
  virtual void on_requested_deadline_missed(DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus &status);

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus &status);

  virtual void on_sample_rejected(DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus &status);

  virtual void on_liveliness_changed(DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus &status);

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_subscription_matched(DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus &status);

  virtual void on_sample_lost(DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus &status);
};

#endif // DATAREADER_LISTENER_IMPL_H

