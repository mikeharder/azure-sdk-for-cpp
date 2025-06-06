// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "gtest/gtest.h"
#include "private/key_constants.hpp"
#include "private/key_serializers.hpp"

#include <azure/core/context.hpp>
#include <azure/identity/client_secret_credential.hpp>
#include <azure/keyvault/keys.hpp>

#include <exception>
#include <memory>

using namespace Azure::Security::KeyVault::Keys;

TEST(KeyVaultKeyClientUnitTest, initClient)
{
  auto credential
      = std::make_shared<Azure::Identity::ClientSecretCredential>("tenantID", "AppId", "SecretId");
  {
    EXPECT_NO_THROW(KeyClient keyClient("http://account.vault.azure.net", credential));
  }
  {
    KeyClientOptions options;
    options.Retry.MaxRetries = 10;
    EXPECT_NO_THROW(KeyClient keyClient("http://account.vault.azure.net", credential, options));
  }
}

TEST(KeyVaultKeyClientUnitTest, ServiceVersion)
{
  auto credential
      = std::make_shared<Azure::Identity::ClientSecretCredential>("tenantID", "AppId", "SecretId");
  // Default - 7.6-preview.2
  EXPECT_NO_THROW(auto options = KeyClientOptions();
                  KeyClient keyClient("http://account.vault.azure.net", credential, options);
                  EXPECT_EQ(options.ApiVersion, "7.6-preview.2"););

  // 7.4
  EXPECT_NO_THROW(auto options = KeyClientOptions(); options.ApiVersion = "7.4";
                  KeyClient keyClient("http://account.vault.azure.net", credential, options);
                  EXPECT_EQ(options.ApiVersion, "7.4"););
}

TEST(KeyVaultKeyClientUnitTest, GetUrl)
{
  auto credential
      = std::make_shared<Azure::Identity::ClientSecretCredential>("tenantID", "AppId", "SecretId");

  auto url = "vaultUrl";
  KeyClient keyClient(url, credential);
  EXPECT_EQ(url, keyClient.GetUrl());
}

TEST(KeyEncryptionAlgorithmUnitTest, CheckValues)
{
// Disable deprecation warning
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  EXPECT_EQ(
      KeyEncryptionAlgorithm::CKM_RSA_AES_KEY_WRAP.ToString(), _detail::CKM_RSA_AES_KEY_WRAP_Value);
  EXPECT_EQ(
      KeyEncryptionAlgorithm::RSA_AES_KEY_WRAP_256.ToString(), _detail::RSA_AES_KEY_WRAP_256_Value);
  EXPECT_EQ(
      KeyEncryptionAlgorithm::RSA_AES_KEY_WRAP_384.ToString(), _detail::RSA_AES_KEY_WRAP_384_Value);
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // _MSC_VER

  EXPECT_EQ(
      KeyEncryptionAlgorithm::CkmRsaAesKeyWrap.ToString(), _detail::CKM_RSA_AES_KEY_WRAP_Value);
  EXPECT_EQ(
      KeyEncryptionAlgorithm::RsaAesKeyWrap256.ToString(), _detail::RSA_AES_KEY_WRAP_256_Value);
  EXPECT_EQ(
      KeyEncryptionAlgorithm::RsaAesKeyWrap384.ToString(), _detail::RSA_AES_KEY_WRAP_384_Value);
}
