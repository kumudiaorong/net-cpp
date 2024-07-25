#pragma once

#ifndef XSL_NET_TRANSPORT_TCP
#  define XSL_NET_TRANSPORT_TCP
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/component.h"
#  include "xsl/net/transport/tcp/conn.h"

TRANSPORT_NB
using TcpHandleHint = tcp::HandleHint;
using tcp::TcpHandlerGeneratorLike;
using tcp::TcpHandlerLike;
using TcpHandleState = tcp::HandleState;
// using TcpSendTaskNode = tcp::SendTaskNode;
// using TcpRecvTaskNode = tcp::RecvTaskNode;
// using TcpRecvTasks = tcp::RecvTasks;
// using TcpSendFile = tcp::SendFile;
// using tcp::TcpRecvString;
// using tcp::TcpSendString;
// using TcpSendTasksProxy = tcp::SendTasksProxy;
// using TcpSendContext = tcp::SendContext;
// using TcpRecvResult = tcp::RecvResult;
// using TcpRecvError = tcp::RecvError;
// using TcpSendError = tcp::SendError;
// using TcpRecvContext = tcp::RecvContext;
// using TcpSendTasks = tcp::SendTasks;
using tcp::TcpConnManager;
using tcp::TcpConnManagerConfig;
using tcp::TcpHandler;
// using tcp::TcpServer;
TRANSPORT_NE
#endif
