#pragma once
#include <string>
#include <utility>

// High level operations, one-to-one with the original Python script's
// process_decrypt / process_encrypt functions.
namespace SaveTool {

// Returns {success, message}.
std::pair<bool, std::string> decrypt(const std::string& baseName, bool prettify);
std::pair<bool, std::string> encrypt(const std::string& baseName);

}
