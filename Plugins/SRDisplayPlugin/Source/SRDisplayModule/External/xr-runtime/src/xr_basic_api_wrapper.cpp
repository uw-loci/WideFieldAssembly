/**
 * Sony CONFIDENTIAL
 *
 * Copyright 2022 Sony Corporation
 *
 * DO NOT COPY AND/OR REDISTRIBUTE WITHOUT PERMISSION.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "xr_basic_api_wrapper.h"

#include "xr-runtime-common/xr_windows.h"

#pragma comment(lib, "version.lib")


namespace srdisplay::basic::api {
  using SetCameraWindowEnabled_t = SonyOzResult(*)(SonyOzSessionHandle, const bool);
  SetCameraWindowEnabled_t SetCameraWindowEnabled = nullptr;

  using GetCrosstalkCorrectionSettings_t = SonyOzResult(*)(SonyOzSessionHandle, sony::oz::srd_base_settings::SrdXrCrosstalkCorrectionMode*);
  GetCrosstalkCorrectionSettings_t GetCrosstalkCorrectionSettings = nullptr;

  using SetCrosstalkCorrectionSettings_t = SonyOzResult(*)(SonyOzSessionHandle, sony::oz::srd_base_settings::SrdXrCrosstalkCorrectionMode);
  SetCrosstalkCorrectionSettings_t SetCrosstalkCorrectionSettings = nullptr;

  using GetStubHeadPose_t = SonyOzResult(*)(SonyOzSessionHandle, SonyOzPosef*);
  GetStubHeadPose_t GetStubHeadPose = nullptr;

  using SetStubHeadPose_t = SonyOzResult(*)(SonyOzSessionHandle, const SonyOzPosef);
  SetStubHeadPose_t SetStubHeadPose = nullptr;

  using GetPauseHeadPose_t = SonyOzResult(*)(SonyOzSessionHandle, bool*);
  GetPauseHeadPose_t GetPauseHeadPose = nullptr;

  using SetPauseHeadPose_t =  SonyOzResult(*)(SonyOzSessionHandle, const bool);
  SetPauseHeadPose_t SetPauseHeadPose = nullptr;

  using GetSoftwareVersion_t = SonyOzResult(*)(SonyOzSessionHandle, char*, uint32_t);
  GetSoftwareVersion_t GetSoftwareVersion = nullptr;

  using SetForce90Degree_t = SonyOzResult(*)(SonyOzSessionHandle, const bool);
  SetForce90Degree_t SetForce90Degree = nullptr;

  using GetSystemTiltDegree_t = SonyOzResult(*)(SonyOzSessionHandle, const int*);
  GetSystemTiltDegree_t GetSystemTiltDegree = nullptr;

  std::wstring GetInstallPath() {

    HKEY hkey = HKEY_LOCAL_MACHINE;
    const std::wstring sub_key = std::wstring(L"SOFTWARE\\Sony Corporation\\Spatial Reality Display");
    const std::wstring value = L"Path";

    DWORD data_size{};
    LONG return_code =
      ::RegGetValueW(hkey, sub_key.c_str(), value.c_str(), RRF_RT_REG_SZ,
                     nullptr, nullptr, &data_size);
    if (return_code != ERROR_SUCCESS) {
      return L"";
    }

    std::wstring data;
    data.resize(data_size / sizeof(wchar_t));

    return_code = ::RegGetValueW(hkey, sub_key.c_str(), value.c_str(),
                                 RRF_RT_REG_SZ, nullptr, &data[0], &data_size);
    if (return_code != ERROR_SUCCESS) {
      return L"";
    }

    DWORD string_length_in_wchars = data_size / sizeof(wchar_t);

    // Exclude the NULL written by the Win32 API
    string_length_in_wchars--;

    data.resize(string_length_in_wchars);
    return data;
  }

  bool LoadXrRuntimeLibrary() {
    static HMODULE handle = nullptr;

    if (handle) {
      return true;
    }

    std::wstring directry = GetInstallPath();
    std::wstring xrmw_path = directry + L"lib\\xr_runtime.dll";
    handle = ::LoadLibraryW(xrmw_path.c_str());

    if (handle == nullptr) {
      return false;
    }

#pragma warning(disable:4191)
  #define GET_FUNCTION(name)                      \
  name = (name##_t)GetProcAddress(handle, #name); \
  if (!name) {                                    \
    return false;                                 \
  }

    GET_FUNCTION(SetCameraWindowEnabled);
    GET_FUNCTION(GetCrosstalkCorrectionSettings);
    GET_FUNCTION(SetCrosstalkCorrectionSettings);
    GET_FUNCTION(GetStubHeadPose);
    GET_FUNCTION(SetStubHeadPose);
    GET_FUNCTION(GetPauseHeadPose);
    GET_FUNCTION(SetPauseHeadPose);
    GET_FUNCTION(GetSoftwareVersion);
    GET_FUNCTION(SetForce90Degree);
    GET_FUNCTION(GetSystemTiltDegree);
#pragma warning(default:4191)

    return true;
  }
}

namespace sony::oz::xr_runtime {
  SonyOzResult SetCameraWindowEnabled(SonyOzPlatformId platform_id, SonyOzSessionHandle session, const bool enable) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }

    if (srdisplay::basic::api::SetCameraWindowEnabled == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::SetCameraWindowEnabled(session, enable);
  }

  SonyOzResult GetCrosstalkCorrectionSettings(SonyOzPlatformId platform_id, SonyOzSessionHandle session, sony::oz::srd_base_settings::SrdXrCrosstalkCorrectionMode* mode) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }

    if (srdisplay::basic::api::GetCrosstalkCorrectionSettings == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::GetCrosstalkCorrectionSettings(session, mode);
  }

  SonyOzResult SetCrosstalkCorrectionSettings(SonyOzPlatformId platform_id, SonyOzSessionHandle session, const sony::oz::srd_base_settings::SrdXrCrosstalkCorrectionMode mode) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }

    if (srdisplay::basic::api::SetCrosstalkCorrectionSettings == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::SetCrosstalkCorrectionSettings(session, mode);
  }

  SonyOzResult GetStubHeadPose(SonyOzSessionHandle session, SonyOzPosef* pose) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::GetStubHeadPose == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::GetStubHeadPose(session, pose);
  }

  SonyOzResult SetStubHeadPose(SonyOzSessionHandle session, const SonyOzPosef pose) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::SetStubHeadPose == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::SetStubHeadPose(session, pose);
  }

  SonyOzResult GetPauseHeadPose(SonyOzSessionHandle session, bool* enabled) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::GetPauseHeadPose == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }
    return srdisplay::basic::api::GetPauseHeadPose(session, enabled);
  }

  SonyOzResult SetPauseHeadPose(SonyOzSessionHandle session, const bool enabled) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::SetPauseHeadPose == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }
    return srdisplay::basic::api::SetPauseHeadPose(session, enabled);
  }

  SonyOzResult GetSoftwareVersion(SonyOzSessionHandle session, char* version, uint32_t length) {
      if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
          return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
      }
      if (srdisplay::basic::api::GetSoftwareVersion == nullptr) {
          return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
      }
      return srdisplay::basic::api::GetSoftwareVersion(session, version, length);
  }

  std::wstring StringToWString(std::string oString) {
      int buffer_size = MultiByteToWideChar(CP_OEMCP, 0, oString.c_str(), -1, (wchar_t*)NULL, 0);
      LPWSTR cpWideByte = new WCHAR[buffer_size];

      MultiByteToWideChar(CP_OEMCP, 0, oString.c_str(), -1, cpWideByte, buffer_size);
      std::wstring out{ cpWideByte, cpWideByte + buffer_size - 1 };
      delete[] cpWideByte;
      return out;
  }

  std::wstring GetRegistryString(HKEY hkey, const std::wstring& sub_key, const std::wstring& value) {
      DWORD data_size{};
      LONG return_code = ::RegGetValueW(hkey, sub_key.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr, nullptr, &data_size);
      if (return_code != ERROR_SUCCESS) {
          return L"";
      }

      std::wstring data;
      data.resize(data_size / sizeof(wchar_t));
      return_code = ::RegGetValueW(hkey, sub_key.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr, &data[0], &data_size);
      if (return_code != ERROR_SUCCESS) {
          return L"";
      }

      DWORD string_length_in_wchars = data_size / sizeof(wchar_t);

      // Exclude the NULL written by the Win32 API
      string_length_in_wchars--;
      data.resize(string_length_in_wchars);
      return data;
  }

  std::wstring GetRegistryPathW(SonyOzPlatformId platform_id) {
      std::wstring registry_path = std::wstring(L"SOFTWARE\\Sony Corporation\\") + StringToWString(platform_id);
      return GetRegistryString(HKEY_LOCAL_MACHINE, registry_path, L"Path");
  }

  std::pair<bool, std::wstring> GetRuntimeVersion(SonyOzPlatformId platform_id) {
    bool result = false;
    std::wstring version = L"";

    DWORD version_dw[VERSION_SIZE];

    int major, minor, build, revision;
    if (GetRuntimeVersionInfo(platform_id, major, minor, build, revision)) {
        version_dw[0] = major;
        version_dw[1] = minor;
        version_dw[2] = build;
        version_dw[3] = revision;
        auto version_string = [](const DWORD in[VERSION_SIZE]) -> std::wstring {
            return std::to_wstring(in[0]) + L"," + std::to_wstring(in[1]) + L"," + std::to_wstring(in[2]) + L"," + std::to_wstring(in[3]);
        };

        result = true;
        version = version_string(version_dw);
    }
    
    return { result, version };
  }

  bool GetRuntimeVersionInfo(SonyOzPlatformId platform_id, int& major, int& minor, int& build, int& revision) {
      bool result = false;

      auto directory = GetRegistryPathW(platform_id);
      auto path = directory + L"lib\\xr_runtime.dll";
      DWORD info_size = ::GetFileVersionInfoSizeW(path.c_str(), NULL);
      void* buf = malloc(sizeof(char) * info_size);

      if (buf != nullptr && info_size && ::GetFileVersionInfoW(path.c_str(), NULL, info_size, buf)) {
          VS_FIXEDFILEINFO* file_info;
          UINT len;
          if (::VerQueryValueW(buf, (L"\\"), (LPVOID*)&file_info, &len)) {
              major = int(HIWORD(file_info->dwFileVersionMS));
              minor = int(LOWORD(file_info->dwFileVersionMS));
              build = int(HIWORD(file_info->dwFileVersionLS));
              revision = int(LOWORD(file_info->dwFileVersionLS));
              result = true;
          }
      }
      free(buf);
      buf = nullptr;

      return result;
  }

  std::pair<bool, std::wstring> GetDisplayVersion(SonyOzSessionHandle session) {
    bool result = false;
    std::wstring version = L"";

    const uint32_t max_length = 16;
    char buffer[max_length] = {};
    char* version_c = buffer;

    SonyOzResult has_result = GetSoftwareVersion(session, version_c, max_length);

    if (has_result == SonyOzResult::SUCCESS) {
      std::string version_str = version_c;
      result = true;
      version = StringToWString(version_str);
    }

    return { result, version };
  }
  
  SonyOzResult SetForce90Degree(SonyOzSessionHandle session, const bool enable) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::SetForce90Degree == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::SetForce90Degree(session, enable);
  }
  
  SonyOzResult GetSystemTiltDegree(SonyOzSessionHandle session, int* param) {
    if (srdisplay::basic::api::LoadXrRuntimeLibrary() == false) {
      return SonyOzResult::ERROR_RUNTIME_NOT_FOUND;
    }
    if (srdisplay::basic::api::GetSystemTiltDegree == nullptr) {
      return SonyOzResult::ERROR_FUNCTION_UNSUPPORTED;
    }

    return srdisplay::basic::api::GetSystemTiltDegree(session, param);
  }
}
