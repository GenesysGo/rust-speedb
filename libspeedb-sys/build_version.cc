// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.

#include <memory>

#include "rocksdb/version.h"
#include "speedb/version.h"
#include "rocksdb/utilities/object_registry.h"
#include "util/string_util.h"

// The build script may replace these values with real values based
// on whether or not GIT is available and the platform settings
static const std::string speedb_build_git_sha  = "speedb_build_git_sha:ea70d40f86b70b1ac71d0d15152185a54e623f76";
static const std::string speedb_build_git_tag = "speedb_build_git_tag:release/2.6";
#define HAS_GIT_CHANGES 0
#if HAS_GIT_CHANGES == 0
// If HAS_GIT_CHANGES is 0, the GIT date is used.
// Use the time the branch/tag was last modified
static const std::string speedb_build_date = "speedb_build_date: ";
#else
// If HAS_GIT_CHANGES is > 0, the branch/tag has modifications.
// Use the time the build was created.
static const std::string speedb_build_date = "speedb_build_date:2023-09-12 06:50:56";
#endif

#define SPDB_BUILD_TAG "?"
static const std::string speedb_build_tag = "speedb_build_tag:" SPDB_BUILD_TAG;

#define USE_RTTI ""
static const std::string use_rtti = "use_rtti:" USE_RTTI;

#define DEBUG_LEVEL "0"
static const std::string debug_level = "debug_level:" DEBUG_LEVEL;

#define PORTABLE ""
static const std::string portable = "portable:" PORTABLE;

extern "C" {
 int register_SpeedbPlugins(ROCKSDB_NAMESPACE::ObjectLibrary&, const std::string&);
} // extern "C"

std::unordered_map<std::string, ROCKSDB_NAMESPACE::RegistrarFunc> ROCKSDB_NAMESPACE::ObjectRegistry::builtins_ = {
   {"speedb", register_SpeedbPlugins},
};

namespace ROCKSDB_NAMESPACE {
static void AddProperty(std::unordered_map<std::string, std::string> *props, const std::string& name) {
  size_t colon = name.find(":");
  if (colon != std::string::npos && colon > 0 && colon < name.length() - 1) {
    // If we found a "@:", then this property was a build-time substitution that failed.  Skip it
    size_t at = name.find("@", colon);
    if (at != colon + 1) {
      // Everything before the colon is the name, after is the value
      (*props)[name.substr(0, colon)] = name.substr(colon + 1);
    }
  }
}
  
static std::unordered_map<std::string, std::string>* LoadPropertiesSet(std::string p) {
  if(p == "properties"){
    auto * properties = new std::unordered_map<std::string, std::string>();
    AddProperty(properties, speedb_build_git_sha);
    AddProperty(properties, speedb_build_git_tag);
    AddProperty(properties, speedb_build_date);
    if (SPDB_BUILD_TAG[0] == '@') {
      AddProperty(properties, "?");
    } else {
      AddProperty(properties, speedb_build_tag);
    }
    return properties;
  } else {
    auto * debug_properties = new std::unordered_map<std::string, std::string>();
    AddProperty(debug_properties, use_rtti);
    AddProperty(debug_properties, debug_level);
    AddProperty(debug_properties, portable);
    return debug_properties;
  }
}

const std::unordered_map<std::string, std::string>& GetRocksBuildProperties() {
  static std::unique_ptr<std::unordered_map<std::string, std::string>> props(LoadPropertiesSet("properties"));
  return *props;
}
const std::unordered_map<std::string, std::string>& GetRocksDebugProperties() {
  static std::unique_ptr<std::unordered_map<std::string, std::string>> props(LoadPropertiesSet("debug_properties"));
  return *props;
}

std::string GetRocksVersionAsString(bool with_patch) {
  std::string version = std::to_string(ROCKSDB_MAJOR) + "." + std::to_string(ROCKSDB_MINOR);
  if (with_patch) {
    return version + "." + std::to_string(ROCKSDB_PATCH);
  } else {
    return version;
  }
}

std::string GetSpeedbVersionAsString(bool with_patch) {
  std::string version = std::to_string(SPEEDB_MAJOR) + "." + std::to_string(SPEEDB_MINOR);
  if (with_patch) {
    version += "." + std::to_string(SPEEDB_PATCH);
    // Only add a build tag if it was specified (e.g. not a release build)
    if (SPDB_BUILD_TAG[0] != '\0') {
      if (SPDB_BUILD_TAG[0] == '@') {
        // In case build tag substitution at build time failed, add a question mark
        version += "-?";
      } else {
        version += "-" + std::string(SPDB_BUILD_TAG);
      }
    }
  }
  return version;
}

std::string GetRocksBuildInfoAsString(const std::string& program, bool verbose) {
  std::string info = program + " (Speedb) " + GetSpeedbVersionAsString(true) +
                     " (" + GetRocksVersionAsString(true) + ")";
  if (verbose) {
    for (const auto& it : GetRocksBuildProperties()) {
      info.append("\n    ");
      info.append(it.first);
      info.append(": ");
      info.append(it.second);
    }
    info.append("\n Build properties:");
    info.append(GetRocksDebugPropertiesAsString());
  }
  return info;
}

std::string GetRocksDebugPropertiesAsString() {
  std::string info;
  for (const auto& it : GetRocksDebugProperties()) {
    info.append(" ");
    info.append(it.first);
    info.append("=");
    info.append(it.second);
  }
  return info;
}
} // namespace ROCKSDB_NAMESPACE
