#ifndef CONFIG_EXTRACTOR_H
#define CONFIG_EXTRACTOR_H

#include <string>
#include <map>

std::map<std::string, std::string> parseConfigFile(const std::string& filepath);
void displayConfig(const std::string& title, const std::map<std::string, std::string>& config);

#endif // CONFIG_EXTRACTOR_H