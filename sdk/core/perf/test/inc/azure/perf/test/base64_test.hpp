// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <azure/core/base64.hpp>
#include <azure/perf.hpp>
#include <vector>

namespace Azure { namespace Perf { namespace Test {

  class Base64Test : public Azure::Perf::PerfTest {
  private:
    std::vector<uint8_t> m_data;

  public:
    Base64Test(Azure::Perf::TestOptions options) : PerfTest(options)
    {
      for (uint8_t i = 0; i < maxLength; i++)
      {
        data.push_back(i + 1);
      }
    }

    void Run(Azure::Core::Context const&) override
    {
      Convert::Base64Encode(data);
    }

    static Azure::Perf::TestMetadata GetTestMetadata()
    {
      return {
          "base64",
          "Base64 Encoding",
          [](Azure::Perf::TestOptions options) {
            return std::make_unique<Azure::Perf::Test::Base64Test>(options);
          }};
    }
  };

}}}