// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "private/claims_based_security_impl.hpp"
#include "private/connection_impl.hpp"
#include "private/management_impl.hpp"
#include "private/session_impl.hpp"

#include <azure/core/diagnostics/logger.hpp>
#include <azure/core/internal/diagnostics/log.hpp>

using namespace Azure::Core::Diagnostics::_internal;
using namespace Azure::Core::Diagnostics;
using namespace Azure::Core::Amqp::_internal;

namespace Azure { namespace Core { namespace Amqp { namespace _detail {
  ClaimsBasedSecurityImpl::ClaimsBasedSecurityImpl(std::shared_ptr<_detail::SessionImpl> session)
      : m_session{session}
  {
  }

  ClaimsBasedSecurityImpl::~ClaimsBasedSecurityImpl() noexcept {}

  CbsOpenResult ClaimsBasedSecurityImpl::Open(Context const& context)
  {
    if (!m_management)
    {
      ManagementClientOptions managementOptions;
      managementOptions.EnableTrace = m_session->GetConnection()->IsTraceEnabled();
      managementOptions.ExpectedStatusCodeKeyName = "status-code";
      managementOptions.ExpectedStatusDescriptionKeyName = "status-description";
      managementOptions.ManagementNodeName = "$cbs";
      m_management
          = std::make_shared<ManagementClientImpl>(m_session, "$cbs", managementOptions, this);

      auto rv{m_management->Open(context)};
      switch (rv)
      {
        case ManagementOpenStatus::Invalid:
          return CbsOpenResult::Invalid;
        case ManagementOpenStatus::Ok:
          return CbsOpenResult::Ok;
        case ManagementOpenStatus::Error:
          return CbsOpenResult::Error;
        case ManagementOpenStatus::Cancelled:
          return CbsOpenResult::Cancelled;
        default:
          throw std::runtime_error("Unknown return value from Management::Open()");
      }
    }
    else
    {
      return CbsOpenResult::Error;
    }
  }

  void ClaimsBasedSecurityImpl::Close(Context const& context) { m_management->Close(context); }

  std::tuple<CbsOperationResult, uint32_t, std::string> ClaimsBasedSecurityImpl::PutToken(
      CbsTokenType tokenType,
      std::string const& audience,
      std::string const& token,
      Azure::DateTime const& tokenExpirationTime,
      Context const& context)
  {
    Models::AmqpMessage message;
    message.SetBody(static_cast<Models::AmqpValue>(token));

    message.ApplicationProperties["name"] = static_cast<Models::AmqpValue>(audience);
    message.ApplicationProperties["expiration"]
        = std::chrono::duration_cast<std::chrono::seconds>(tokenExpirationTime.time_since_epoch())
              .count();

    auto result = m_management->ExecuteOperation(
        "put-token",
        (tokenType == CbsTokenType::Jwt ? "jwt" : "servicebus.windows.net:sastoken"),
        {},
        message,
        context);
    if (result.Status != ManagementOperationStatus::Ok)
    {
      CbsOperationResult cbsResult;
      switch (result.Status)
      {
        case ManagementOperationStatus::Invalid:
          cbsResult = CbsOperationResult::Invalid;
          break;
        case ManagementOperationStatus::Ok:
          cbsResult = CbsOperationResult::Ok;
          break;
        case ManagementOperationStatus::Error:
          cbsResult = CbsOperationResult::Error;
          break;
        case ManagementOperationStatus::FailedBadStatus:
          cbsResult = CbsOperationResult::Failed;
          break;
        case ManagementOperationStatus::InstanceClosed:
          cbsResult = CbsOperationResult::InstanceClosed;
          break;
        case ManagementOperationStatus::Cancelled:
          cbsResult = CbsOperationResult::Cancelled;
          break;
        default:
          throw std::runtime_error("Unknown management operation status.");
      }
      Log::Stream(Logger::Level::Informational)
          << "CBS PutToken result: " << cbsResult << " status code: " << result.StatusCode
          << " Error: " << result.Error << ".";
      return std::make_tuple(cbsResult, result.StatusCode, result.Error.Description);
    }
    else
    {
      return std::make_tuple(CbsOperationResult::Ok, result.StatusCode, result.Error.Description);
    }
  }
  std::ostream& operator<<(std::ostream& os, CbsOperationResult operationResult)
  {
    switch (operationResult)
    {
      case CbsOperationResult::Invalid:
        os << "Invalid";
        break;
      case CbsOperationResult::Ok:
        os << "Ok";
        break;
      case CbsOperationResult::Error:
        os << "Error";
        break;
      case CbsOperationResult::Failed:
        os << "Failed";
        break;
      case CbsOperationResult::InstanceClosed:
        os << "InstanceClosed";
        break;
      case CbsOperationResult::Cancelled:
        os << "Cancelled";
        break;
    }
    return os;
  }

  std::ostream& operator<<(std::ostream& os, CbsOpenResult openResult)
  {
    switch (openResult)
    {
      case CbsOpenResult::Invalid:
        os << "Invalid";
        break;
      case CbsOpenResult::Ok:
        os << "Ok";
        break;
      case CbsOpenResult::Error:
        os << "Error";
        break;
      case CbsOpenResult::Cancelled:
        os << "Cancelled";
        break;
    }
    return os;
  }

  void ClaimsBasedSecurityImpl::OnError(Models::_internal::AmqpError const& error)
  {
    Log::Stream(Logger::Level::Warning) << "AMQP Error processing ClaimsBasedSecurity: " << error;
  }

}}}} // namespace Azure::Core::Amqp::_detail
