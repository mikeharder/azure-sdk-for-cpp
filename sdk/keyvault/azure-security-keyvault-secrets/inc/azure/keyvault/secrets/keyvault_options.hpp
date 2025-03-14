// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/**
 * @file
 * @brief Keyvault Secret actions options
 */
#pragma once
#include "azure/keyvault/secrets/generated/key_vault_client_options.hpp"
#include "dll_import_export.hpp"

#include <azure/core/internal/client_options.hpp>
namespace Azure { namespace Security { namespace KeyVault { namespace Secrets {

  /**
   * @brief Define the options to create an SDK Keys client.
   *
   */
  struct SecretClientOptions final : public Azure::Core::_internal::ClientOptions
  {
    /**
     * @brief Service Version used.
     *
     */
    std::string ApiVersion{_detail::KeyVaultClientOptions().ApiVersion};
  };

  /**
   * @brief Optional parameters for #Azure::Security::KeyVault::Secrets::SecretClient::GetSecret
   *
   */
  struct GetSecretOptions final
  {
    /**
     * @brief Specify the secret version to get.
     *
     */
    std::string Version;
  };

  /**
   * @brief Optional parameters for
   * #Azure::Security::KeyVault::Secrets::SecretClient::UpdateSecretProperties
   *
   */
  struct UpdateSecretPropertiesOptions final
  {
    /**
     * @brief Specify the secret version to update.
     *
     */
    std::string Version;
  };

  /**
   * @brief The options for calling an operation
   * #Azure::Security::KeyVault::Secrets::SecretClient::GetPropertiesOfSecrets.
   *
   */
  struct GetPropertiesOfSecretsOptions final
  {
    /**@brief Token for the next page.  */
    Azure::Nullable<std::string> NextPageToken;
  };

  /**
   * @brief The options for calling an operation
   * #Azure::Security::KeyVault::Secrets::SecretClient::GetPropertiesOfSecretsVersions.
   *
   */
  struct GetPropertiesOfSecretVersionsOptions final
  {
    /**@brief Token for the next page.  */
    Azure::Nullable<std::string> NextPageToken;
  };

  /**
   * @brief The options for calling an operation
   * #Azure::Security::KeyVault::Secrets::SecretClient::GetDeletedSecrets.
   *
   */
  struct GetDeletedSecretsOptions final
  {
    /**@brief Token for the next page.  */
    Azure::Nullable<std::string> NextPageToken;
  };
}}}} // namespace Azure::Security::KeyVault::Secrets
