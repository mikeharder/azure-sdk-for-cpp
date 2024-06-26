// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/**
 * @brief This sample provides the code implementation to use the Key Vault SDK client for C++
 * to back up and restore a key.
 *
 * @remark The following environment variables must be set before running the sample.
 * - AZURE_KEYVAULT_URL:  To the Key Vault account URL.
 *
 */

#include <azure/core.hpp>
#include <azure/identity.hpp>
#include <azure/keyvault/keys.hpp>

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

using namespace Azure::Security::KeyVault::Keys;

static void AssertKeysEqual(KeyProperties const& expected, KeyProperties const& actual);

int main()
{
  auto const keyVaultUrl = std::getenv("AZURE_KEYVAULT_URL");
  auto credential = std::make_shared<Azure::Identity::DefaultAzureCredential>();

  KeyClient keyClient(keyVaultUrl, credential);

  std::string rsaKeyName("CloudRsaKey" + Azure::Core::Uuid::CreateUuid().ToString());
  try
  {
    auto rsaKey = CreateRsaKeyOptions(rsaKeyName);
    rsaKey.KeySize = 2048;
    rsaKey.ExpiresOn = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);

    std::cout << "\t-Create Key" << std::endl;
    auto storedKey = keyClient.CreateRsaKey(rsaKey).Value;
    size_t backUpSize = 0;
    {
      std::cout << "\t-Backup Key" << std::endl;
      std::vector<uint8_t> backupKey(keyClient.BackupKey(rsaKeyName).Value.BackupKey);
      backUpSize = backupKey.size();

      // save data to file
      std::cout << "\t-Save to file" << std::endl;
      std::ofstream savedFile;
      savedFile.open("backup.dat");
      for (auto const& data : backupKey)
      {
        savedFile << data;
      }
      savedFile.close();
    }
    // backup key is destroy at this point as it is out of the scope.
    // The storage account key is no longer in use, so you delete it.
    std::cout << "\t-Delete and purge key" << std::endl;
    DeleteKeyOperation operation = keyClient.StartDeleteKey(rsaKeyName);
    // You only need to wait for completion if you want to purge or recover the key.
    operation.PollUntilDone(std::chrono::milliseconds(2000));
    keyClient.PurgeDeletedKey(rsaKeyName);

    // Let's wait for a bit (maximum ~5 minutes) so we know the key was purged.
    uint16_t loop = 0;
    try
    {

      // To check if the key purged has started we attempt to get the key from the Key Vault.
      // If we get an exception, the key purge has started.
      // If we get more than 300 loops (~5minutes), we assume something went wrong.
      while (!keyClient.GetDeletedKey(rsaKeyName).Value.Name().empty() && loop < 300)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        loop++;
      }
      throw std::runtime_error("Key was not purged.");
    }
    catch (Azure::Core::RequestFailedException const& e)
    {
      std::cout << "\t" << e.what() << std::endl;
      std::cout << e.Message << std::endl;
      std::cout << loop << std::endl;
      std::cout << "\t-Key purge started" << std::endl;
    }
    // Restore the key from the file backup
    std::cout << "\t-Read from file." << std::endl;
    std::ifstream inFile;
    inFile.open("backup.dat");
    std::vector<uint8_t> inMemoryBackup(backUpSize);
    inFile >> inMemoryBackup.data();
    inFile.close();

    std::cout << "\t-Restore Key" << std::endl;

    KeyVaultKey restoredKey;
    bool restored = false;
    loop = 0;
    // the purge process is not instantaneous, so when we try to restore the key, it might fail.
    // we will attempt to restore the key for 5 times, waiting 20 seconds between each try.
    // If the key is not restored after 5 tries, we assume something went wrong.
    while (!restored && loop < 5)
    {
      try
      {
        restoredKey = keyClient.RestoreKeyBackup(inMemoryBackup).Value;
        restored = true;
        break;
      }
      catch (Azure::Core::RequestFailedException const& e)
      {
        std::cout << "\t" << e.what() << std::endl;
        std::cout << e.Message << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(20));
        loop++;
      }
    }

    if (!restored)
    {
      throw std::runtime_error("Key was not restored.");
    }
    AssertKeysEqual(storedKey.Properties, restoredKey.Properties);

    operation = keyClient.StartDeleteKey(rsaKeyName);
    // You only need to wait for completion if you want to purge or recover the key.
    operation.PollUntilDone(std::chrono::milliseconds(2000));
    keyClient.PurgeDeletedKey(rsaKeyName);
  }
  catch (Azure::Core::Credentials::AuthenticationException const& e)
  {
    std::cout << "Authentication Exception happened:" << std::endl << e.what() << std::endl;
    return 1;
  }
  catch (Azure::Core::RequestFailedException const& e)
  {
    std::cout << "KeyVault Client Exception happened:" << std::endl << e.Message << std::endl;
    return 1;
  }

  return 0;
}

template <class T>
static inline bool CompareNullableT(Azure::Nullable<T> const& left, Azure::Nullable<T> const& right)
{
  if (!left.HasValue() && !right.HasValue())
  {
    return true;
  }
  if (left.HasValue() && !right.HasValue())
  {
    return false;
  }
  if (!left.HasValue() && right.HasValue())
  {
    return false;
  }
  return left.Value() == right.Value();
}

void AssertKeysEqual(KeyProperties const& expected, KeyProperties const& actual)
{
#if defined(NDEBUG)
  // Use (void) to silent unused warnings.
  (void)expected;
  (void)actual;
#endif
  assert(expected.Name == actual.Name);
  assert(expected.Version == actual.Version);
  assert(expected.Managed == actual.Managed);
  assert(expected.RecoveryLevel == actual.RecoveryLevel);
  assert(CompareNullableT(expected.ExpiresOn, actual.ExpiresOn));
  assert(CompareNullableT(expected.NotBefore, actual.NotBefore));
}
